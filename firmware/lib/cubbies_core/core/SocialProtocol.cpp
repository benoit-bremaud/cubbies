#include "core/SocialProtocol.h"

#include "core/PresenceCodec.h"

namespace cubbies {

namespace {
bool present(EntryState s) { return s == EntryState::Waiting || s == EntryState::Greeted; }
}  // namespace

SocialProtocol::SocialProtocol(IClock& clock, IRadio& radio, Prng& prng)
    : clock_(clock), radio_(radio), prng_(prng) {}

SocialOutput SocialProtocol::tick(const SelfState& self) {
  const uint32_t now = clock_.nowMs();
  RxFrame f;
  for (uint8_t i = 0; i < kMaxFramesPerTick && radio_.receive(f); ++i) receive(f);
  expire(now);
  const SocialOutput out = greet(now);
  broadcastIfDue(now, self);
  return out;
}

void SocialProtocol::receive(const RxFrame& f) {
  Presence p{};
  switch (decodePresence(f.bytes, f.len, p)) {
    case DecodeResult::Ok:
      break;
    case DecodeResult::Len:
      ++stats_.dropLen;
      return;
    case DecodeResult::Magic:
      ++stats_.dropMagic;
      return;
    case DecodeResult::Version:
      ++stats_.dropVersion;
      return;
    case DecodeResult::Type:
      ++stats_.dropType;
      return;
    case DecodeResult::Range:
      ++stats_.dropRange;
      return;
  }

  Neighbour* slot = nullptr;
  Neighbour* freeSlot = nullptr;
  Neighbour* oldestAbsent = nullptr;
  for (Neighbour& n : table_) {
    if (n.state == EntryState::Free) {
      if (freeSlot == nullptr) freeSlot = &n;
    } else if (n.mac == f.mac) {
      slot = &n;
      break;
    } else if (n.state == EntryState::Absent &&
               (oldestAbsent == nullptr || ageMs(f.nowMs, n.lastSeen) > ageMs(f.nowMs, oldestAbsent->lastSeen))) {
      oldestAbsent = &n;
    }
  }

  if (slot == nullptr) {  // newcomer: Free first, else evict the oldest Absent
    slot = freeSlot != nullptr ? freeSlot : oldestAbsent;
    if (slot == nullptr) {
      ++stats_.dropFull;
      return;
    }
    *slot = Neighbour{};
    slot->mac = f.mac;
  } else if (slot->state == EntryState::Absent && ageMs(f.nowMs, slot->lastSeen) > kRegreetMs) {
    slot->greeted = false;
  }
  slot->state = slot->greeted ? EntryState::Greeted : EntryState::Waiting;
  slot->paletteIndex = p.paletteIndex;
  slot->mood = p.mood;
  slot->lastSeen = f.nowMs;
}

void SocialProtocol::expire(uint32_t now) {
  for (Neighbour& n : table_)
    if (present(n.state) && ageMs(now, n.lastSeen) > kTtlMs) n.state = EntryState::Absent;
}

SocialOutput SocialProtocol::greet(uint32_t now) {
  if (greetCount_ > 0) {
    const uint8_t last = static_cast<uint8_t>((greetHead_ + kBudget - 1) % kBudget);
    if (ageMs(now, greetings_[last]) < kGreetCooldownMs) return {};
  }
  // Full ring: greetHead_ is the oldest of the last kBudget greetings.
  if (greetCount_ == kBudget && ageMs(now, greetings_[greetHead_]) < kBudgetWindowMs) return {};

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

}  // namespace cubbies
