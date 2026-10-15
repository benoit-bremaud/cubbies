// 96×64 RGB565 framebuffer (12 KB), the only thing the display port receives. Every
// primitive clips to the bounds and tolerates extreme coordinates, so callers never check.
// 12 KB by value: never a local on the target (the Arduino loop task stack is 8 KB); own it
// statically or inside a static owner (the EyesRenderer in main.cpp).
#pragma once

#include <cstddef>
#include <cstdint>

namespace cubbies {

struct Rgb565 {
  uint16_t value;
};

constexpr Rgb565 rgb565(uint8_t r, uint8_t g, uint8_t b) {
  return Rgb565{static_cast<uint16_t>(((r & 0xF8u) << 8) | ((g & 0xFCu) << 3) | (b >> 3))};
}

constexpr Rgb565 kBlack{0};

class Framebuffer {
 public:
  static constexpr int kWidth = 96;
  static constexpr int kHeight = 64;
  static constexpr size_t kPixels = static_cast<size_t>(kWidth) * kHeight;

  Framebuffer();

  void clear(Rgb565 colour);
  void fillRect(int x, int y, int w, int h, Rgb565 colour);
  // Rounded rectangle, corner radius r (clamped to half the smaller side).
  void fillRoundRect(int x, int y, int w, int h, int r, Rgb565 colour);
  void fillCircle(int cx, int cy, int r, Rgb565 colour);
  // Pixel at (x, y); black outside the bounds.
  Rgb565 at(int x, int y) const;
  const uint16_t* data() const { return pixels_; }

 private:
  void set(int x, int y, Rgb565 colour);
  uint16_t pixels_[kPixels];
};

}  // namespace cubbies
