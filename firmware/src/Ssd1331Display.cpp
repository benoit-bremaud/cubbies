#include "Ssd1331Display.h"

#include "eyes/Framebuffer.h"

namespace cubbies {

namespace {

// SSD1331 write cycle ≥ 150 ns (6.67 MHz); 6 MHz gives ≈ 16 ms per frame (electronics-spec).
constexpr uint32_t kSpiHz = 6000000;

// Panel brightness per level, provisional until the bench (05b: Full awake, Low dozing,
// Minimum asleep).
constexpr uint8_t kBrightness[] = {24, 96, 255};  // indexed by Brightness
static_assert(sizeof(kBrightness) == static_cast<size_t>(Brightness::Full) + 1,
              "one panel value per Brightness level");

}  // namespace

Ssd1331Display::Device::Device(int8_t sck, int8_t mosi, int8_t cs, int8_t dc) {
  auto b = bus_.config();
  b.spi_host = SPI2_HOST;
  b.spi_mode = 0;
  b.freq_write = kSpiHz;
  b.pin_sclk = sck;
  b.pin_mosi = mosi;
  b.pin_miso = -1;
  b.pin_dc = dc;
  b.dma_channel = SPI_DMA_CH_AUTO;
  bus_.config(b);
  panel_.setBus(&bus_);

  auto p = panel_.config();
  p.pin_cs = cs;
  p.pin_rst = -1;  // reset held by the module's RC network: no GPIO left (ADR-0007)
  p.pin_busy = -1;
  p.panel_width = Framebuffer::kWidth;
  p.panel_height = Framebuffer::kHeight;
  p.readable = false;
  p.bus_shared = false;
  panel_.config(p);
  setPanel(&panel_);
}

Ssd1331Display::Ssd1331Display(int8_t sck, int8_t mosi, int8_t cs, int8_t dc)
    : device_(sck, mosi, cs, dc) {}

bool Ssd1331Display::begin() {
  ready_ = device_.init();
  return ready_;
}

void Ssd1331Display::present(const Framebuffer& frame) {
  if (!ready_) return;
  // Framebuffer pixels are native RGB565, the layout of lgfx::rgb565_t. pushImage blocks until
  // the transfer ends, so no reference to `frame` survives this call.
  device_.pushImage(0, 0, Framebuffer::kWidth, Framebuffer::kHeight,
                    reinterpret_cast<const lgfx::rgb565_t*>(frame.data()));
}

void Ssd1331Display::setBrightness(Brightness level) {
  if (!ready_) return;
  device_.setBrightness(kBrightness[static_cast<uint8_t>(level)]);
}

}  // namespace cubbies
