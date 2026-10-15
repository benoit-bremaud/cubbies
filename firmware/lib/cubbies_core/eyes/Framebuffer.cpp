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
  const int maxR = (w < h ? w : h) / 2;
  if (r > maxR) r = maxR;
  if (r <= 0) {
    fillRect(x, y, w, h, colour);
    return;
  }
  // Centre band and the two side bands are plain rectangles (64-bit positions, then clipped).
  const int64_t X = x, Y = y, W = w, H = h;
  auto band = [&](int64_t bx, int64_t by, int64_t bw, int64_t bh) {
    if (bx >= kWidth || by >= kHeight || bx + bw <= 0 || by + bh <= 0) return;
    const int cx0 = bx < 0 ? 0 : static_cast<int>(bx);
    const int cy0 = by < 0 ? 0 : static_cast<int>(by);
    const int cx1 = bx + bw > kWidth ? kWidth : static_cast<int>(bx + bw);
    const int cy1 = by + bh > kHeight ? kHeight : static_cast<int>(by + bh);
    for (int yy = cy0; yy < cy1; ++yy)
      for (int xx = cx0; xx < cx1; ++xx) pixels_[static_cast<size_t>(yy) * kWidth + xx] = colour.value;
  };
  band(X + r, Y, W - 2 * r, H);
  band(X, Y + r, r, H - 2 * r);
  band(X + W - r, Y + r, r, H - 2 * r);
  // Corners: each corner block is r×r; only its on-screen part is visited, so the work is
  // bounded by the screen whatever r is. Pixel (ox + sx*dx, oy + sy*dy) is inside the corner
  // arc when (r-dx-1)^2 + (r-dy-1)^2 < r^2.
  const int64_t r2 = static_cast<int64_t>(r) * r;
  const int64_t corners[4][4] = {{X, Y, 1, 1}, {X + W - 1, Y, -1, 1}, {X, Y + H - 1, 1, -1}, {X + W - 1, Y + H - 1, -1, -1}};
  for (const auto& c : corners) {
    const int64_t ox = c[0], oy = c[1], sx = c[2], sy = c[3];
    for (int64_t dy = 0; dy < r; ++dy) {
      const int64_t py = oy + sy * dy;
      if (py < 0 || py >= kHeight) continue;
      const int64_t cy = r - dy - 1;
      const int64_t rem = r2 - cy * cy;
      if (rem <= 0) continue;
      for (int64_t dx = 0; dx < r; ++dx) {
        const int64_t px = ox + sx * dx;
        if (px < 0 || px >= kWidth) continue;
        const int64_t cx = r - dx - 1;
        if (cx * cx < rem) pixels_[static_cast<size_t>(py) * kWidth + static_cast<size_t>(px)] = colour.value;
      }
    }
  }
}

void Framebuffer::fillCircle(int cx, int cy, int r, Rgb565 colour) {
  if (r <= 0) return;
  // Everything in 64 bits: the bounding box is intersected with the screen (so the work is
  // bounded by the screen, not by r), the radius is never altered, and the distance test uses
  // the remainder r^2 - dy^2 so that two squares are never added (each fits in 64 bits).
  const int64_t CX = cx, CY = cy, R = r;
  if (CX + R <= 0 || CY + R <= 0 || CX - R >= kWidth || CY - R >= kHeight) return;
  const int yLo = CY - R < 0 ? 0 : static_cast<int>(CY - R);
  const int yHi = CY + R > kHeight - 1 ? kHeight - 1 : static_cast<int>(CY + R);
  const int xLo = CX - R < 0 ? 0 : static_cast<int>(CX - R);
  const int xHi = CX + R > kWidth - 1 ? kWidth - 1 : static_cast<int>(CX + R);
  const int64_t r2 = R * R;
  for (int y = yLo; y <= yHi; ++y) {
    const int64_t dy = y - CY;
    const int64_t rem = r2 - dy * dy;
    if (rem <= 0) continue;
    for (int x = xLo; x <= xHi; ++x) {
      const int64_t dx = x - CX;
      if (dx * dx < rem) pixels_[static_cast<size_t>(y) * kWidth + x] = colour.value;
    }
  }
}

}  // namespace cubbies
