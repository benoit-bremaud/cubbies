// Test double for IAudio: records what reached the buzzer.
#pragma once

#include <cstdint>

#include "ports/IAudio.h"

namespace cubbies::test {

class RecordingAudio final : public IAudio {
 public:
  void play(const Note* notes, uint8_t count, VolumeLevel level) override {
    ++plays;
    lastNotes = notes;
    lastCount = count;
    lastLevel = level;
  }
  void stop() override { ++stops; }

  int plays = 0;
  int stops = 0;
  const Note* lastNotes = nullptr;
  uint8_t lastCount = 0;
  VolumeLevel lastLevel = 0;
};

}  // namespace cubbies::test
