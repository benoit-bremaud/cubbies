#include "eyes/Framebuffer.h"

namespace cubbies {

Framebuffer::Framebuffer() { clear(kBlack); }

void Framebuffer::clear(Rgb565 colour) {
  for (size_t i = 0; i < kPixels; ++i) pixels_[i] = colour.value;
}

void Framebuffer::set(int x, int y, Rgb565 colour) {
  if (x < 0 || y < 0 || x >= kWidth || y >= kHeight) return;
  pixels_[static_cast<size_t>(y) * kWidth + x] = colour.value;
}

Rgb565 Framebuffer::at(int x, int y) const {
  if (x < 0 || y < 0 || x >= kWidth || y >= kHeight) return kBlack;
  return Rgb565{pixels_[static_cast<size_t>(y) * kWidth + x]};
}

void Framebuffer::fillRect(int x, int y, int w, int h, Rgb565 colour) {
  if (w <= 0 || h <= 0) return;
  const int x0 = x < 0 ? 0 : x;
  const int y0 = y < 0 ? 0 : y;
  const int x1 = (x + w > kWidth) ? kWidth : x + w;
  const int y1 = (y + h > kHeight) ? kHeight : y + h;
  for (int yy = y0; yy < y1; ++yy)
    for (int xx = x0; xx < x1; ++xx) pixels_[static_cast<size_t>(yy) * kWidth + xx] = colour.value;
}

void Framebuffer::fillRoundRect(int x, int y, int w, int h, int r, Rgb565 colour) {
  if (w <= 0 || h <= 0) return;
  const int maxR = (w < h ? w : h) / 2;
  if (r > maxR) r = maxR;
  if (r <= 0) {
    fillRect(x, y, w, h, colour);
    return;
  }
  // Centre band and the two side bands are plain rectangles; corners test the circle equation.
  fillRect(x + r, y, w - 2 * r, h, colour);
  fillRect(x, y + r, r, h - 2 * r, colour);
  fillRect(x + w - r, y + r, r, h - 2 * r, colour);
  const int r2 = r * r;
  for (int dy = 0; dy < r; ++dy) {
    for (int dx = 0; dx < r; ++dx) {
      const int cx = r - dx;  // distance from the corner's circle centre, in pixel units
      const int cy = r - dy;
      const int d2 = (cx - 1) * (cx - 1) + (cy - 1) * (cy - 1);
      if (d2 >= r2) continue;
      set(x + dx, y + dy, colour);                      // top-left
      set(x + w - 1 - dx, y + dy, colour);              // top-right
      set(x + dx, y + h - 1 - dy, colour);              // bottom-left
      set(x + w - 1 - dx, y + h - 1 - dy, colour);      // bottom-right
    }
  }
}

void Framebuffer::fillCircle(int cx, int cy, int r, Rgb565 colour) {
  if (r <= 0) return;
  const int r2 = r * r;
  for (int dy = -r; dy <= r; ++dy)
    for (int dx = -r; dx <= r; ++dx)
      if (dx * dx + dy * dy < r2) set(cx + dx, cy + dy, colour);
}

}  // namespace cubbies
