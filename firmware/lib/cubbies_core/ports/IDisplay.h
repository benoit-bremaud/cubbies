// Port: the face. One real adapter (SSD1331 over SPI), two test doubles (RecordingDisplay,
// PNG writer for golden images). The renderer owns the framebuffer; the adapter only pushes it.
#pragma once

#include <cstdint>

namespace cubbies {

class Framebuffer;

enum class Brightness : uint8_t { Minimum = 0, Low = 1, Full = 2 };

class IDisplay {
 public:
  virtual ~IDisplay() = default;
  virtual void present(const Framebuffer& frame) = 0;
  virtual void setBrightness(Brightness level) = 0;
};

}  // namespace cubbies
