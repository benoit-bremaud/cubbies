// Sound gate (05e): the single sound egress. Owns the note table of every sound, applies the
// volume (constant in V1), stays silent unless the cube is Awake or Waking, and resolves
// overlaps by rank (a strictly higher rank replaces the sound in progress, anything else is
// dropped, nothing is queued).
#pragma once

#include <cstdint>

#include "core/SleepCycle.h"
#include "ports/IAudio.h"
#include "ports/IClock.h"

namespace cubbies {

enum class SoundId : uint8_t { Surprise, Joy, Purr, Hello, Yawn, Grumpy, WakeMelody };

class SoundGate {
 public:
  SoundGate(IClock& clock, IAudio& audio);

  bool play(SoundId id);               // true when forwarded to the buzzer
  void setSleepState(SleepState s);    // from main.cpp after each Sleep cycle tick
  void tick();                         // ends the sound in progress; call once per tick
  bool playing() const { return playing_; }

  static uint8_t rank(SoundId id);
  static uint32_t durationMs(SoundId id);  // sum of the note durations

  static constexpr VolumeLevel kVolume = 1;  // V1 default, ≤ 40 dB(A) at 1 m (REQ-NF-03)

 private:
  IClock& clock_;
  IAudio& audio_;
  SleepState sleep_ = SleepState::Awake;
  bool playing_ = false;
  SoundId current_ = SoundId::Surprise;
  uint32_t startMs_ = 0;
};

}  // namespace cubbies
