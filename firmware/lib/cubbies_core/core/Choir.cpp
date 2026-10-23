#include "core/Choir.h"

#include "ports/IClock.h"

namespace cubbies {

bool Choir::start(uint32_t now, bool awake, uint8_t paletteIndex, uint8_t presentNeighbours, ChoirStart& out) {
  if (!awake || presentNeighbours == 0) return false;  // rows 1, 2
  if (pending_ || cooling(now)) return false;           // row 3
  out = ChoirStart{paletteIndex, 0, kLead};
  pending_ = true;
  startAt_ = now + kLeadMs;
  return true;
}

void Choir::hear(const ChoirStart& c, uint32_t rxMs) {
  if (pending_ || cooling(rxMs)) return;  // rows 6, 7
  pending_ = true;
  startAt_ = rxMs + c.lead * 10u;  // anchored on the receive time (05c "shared clock")
}

ChoirCue Choir::tick(uint32_t now, bool awake, uint8_t rank) {
  if (!pending_) return {};
  pending_ = false;
  if (!awake) return {};                                         // row 5
  const int32_t rest = static_cast<int32_t>(startAt_ - now);  // wrap-safe
  if (rest <= 0) return {};                                      // row 8: too late, no cooldown
  started_ = true;
  lastStart_ = now;
  return ChoirCue{true, static_cast<uint8_t>(rank % kVoices), static_cast<uint32_t>(rest)};  // rows 4, 10
}

bool Choir::cooling(uint32_t now) const { return started_ && ageMs(now, lastStart_) < kCooldownMs; }

}  // namespace cubbies
