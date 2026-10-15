// Sound classifier (05d): turns the rms and peak read each tick into Voice, Clap or Loud.
// Thresholds are ratios over an adaptive baseline (slow running mean of the rms, frozen while a
// sound is in progress), with an absolute floor kMinLevel. At most one class per tick.
#pragma once

#include <cstdint>

#include "ports/IClock.h"
#include "ports/IMic.h"

namespace cubbies {

enum class SoundClass : uint8_t { None, Voice, Clap, Loud };

class SoundClassifier {
 public:
  SoundClassifier(IClock& clock, IMic& mic);

  // Reads the microphone and returns the class completed on this tick, if any. Call once per tick.
  SoundClass tick();

  // Tuning values (05d), calibrated on the bench.
  static constexpr uint32_t kVoiceRatio = 2;       // ≈ +6 dB
  static constexpr uint32_t kLoudRatio = 4;        // ≈ +12 dB
  static constexpr uint32_t kClapRatio = 8;        // ≈ +18 dB, on the peak
  static constexpr uint32_t kVoiceMs = 200;
  static constexpr uint32_t kLoudMs = 500;
  static constexpr uint32_t kClapMs = 100;         // a burst this long or longer is no clap
  static constexpr uint32_t kBaselineTicks = 250;  // running-mean time constant, 10 s at 40 ms
  static constexpr uint16_t kMinLevel = 64;        // floor of the reference (provisional)

 private:
  enum class State : uint8_t { Quiet, Burst, Sound };

  IClock& clock_;
  IMic& mic_;
  State state_ = State::Quiet;
  uint32_t sinceMs_ = 0;   // start of the burst or of the sound
  bool voiceSent_ = false;
  bool loudSent_ = false;
  bool seeded_ = false;
  uint32_t baselineQ8_ = 0;  // baseline rms × 256
};

}  // namespace cubbies
