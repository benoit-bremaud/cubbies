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

enum class SoundId : uint8_t { Surprise, Joy, Purr, Hello, Yawn, Grumpy, WakeMelody,
                               RitualChime, RitualTick, Adoption,  // ritual sounds: identity/05b
                               ChoirLow, ChoirMid, ChoirHigh };    // choir voices: social-group/05c
constexpr uint8_t kSoundCount = 13;

constexpr uint8_t kChoirVoices =
    static_cast<uint8_t>(SoundId::ChoirHigh) - static_cast<uint8_t>(SoundId::ChoirLow) + 1;
// The choir voice of a voice index (Choir already reduces the rank mod Choir::kVoices; the mod
// here only keeps the SoundId in range). main.cpp asserts both counts agree.
SoundId choirVoice(uint8_t voice);

class SoundGate {
 public:
  SoundGate(IClock& clock, IAudio& audio);

  // True when forwarded to the buzzer; with `restMs`, after a rest of `restMs` (≤ kMaxRestMs) played by the buzzer's own timer, so a choir
  // starts on the same millisecond on every cube whatever its tick phase (social-group/05c).
  bool play(SoundId id, uint32_t restMs = 0);
  // Sync with the Sleep cycle after every SleepCycle call that returns an effect and before
  // applying effects(): the wake-up melody starts on a gesture() or sound() that enters Waking.
  // Entering Dozing or Asleep stops the sound in progress.
  void setSleepState(SleepState s);
  void tick();                         // ends the sound in progress; call once per tick
  bool playing() const { return playing_; }

  static uint8_t rank(SoundId id);         // a strictly higher rank replaces (05e)
  static uint32_t durationMs(SoundId id);  // sum of the note durations

  static constexpr uint32_t kMaxRestMs = 1000;  // the CHOIR lead bound (06)
  static constexpr uint8_t kMaxNotes = 8;       // upper bound on any note table (static_assert in .cpp)
  static constexpr VolumeLevel kVolume = 1;  // V1 default, ≤ 40 dB(A) at 1 m (REQ-NF-03)
  static_assert(kVolume <= kMaxVolume, "kVolume must be a level of the four-step scale");

 private:
  IClock& clock_;
  IAudio& audio_;
  SleepState sleep_ = SleepState::Awake;
  bool playing_ = false;
  SoundId current_ = SoundId::Surprise;
  uint32_t startMs_ = 0;
  uint32_t restMs_ = 0;
  Note buffer_[kMaxNotes + 1] = {};  // the rest then the notes, alive while they play
};

}  // namespace cubbies
