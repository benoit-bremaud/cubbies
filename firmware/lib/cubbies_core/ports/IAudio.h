// Port: the buzzer as a note sequence at a volume level, never a PWM duty (03-component, 05e).
// play() replaces any sequence in progress; the caller keeps `notes` alive until it is done.
// Real adapter: BuzzerAudio (F5), the only place a level becomes a duty; test double:
// RecordingAudio.
#pragma once

#include <cstdint>

namespace cubbies {

struct Note {
  uint16_t hz;  // 0 = rest
  uint16_t ms;
};

using VolumeLevel = uint8_t;  // 0..kMaxVolume, the four steps of the V1.x ritual
constexpr VolumeLevel kMaxVolume = 3;

class IAudio {
 public:
  virtual ~IAudio() = default;
  virtual void play(const Note* notes, uint8_t count, VolumeLevel level) = 0;
  virtual void stop() = 0;
};

}  // namespace cubbies
