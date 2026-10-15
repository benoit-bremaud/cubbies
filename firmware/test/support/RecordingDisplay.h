// Test double for IDisplay: keeps the last presented frame and counts calls.
#pragma once

#include <cstdint>

#include "eyes/Framebuffer.h"
#include "ports/IDisplay.h"

namespace cubbies::test {

class RecordingDisplay final : public IDisplay {
 public:
  void present(const Framebuffer& frame) override {
    last = frame;
    ++presentCount;
  }
  void setBrightness(Brightness level) override {
    brightness = level;
    ++brightnessCount;
  }

  Framebuffer last;
  uint32_t presentCount = 0;
  uint32_t brightnessCount = 0;
  Brightness brightness = Brightness::Full;
};

}  // namespace cubbies::test
