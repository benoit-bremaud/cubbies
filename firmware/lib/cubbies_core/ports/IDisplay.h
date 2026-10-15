// Port: the face. One real adapter (SSD1331 over SPI, brick F5); test double: RecordingDisplay
// (golden images are written from its last frame through test/support/Png.h).
// Single buffer: the renderer owns the only framebuffer and redraws it every tick, so
// present() must finish reading `frame` before it returns (a DMA adapter copies it or waits
// for the transfer) and the adapter keeps no reference to it.
// Brightness: three levels used by the sleep cycle (05b): Full awake, Low dozing, Minimum
// asleep; the adapter maps them to panel contrast values.
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
