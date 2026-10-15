// IDisplay adapter: SSD1331 96×64 OLED over 4-wire SPI with LovyanGFX 1.2.33 (ADR-0014).
// present() pushes the whole framebuffer and returns once the transfer is done, so the
// renderer may redraw it at once (IDisplay single-buffer contract). Brightness levels map to
// the panel's master current and contrast through LovyanGFX's setBrightness.
#pragma once

#include <LovyanGFX.hpp>

#include "ports/IDisplay.h"

namespace cubbies {

class Ssd1331Display final : public IDisplay {
 public:
  Ssd1331Display(int8_t sck, int8_t mosi, int8_t cs, int8_t dc);
  bool begin();

  void present(const Framebuffer& frame) override;
  void setBrightness(Brightness level) override;

 private:
  class Device : public lgfx::LGFX_Device {
   public:
    Device(int8_t sck, int8_t mosi, int8_t cs, int8_t dc);

   private:
    lgfx::Bus_SPI bus_;
    lgfx::Panel_SSD1331 panel_;
  };

  Device device_;
  bool ready_ = false;  // a failed init leaves the panel dark instead of driving a dead bus
};

}  // namespace cubbies
