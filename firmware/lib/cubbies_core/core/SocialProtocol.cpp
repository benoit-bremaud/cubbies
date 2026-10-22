#include "core/SocialProtocol.h"

#include "core/PresenceCodec.h"

namespace cubbies {

namespace {
bool present(EntryState s) { return s == EntryState::Waiting || s == EntryState::Greeted; }
}  // namespace

SocialProtocol::SocialProtocol(IClock& clock, IRadio& radio, Prng& prng, const Mac& self)
    : clock_(clock), radio_(radio), prng_(prng), self_(self) {}

bool SocialProtocol::startChain(uint8_t paletteIndex) {
  Chain c{};
  if (!chain_.start(clock_.nowMs(), paletteIndex, static_cast<uint8_t>(prng_.uniform(256)), presentCount(), c))
    return false;
  uint8_t frame[kFrameSize];
  encodeChain(c, frame);
  radio_.broadcast(frame, kFrameSize);
  return true;
}

SocialOutput SocialProtocol::tick(const SelfState& self) {
  const uint32_t now = clock_.nowMs();
  RxFrame f;
  for (uint8_t i = 0; i < kMaxFramesPerTick && radio_.receive(f); ++i) receive(f);
  expire(now);
  SocialOutput out = greet(now);
  const ChainFire fire = chain_.tick(now, self.sleep == SleepState::Awake);
  out.chain = fire.react;
  if (fire.relay) {
    uint8_t frame[kFrameSize];
    encodeChain(fire.frame, frame);
    radio_.broadcast(frame, kFrameSize);
  }
  broadcastIfDue(now, self);
  return out;
}

void SocialProtocol::receive(const RxFrame& f) {
  Presence p{};
  DecodeResult r = decodePresence(f.bytes, f.len, p);
  if (r == DecodeResult::Type) {
    Chain c{};
    r = decodeChain(f.bytes, f.len, c);
    if (r == DecodeResult::Ok) {
      chain_.hear(c, f.nowMs, rank());
      return;
    }
  }
  if (r != DecodeResult::Ok) {
    count(r);
    return;
  }

  Neighbour* slot = findSlot(f.mac, f.nowMs);
  if (slot == nullptr) {
    ++stats_.dropFull;
    return;
  }
  if (slot->state == EntryState::Free) {
    slot->mac = f.mac;
  } else if (slot->state == EntryState::Absent && ageMs(f.nowMs, slot->lastSeen) > kRegreetMs) {
    slot->greeted = false;
  }
  slot->state = slot->greeted ? EntryState::Greeted : EntryState::Waiting;
  slot->paletteIndex = p.paletteIndex;
  slot->mood = p.mood;
  slot->lastSeen = f.nowMs;
}

void SocialProtocol::count(DecodeResult r) {
  switch (r) {
    case DecodeResult::Ok:
      break;
    case DecodeResult::Len:
      ++stats_.dropLen;
      break;
    case DecodeResult::Magic:
      ++stats_.dropMagic;
      break;
    case DecodeResult::Version:
      ++stats_.dropVersion;
      break;
    case DecodeResult::Type:
      ++stats_.dropType;
      break;
    case DecodeResult::Range:
      ++stats_.dropRange;
      break;
  }
}

uint8_t SocialProtocol::presentCount() const {
  uint8_t n = 0;
  for (const Neighbour& e : table_)
    if (present(e.state)) ++n;
  return n;
}

// Present neighbours with a lower MAC than this cube: its place in the chain wave (05b).
uint8_t SocialProtocol::rank() const {
  uint8_t n = 0;
  for (const Neighbour& e : table_)
    if (present(e.state) && e.mac < self_) ++n;
  return n;
}

// The known entry for `mac`, else a Free slot, else the Absent entry with the oldest lastSeen
// (reset to Free for the newcomer), else nullptr: present entries are never evicted.
Neighbour* SocialProtocol::findSlot(const Mac& mac, uint32_t now) {
  Neighbour* freeSlot = nullptr;
  Neighbour* oldestAbsent = nullptr;
  for (Neighbour& n : table_) {
    if (n.state == EntryState::Free) {
      if (freeSlot == nullptr) freeSlot = &n;
    } else if (n.mac == mac) {
      return &n;
    } else if (n.state == EntryState::Absent &&
               (oldestAbsent == nullptr || ageMs(now, n.lastSeen) > ageMs(now, oldestAbsent->lastSeen))) {
      oldestAbsent = &n;
    }
  }
  Neighbour* slot = freeSlot != nullptr ? freeSlot : oldestAbsent;
  if (slot != nullptr) *slot = Neighbour{};
  return slot;
}

void SocialProtocol::expire(uint32_t now) {
  for (Neighbour& n : table_) {
    if (present(n.state) && ageMs(now, n.lastSeen) > kTtlMs) n.state = EntryState::Absent;
    // Past T_regreet the entry would be greeted as new anyway: free it, so no Absent age grows
    // towards the 2^32 ms wrap where it would alias to fresh and mislead eviction (PR #22).
    if (n.state == EntryState::Absent && ageMs(now, n.lastSeen) > kRegreetMs) n.state = EntryState::Free;
  }
}

SocialOutput SocialProtocol::greet(uint32_t now) {
  if (greetCount_ > 0) {
    const uint8_t last = static_cast<uint8_t>((greetHead_ + kBudget - 1) % kBudget);
    const uint32_t sinceLast = ageMs(now, greetings_[last]);
    // Every recorded greeting is out of the window: forget them before they can alias (2^32 ms).
    if (sinceLast > kBudgetWindowMs) greetCount_ = 0;
    else if (sinceLast < kGreetCooldownMs) return {};
  }
  // Full ring: greetHead_ is the oldest of the last kBudget greetings, which must be more than
  // the window old (05a, strict).
  if (greetCount_ == kBudget && ageMs(now, greetings_[greetHead_]) <= kBudgetWindowMs) return {};

  for (Neighbour& n : table_) {
    if (n.state != EntryState::Waiting) continue;
    n.state = EntryState::Greeted;
    n.greeted = true;
    greetings_[greetHead_] = now;
    greetHead_ = static_cast<uint8_t>((greetHead_ + 1) % kBudget);
    if (greetCount_ < kBudget) ++greetCount_;
    return SocialOutput{true, n.paletteIndex};
  }
  return {};
}

void SocialProtocol::broadcastIfDue(uint32_t now, const SelfState& self) {
  if (started_ && !isDue(now, nextBroadcast_)) return;
  started_ = true;
  uint8_t frame[kFrameSize];
  encodePresence(Presence{self.paletteIndex, moodByte(self.sleep, self.wakeOrigin, self.mood), self.adopted},
                 frame);
  radio_.broadcast(frame, kFrameSize);
  nextBroadcast_ = now + kPresenceMs - kJitterMs + prng_.uniform(2 * kJitterMs + 1);
}

MoodTally SocialProtocol::moodTally() const {
  MoodTally t;
  // Mood byte 5 (asleep) fails the kMoodCount bound; Tired is excluded explicitly (row 9).
  for (const Neighbour& n : table_)
    if (present(n.state) && n.mood < kMoodCount && n.mood != static_cast<uint8_t>(Mood::Tired)) ++t.votes[n.mood];
  return t;
}

}  // namespace cubbies
