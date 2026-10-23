#include "core/Choir.h"

#include "ports/IClock.h"

namespace cubbies {

bool Choir::start(uint32_t now, bool awake, uint8_t paletteIndex, uint8_t presentNeighbours, ChoirStart& out) {
  if (!awake || presentNeighbours == 0) return false;  // rows 1, 2
  if (pending_ || cooling(now)) return false;           // row 3
  out = ChoirStart{paletteIndex, kChoirMelody, kLead};
  pending_ = true;
  startAt_ = now + kLeadMs;
  return true;
}

void Choir::hear(const ChoirStart& c, uint32_t rxMs) {
  if (pending_) return;  // row 6; the cooldown (row 7) is judged in tick(), on the tick's own now:
                         // a frame stamped before the last start must not slip past it
  pending_ = true;
  startAt_ = rxMs + c.lead * kChoirLeadUnitMs;  // anchored on the receive time (05c "shared clock")
}

ChoirCue Choir::tick(uint32_t now, bool awake, uint8_t rank) {
  if (!pending_) return {};
  pending_ = false;
  if (!awake || cooling(now)) return {};                         // rows 5, 7
  const int32_t rest = static_cast<int32_t>(startAt_ - now);  // wrap-safe
  if (rest <= 0) return {};                                      // row 8: too late, no cooldown
  // The cooldown runs from this tick, up to kLeadMs before the audible start; it outlasts the
  // longest rest plus the melody, which is what keeps a singing cube out of a second choir (row 6).
  started_ = true;
  lastStart_ = now;
  return ChoirCue{true, static_cast<uint8_t>(rank % kVoices), static_cast<uint32_t>(rest)};  // rows 4, 10
}

bool Choir::cooling(uint32_t now) const { return started_ && ageMs(now, lastStart_) < kCooldownMs; }

}  // namespace cubbies
