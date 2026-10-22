#include "core/ChainReaction.h"

#include "ports/IClock.h"

namespace cubbies {

bool ChainReaction::start(uint32_t now, bool awake, uint8_t paletteIndex, uint8_t presentNeighbours, Prng& prng,
                          Chain& out) {
  if (!awake || presentNeighbours == 0) return false;                   // rows 1, 2
  if (triggered_ && ageMs(now, lastTrigger_) < kTriggerMs) return false;  // row 3
  triggered_ = true;
  lastTrigger_ = now;
  reacted_ = true;  // the origin's own Joy counts as its chain reaction (row 12)
  lastReaction_ = now;
  out = Chain{paletteIndex, static_cast<uint8_t>(prng.uniform(256)), 0};
  remember(out, now);  // row 9
  return true;
}

void ChainReaction::hear(const Chain& c, uint32_t now, uint8_t rank) {
  if (seen(c, now)) return;  // rows 8, 9
  if (pending_) return;      // row 10: not remembered
  remember(c, now);
  if (reacted_ && ageMs(now, lastReaction_) < kCooldownMs) return;  // row 12
  pending_ = true;
  pendingFrame_ = c;
  fireAt_ = now + kStepMs * (1u + rank);  // row 4
}

ChainFire ChainReaction::tick(uint32_t now, bool awake) {
  ChainFire f;
  if (!pending_ || !isDue(now, fireAt_)) return f;
  pending_ = false;
  reacted_ = true;
  lastReaction_ = now;
  f.react = awake;                          // rows 5, 11
  f.relay = pendingFrame_.hop < kMaxHop;    // rows 6, 7
  f.frame = Chain{pendingFrame_.paletteIndex, pendingFrame_.eventId,
                  static_cast<uint8_t>(pendingFrame_.hop + (f.relay ? 1 : 0))};
  return f;
}

bool ChainReaction::seen(const Chain& c, uint32_t now) const {
  for (uint8_t i = 0; i < keyCount_; ++i) {
    const Key& k = keys_[i];
    if (k.palette == c.paletteIndex && k.id == c.eventId && ageMs(now, k.at) < kSeenMs) return true;  // row 14
  }
  return false;
}

void ChainReaction::remember(const Chain& c, uint32_t now) {
  keys_[keyHead_] = Key{c.paletteIndex, c.eventId, now};  // row 13: overwrites the oldest
  keyHead_ = static_cast<uint8_t>((keyHead_ + 1) % kSeenKeys);
  if (keyCount_ < kSeenKeys) ++keyCount_;
}

}  // namespace cubbies
