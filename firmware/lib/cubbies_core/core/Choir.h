// Choir (social-group/05c): one start "in kLeadMs" per stroke, anchored on the receive time, one
// voice per cube from its MAC rank, a cooldown after any start, never relayed. Pure: the Social
// protocol feeds it decoded CHOIR frames, the time and the rank, and broadcasts what start()
// returns; main.cpp hands the cue to the Sound gate, whose lead rest makes every cube start on the
// same millisecond whatever its tick phase.
#pragma once

#include <cstdint>

#include "core/PresenceCodec.h"

namespace cubbies {

// What the choir asks for on a tick: sing `voice` after a rest of `restMs`.
struct ChoirCue {
  bool sing = false;
  uint8_t voice = 0;
  uint32_t restMs = 0;
};

class Choir {
 public:
  // Conductor side (rows 1–3): true and `out` set when a CHOIR frame must be sent; the
  // conductor's own start is then scheduled at now + kLeadMs.
  bool start(uint32_t now, bool awake, uint8_t paletteIndex, uint8_t presentNeighbours, ChoirStart& out);
  // Receiver side (rows 6, 7): schedules rxMs + lead × 10 ms unless a start is pending or the
  // cooldown runs.
  void hear(const ChoirStart& c, uint32_t rxMs);
  // Emits the pending start once (rows 4, 5, 8, 10); `rank` = present neighbours with a lower MAC.
  ChoirCue tick(uint32_t now, bool awake, uint8_t rank);

  // Tuning values (05c).
  static constexpr uint8_t kLead = 50;  // × 10 ms on the air
  static constexpr uint32_t kLeadMs = kLead * 10u;
  static constexpr uint32_t kCooldownMs = 60000;
  static constexpr uint8_t kVoices = 3;

 private:
  bool cooling(uint32_t now) const;

  bool pending_ = false;
  uint32_t startAt_ = 0;
  bool started_ = false;
  uint32_t lastStart_ = 0;
};

}  // namespace cubbies
