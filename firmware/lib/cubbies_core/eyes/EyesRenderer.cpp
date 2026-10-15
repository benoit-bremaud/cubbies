#include "eyes/EyesRenderer.h"

namespace cubbies {

namespace {
constexpr int kCentreY = Framebuffer::kHeight / 2;
constexpr int kLeftCx = Framebuffer::kWidth / 2 - EyesRenderer::kEyeGap / 2 - EyesRenderer::kEyeW / 2;
constexpr int kRightCx = Framebuffer::kWidth / 2 + EyesRenderer::kEyeGap / 2 + EyesRenderer::kEyeW / 2;
// Drift: one-pixel steps around a square, one step per kDriftPeriodMs.
constexpr int8_t kDrift[4][2] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};

int clampInt(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }
static_assert(EyesRenderer::kEyeW > 1, "the annoyed wedge divides by kEyeW - 1");
}  // namespace

EyesRenderer::EyesRenderer(IClock& clock, Prng& prng, IDisplay& display)
    : clock_(clock), prng_(prng), display_(display), colour_(rgb565(255, 255, 255)) {}

void EyesRenderer::setColour(Rgb565 colour) { colour_ = colour; }
void EyesRenderer::setMood(Mood mood) { mood_ = mood; }
void EyesRenderer::setBrightness(Brightness level) { display_.setBrightness(level); }
void EyesRenderer::setIdleGlances(bool enabled) { idleGlances_ = enabled; }

void EyesRenderer::setLook(int8_t dx, int8_t dy) {
  lookX_ = static_cast<int8_t>(clampInt(dx, -kMaxLook, kMaxLook));
  lookY_ = static_cast<int8_t>(clampInt(dy, -kMaxLook, kMaxLook));
}

void EyesRenderer::closeEyes() {
  if (!eyesOpen_) return;
  eyesOpen_ = false;
  lidsSettled_ = false;
  lidMoveStartMs_ = clock_.nowMs();
}

void EyesRenderer::openEyes() {
  if (eyesOpen_) return;
  eyesOpen_ = true;
  lidsSettled_ = false;
  lidMoveStartMs_ = clock_.nowMs();
  scheduleBlink();  // the next blink counts from the reopening, not from before the sleep
}

void EyesRenderer::blinkNow() {
  blinking_ = true;
  blinkStartMs_ = clock_.nowMs();
}

void EyesRenderer::scheduleBlink() {
  nextBlinkMs_ = clock_.nowMs() + kBlinkMinMs + prng_.uniform(kBlinkMaxMs - kBlinkMinMs + 1);
}

void EyesRenderer::scheduleGlance() {
  nextGlanceMs_ = clock_.nowMs() + kGlanceMinMs + prng_.uniform(kGlanceMaxMs - kGlanceMinMs + 1);
}

EyesRenderer::Lids EyesRenderer::lidsForMood(Mood mood) const {
  switch (mood) {
    case Mood::Curious: return {0, 0, 0, false};
    case Mood::Happy:   return {20, 0, 0, true};      // bottom carved into an upward arc
    case Mood::Bored:   return {90, 40, 0, false};    // flattened
    case Mood::Tired:   return {140, 0, 0, false};    // half closed
    case Mood::Annoyed: return {60, 0, 70, false};    // slanted top lids, inner corners low
  }
  return {0, 0, 0, false};
}

uint8_t EyesRenderer::currentOpenness() const {
  const uint32_t now = clock_.nowMs();
  uint8_t openness;
  if (lidsSettled_) {
    openness = eyesOpen_ ? 255 : 0;
  } else {
    const uint32_t moved = ageMs(now, lidMoveStartMs_);
    const uint32_t frac = moved >= kLidMoveMs ? 255 : (moved * 255) / kLidMoveMs;
    openness = static_cast<uint8_t>(eyesOpen_ ? frac : 255 - frac);
  }
  if (blinking_) {
    const uint32_t t = ageMs(now, blinkStartMs_);
    if (t < kBlinkHalfMs) openness /= 2;
    else if (t < kBlinkHalfMs + kBlinkClosedMs) openness = 0;
    else if (t < kBlinkTotalMs) openness /= 2;
  }
  return openness;
}

