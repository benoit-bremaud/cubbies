#include "eyes/Framebuffer.h"

namespace cubbies {

namespace {
// Clip a [start, start + length) span to [0, limit) with 64-bit arithmetic, so extreme inputs
// neither overflow nor reach the pixel array. Returns false when nothing remains.
bool clipSpan(int start, int length, int limit, int& lo, int& hi) {
  if (length <= 0 || start >= limit) return false;
  const int64_t end = static_cast<int64_t>(start) + length;
  if (end <= 0) return false;
  lo = start < 0 ? 0 : start;
  hi = end > limit ? limit : static_cast<int>(end);
  return lo < hi;
}
}  // namespace

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
  int x0, x1, y0, y1;
  if (!clipSpan(x, w, kWidth, x0, x1) || !clipSpan(y, h, kHeight, y0, y1)) return;
  for (int yy = y0; yy < y1; ++yy)
    for (int xx = x0; xx < x1; ++xx) pixels_[static_cast<size_t>(yy) * kWidth + xx] = colour.value;
}

void Framebuffer::fillRoundRect(int x, int y, int w, int h, int r, Rgb565 colour) {
  int lo, hi;
  if (!clipSpan(x, w, kWidth, lo, hi) || !clipSpan(y, h, kHeight, lo, hi)) return;
  // A rectangle far larger than the screen is a plain fill: no corner can be visible.
  if (w > 2 * kWidth || h > 2 * kHeight) {
    fillRect(x, y, w, h, colour);
    return;
  }
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
  // Reject a circle whose bounding box misses the screen (64-bit so huge values cannot wrap),
  // then cap the radius: beyond the screen diagonal the visible part is a full fill at most.
  const int64_t R = r;
  if (cx + R < 0 || cy + R < 0 || cx - R >= kWidth || cy - R >= kHeight) return;
  if (r > kWidth + kHeight) r = kWidth + kHeight;
  // Iterate only over the on-screen rows and columns of the bounding box.
  const int yLo = cy - r < 0 ? 0 : cy - r;
  const int yHi = cy + r > kHeight - 1 ? kHeight - 1 : cy + r;
  const int xLo = cx - r < 0 ? 0 : cx - r;
  const int xHi = cx + r > kWidth - 1 ? kWidth - 1 : cx + r;
  const int64_t r2 = static_cast<int64_t>(r) * r;
  for (int y = yLo; y <= yHi; ++y) {
    const int64_t dy = static_cast<int64_t>(y) - cy;
    for (int x = xLo; x <= xHi; ++x) {
      const int64_t dx = static_cast<int64_t>(x) - cx;
      if (dx * dx + dy * dy < r2) pixels_[static_cast<size_t>(y) * kWidth + x] = colour.value;
    }
  }
}

}  // namespace cubbies