void EyesRenderer::drawEye(int cx, int cy, uint8_t openness, Lids lids, bool left) {
  const int w = kEyeW;
  const int h = kEyeH;
  // Visible vertical span after the mood lids, then after openness (blink / sleep).
  const int topCover = (h * lids.top) / 255;
  const int bottomCover = (h * lids.bottom) / 255;
  int visible = h - topCover - bottomCover;
  if (visible < 2) visible = 2;
  const int open = (visible * openness) / 255;
  const int x0 = cx - w / 2;
  if (open <= 1) {
    frame_.fillRect(x0, cy, w, 2, colour_);  // a thin closed line keeps the eye located
    return;
  }
  const int top = cy - h / 2 + topCover + (visible - open) / 2;
  const int r = open < 2 * kEyeRadius ? open / 2 : kEyeRadius;
  frame_.fillRoundRect(x0, top, w, open, r, colour_);
  // Happy: carve the bottom with a black circle so the eye becomes an upward arc. The circle
  // is sized so the arc is 45 % of the open height deep at the centre and meets the sides just
  // above the rounded corners (depth r + 1), leaving no spikes.
  if (lids.smile && openness > 128) {
    const int depthCentre = (open * 45) / 100;
    const int depthEdge = r + 1;
    if (depthCentre > depthEdge) {
      const int half = w / 2;
      const int dd = depthCentre - depthEdge;
      const int radius = (half * half + dd * dd) / (2 * dd);
      frame_.fillCircle(cx, top + open - depthCentre + radius, radius, kBlack);
    }
  }
  // Annoyed: a black wedge over the top lid, lower on the inner side.
  if (lids.topSlant != 0 && openness > 128) {
    const int wedge = (h * lids.topSlant) / 255;
    for (int i = 0; i < w; ++i) {
      const int inner = left ? (w - 1 - i) : i;  // 0 at the outer corner, w-1 at the inner corner
      const int cut = (wedge * inner) / (w - 1);
      if (cut > 0) frame_.fillRect(x0 + i, top, 1, cut, kBlack);
    }
  }
}

void EyesRenderer::tick() {
  const uint32_t now = clock_.nowMs();
  if (firstTick_) {
    firstTick_ = false;
    driftStartMs_ = now;
    scheduleBlink();
    scheduleGlance();
  }
  if (!lidsSettled_ && ageMs(now, lidMoveStartMs_) >= kLidMoveMs) lidsSettled_ = true;
  // Blink lifecycle: only while the eyes are open and settled.
  if (!blinking_ && eyesOpen_ && lidsSettled_ && isDue(now, nextBlinkMs_)) {
    blinking_ = true;
    blinkStartMs_ = now;
  }
  if (blinking_ && ageMs(now, blinkStartMs_) >= kBlinkTotalMs) {
    blinking_ = false;
    scheduleBlink();  // the 2–6 s interval is measured from the reopening
  }
  // Idle glances
  if (idleGlances_ && isDue(now, nextGlanceMs_)) {
    setLook(static_cast<int8_t>(static_cast<int>(prng_.uniform(2 * kMaxLook + 1)) - kMaxLook),
            static_cast<int8_t>(static_cast<int>(prng_.uniform(kMaxLook + 1)) - kMaxLook / 2));
    scheduleGlance();
  }
  const uint32_t driftStep = ageMs(now, driftStartMs_) / kDriftPeriodMs;
  const int driftX = kDrift[driftStep % 4][0];
  const int driftY = kDrift[driftStep % 4][1];

  const uint8_t openness = currentOpenness();
  const Lids lids = lidsForMood(mood_);
  frame_.clear(kBlack);
  const int cy = kCentreY + lookY_ + driftY;
  drawEye(kLeftCx + lookX_ + driftX, cy, openness, lids, true);
  drawEye(kRightCx + lookX_ + driftX, cy, openness, lids, false);
  display_.present(frame_);
}

}  // namespace cubbies
