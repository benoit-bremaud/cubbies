#include "eyes/EyesRenderer.h"

namespace cubbies {

namespace {
constexpr int kCentreY = Framebuffer::kHeight / 2;
constexpr int kLeftCx = Framebuffer::kWidth / 2 - EyesRenderer::kEyeGap / 2 - EyesRenderer::kEyeW / 2;
constexpr int kRightCx = Framebuffer::kWidth / 2 + EyesRenderer::kEyeGap / 2 + EyesRenderer::kEyeW / 2;
// Drift: one-pixel steps around a square, one step per kDriftPeriodMs.
constexpr int8_t kDrift[4][2] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};

// Reaction and identity-frame shape rules (tuning values).
constexpr uint8_t kPurrOpenness = 80;          // nearly shut, the arc stays readable
constexpr uint8_t kYawnClosePhase = 153;       // 60 % of the yawn is the slow close
constexpr uint8_t kWakeOpenPhase = 192;        // 75 % opening, then the stretch
constexpr uint8_t kErrorOpenness = 60;
constexpr int kErrorSegments = 6;              // three on/off flashes
constexpr int kJoyBounceHeight = 4;            // pixels
constexpr int kSurpriseGrow = 6;               // pixels on both axes
constexpr int kInvitationShrink = 8;           // room for the gauge
constexpr int kSmileDepthPercent = 45;         // arc depth at the centre, % of the open height
constexpr int kSmileClosedDepthPercent = 60;   // deeper when nearly shut (purr)

int clampInt(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }
static_assert(EyesRenderer::kEyeW > 1, "the annoyed wedge divides by kEyeW - 1");

// Triangle wave 0..255..0 over a phase 0..255 (rises then falls).
uint8_t bump(uint8_t phase) { return phase < 128 ? static_cast<uint8_t>(phase * 2) : static_cast<uint8_t>((255 - phase) * 2); }
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
  lidMove_.start(clock_.nowMs(), kLidMoveMs);
}

void EyesRenderer::openEyes() {
  if (eyesOpen_) return;
  eyesOpen_ = true;
  lidsSettled_ = false;
  lidMove_.start(clock_.nowMs(), kLidMoveMs);
  scheduleBlink();  // the sleep-time deadline is stale: no blink on top of the lid opening
}

void EyesRenderer::blinkNow() {
  blink_.start(clock_.nowMs(), kBlinkTotalMs);
  blinking_ = true;
}

void EyesRenderer::scheduleBlink() {
  nextBlinkMs_ = clock_.nowMs() + kBlinkMinMs + prng_.uniform(kBlinkMaxMs - kBlinkMinMs + 1);
}

void EyesRenderer::scheduleGlance() {
  nextGlanceMs_ = clock_.nowMs() + kGlanceMinMs + prng_.uniform(kGlanceMaxMs - kGlanceMinMs + 1);
}

// ---- reactions ------------------------------------------------------------------------------

uint32_t EyesRenderer::reactionDurationMs(Reaction reaction) {
  switch (reaction) {
    case Reaction::TouchDown: return 0;     // held while pressed
    case Reaction::Surprise:  return 300;
    case Reaction::Joy:       return 500;
    case Reaction::Purr:      return 0;     // held until stopReaction()
    case Reaction::LookUp:    return 600;
    case Reaction::Yawn:      return 1200;
    case Reaction::Wake:      return 800;
    case Reaction::Greet:     return 400;
  }
  return 0;
}

void EyesRenderer::play(Reaction reaction) {
  if (reacting_ && reaction_ == reaction && reactionDurationMs(reaction) == 0 && !purrReleasing_) return;
  reacting_ = true;
  reaction_ = reaction;
  purrReleasing_ = false;
  const uint32_t d = reactionDurationMs(reaction);
  if (d > 0) reactionTl_.start(clock_.nowMs(), d);
  else if (reaction == Reaction::Purr) reactionTl_.start(clock_.nowMs(), kLidMoveMs);  // ramp in
  else reactionTl_.stop();
}

void EyesRenderer::stopReaction() {
  if (!reacting_ || reactionDurationMs(reaction_) != 0) return;
  if (reaction_ == Reaction::Purr && !purrReleasing_) {
    const uint32_t now = clock_.nowMs();
    purrFrom_ = purrTarget(now);                             // released mid-ramp: no snap
    purrReleasing_ = true;                                   // release over kLidMoveMs, then end
    reactionTl_.start(now, kLidMoveMs);
    return;
  }
  reacting_ = false;
  reactionTl_.stop();
}

// ---- identity frames ------------------------------------------------------------------------

void EyesRenderer::showInvitation(uint8_t progress) {
  identity_ = IdentityFrame::Invitation;
  invitationProgress_ = progress;
}
void EyesRenderer::showRitualFrame(Rgb565 candidate) {
  if (identity_ != IdentityFrame::Ritual) ritualPulse_.start(clock_.nowMs(), kRitualPulseMs);
  identity_ = IdentityFrame::Ritual;
  identityColour_ = candidate;
}
void EyesRenderer::playAdoption(Rgb565 chosen) {
  identity_ = IdentityFrame::Adoption;
  identityColour_ = chosen;
  identityTl_.start(clock_.nowMs(), kAdoptionMs);
}
void EyesRenderer::showError() {
  identity_ = IdentityFrame::Error;
  identityTl_.start(clock_.nowMs(), kErrorMs);
}
void EyesRenderer::clearIdentityFrame() {
  identity_ = IdentityFrame::None;
  identityTl_.stop();
  ritualPulse_.stop();
}

// ---- shapes ---------------------------------------------------------------------------------

EyesRenderer::Lids EyesRenderer::lidsForMood(Mood mood) {
  switch (mood) {
    case Mood::Curious: return {0, 0, 0, Arc::None};
    case Mood::Happy:   return {20, 0, 0, Arc::Smile};      // bottom carved into an upward arc
    case Mood::Bored:   return {90, 40, 0, Arc::None};      // flattened
    case Mood::Tired:   return {140, 0, 0, Arc::None};      // half closed
    case Mood::Annoyed: return {60, 0, 70, Arc::None};      // slanted top lids, inner corners low
  }
  return {0, 0, 0, Arc::None};
}

uint8_t EyesRenderer::currentOpenness(uint32_t now) const {
  uint8_t openness;
  if (lidsSettled_) {
    openness = eyesOpen_ ? 255 : 0;
  } else {
    const uint8_t frac = lidMove_.phase(now);
    openness = eyesOpen_ ? frac : static_cast<uint8_t>(255 - frac);
  }
  if (blink_.active(now)) {
    const uint32_t t = blink_.elapsed(now);
    if (t < kBlinkHalfMs) openness /= 2;
    else if (t < kBlinkHalfMs + kBlinkClosedMs) openness = 0;
    else openness /= 2;
  }
  return openness;
}

EyesRenderer::Shape EyesRenderer::moodShape(uint32_t now) const {
  return Shape{lidsForMood(mood_), kEyeW, kEyeH, 0, 0, currentOpenness(now), false};
}

// Purr openness ceiling: ramp 255 -> kPurrOpenness, hold, then release from where it was.
uint8_t EyesRenderer::purrTarget(uint32_t now) const {
  const uint8_t p = reactionTl_.phase(now);
  if (purrReleasing_) return static_cast<uint8_t>(purrFrom_ + ((255 - purrFrom_) * p) / 255);
  if (reactionTl_.active(now)) return static_cast<uint8_t>(255 - ((255 - kPurrOpenness) * p) / 255);
  return kPurrOpenness;  // ramp done: hold
}

void EyesRenderer::applyReaction(Shape& s, uint32_t now) const {
  const uint8_t p = reactionTl_.phase(now);
  const Lids open{0, 0, 0, Arc::None};
  switch (reaction_) {
    case Reaction::TouchDown:  // one notch wider while pressed
      s.w += 2; s.h += 4; s.lids = open;
      break;
    case Reaction::Surprise:   // round, big eyes
      s.w = kEyeW + kSurpriseGrow; s.h = kEyeH + kSurpriseGrow; s.lids = open;
      break;
    case Reaction::Joy: {      // happy arcs bouncing twice
      s.lids = lidsForMood(Mood::Happy);
      const uint8_t b = bump(static_cast<uint8_t>((p * 2) & 0xFF));
      s.dy = -(b * kJoyBounceHeight) / 255;
      break;
    }
    case Reaction::Purr: {     // soft closed arcs "^ ^": ramp down, hold, release; never opens
      s.lids = {0, 0, 0, Arc::SmileClosed};   // eyes the sleep cycle closed
      const uint8_t towards = purrTarget(now);
      if (towards < s.openness) s.openness = towards;
      break;
    }
    case Reaction::LookUp:     // eyes up and a little wider
      s.dy = -kMaxLook; s.h += 2; s.lids = open;
      break;
    case Reaction::Yawn: {     // slow close (first 60 %), then wide open
      if (p < kYawnClosePhase) s.openness = static_cast<uint8_t>(255 - (p * 255) / kYawnClosePhase);
      else { s.openness = 255; s.h += 4; s.lids = open; }
      break;
    }
    case Reaction::Wake: {     // progressive open and a stretch at the end
      s.openness = p < kWakeOpenPhase ? static_cast<uint8_t>((p * 255) / kWakeOpenPhase) : 255;
      if (p >= kWakeOpenPhase) s.h += 3;
      s.lids = open;
      break;
    }
    case Reaction::Greet: {    // quick glance to one side, then the other
      s.dx = p < 128 ? kMaxLook : -kMaxLook;
      break;
    }
  }
}

void EyesRenderer::drawEye(int cx, int cy, const Shape& s, bool left, Rgb565 colour) {
  const int w = s.w;
  const int h = s.h;
  const int topCover = (h * s.lids.top) / 255;
  const int bottomCover = (h * s.lids.bottom) / 255;
  int visible = h - topCover - bottomCover;
  if (visible < 2) visible = 2;
  const uint8_t openness = (left && s.winkLeft) ? 0 : s.openness;
  const int open = (visible * openness) / 255;
  const int x0 = cx - w / 2;
  if (open <= 1) {
    frame_.fillRect(x0, cy, w, 2, colour);  // a thin closed line keeps the eye located
    return;
  }
  const int top = cy - h / 2 + topCover + (visible - open) / 2;
  const int r = open < 2 * kEyeRadius ? open / 2 : kEyeRadius;
  frame_.fillRoundRect(x0, top, w, open, r, colour);
  // Smile: carve the bottom with a black circle so the eye becomes an upward arc. The circle
  // is sized so the arc is kSmileDepthPercent of the open height deep at the centre and meets
  // the sides just above the rounded corners, leaving no spikes. On short eyes the edge depth
  // shrinks with the centre depth, so the carve never vanishes and a closing eye stays monotonic.
  const bool carve = s.lids.arc == Arc::Smile ? openness > 128 : s.lids.arc == Arc::SmileClosed;
  if (carve) {
    const int percent = s.lids.arc == Arc::SmileClosed ? kSmileClosedDepthPercent : kSmileDepthPercent;
    const int depthCentre = (open * percent) / 100;
    const int depthEdge = r + 1 < depthCentre ? r + 1 : depthCentre - 1;
    if (depthCentre >= 2) {
      const int half = w / 2;
      const int dd = depthCentre - depthEdge;
      const int radius = (half * half + dd * dd) / (2 * dd);
      frame_.fillCircle(cx, top + open - depthCentre + radius, radius, kBlack);
    }
  }
  // Annoyed: a black wedge over the top lid, lower on the inner side.
  if (s.lids.topSlant != 0 && openness > 128) {
    const int wedge = (h * s.lids.topSlant) / 255;
    for (int i = 0; i < w; ++i) {
      const int inner = left ? (w - 1 - i) : i;  // 0 at the outer corner, w-1 at the inner corner
      const int cut = (wedge * inner) / (w - 1);
      if (cut > 0) frame_.fillRect(x0 + i, top, 1, cut, kBlack);
    }
  }
}

void EyesRenderer::drawQuestionMark(int cx, int cy, Rgb565 colour) {
  // A 7×11 question mark centred on (cx, cy): hook, stem, dot.
  frame_.fillRoundRect(cx - 3, cy - 5, 7, 5, 2, colour);
  frame_.fillRect(cx - 3, cy - 3, 3, 2, kBlack);
  frame_.fillRect(cx - 1, cy - 1, 3, 3, colour);
  frame_.fillRect(cx - 1, cy + 4, 3, 2, colour);
}

void EyesRenderer::drawGauge(uint8_t progress, Rgb565 colour) {
  const int x = 8, y = Framebuffer::kHeight - 6, w = Framebuffer::kWidth - 16, h = 3;
  frame_.fillRect(x - 1, y - 1, w + 2, 1, colour);  // top rail
  frame_.fillRect(x - 1, y + h, w + 2, 1, colour);  // bottom rail
  const int fill = (w * progress) / 255;
  if (fill > 0) frame_.fillRect(x, y, fill, h, colour);
}

void EyesRenderer::drawBorder(int thickness, Rgb565 colour) {
  frame_.fillRect(0, 0, Framebuffer::kWidth, thickness, colour);
  frame_.fillRect(0, Framebuffer::kHeight - thickness, Framebuffer::kWidth, thickness, colour);
  frame_.fillRect(0, 0, thickness, Framebuffer::kHeight, colour);
  frame_.fillRect(Framebuffer::kWidth - thickness, 0, thickness, Framebuffer::kHeight, colour);
}

// Identity frames drift like the mood frame; borders and the gauge hug the screen edge.
void EyesRenderer::drawIdentityFrame(uint32_t now, Shape s, int driftX, int driftY) {
  const int cy = kCentreY + driftY;
  const int lx = kLeftCx + driftX;
  const int rx = kRightCx + driftX;
  switch (identity_) {
    case IdentityFrame::Invitation: {
      s.h = kEyeH - kInvitationShrink;
      drawEye(lx, cy - 3, s, true, colour_);
      drawEye(rx, cy - 3, s, false, colour_);
      drawQuestionMark(Framebuffer::kWidth / 2 + driftX, cy - 3, colour_);
      drawGauge(invitationProgress_, colour_);
      break;
    }
    case IdentityFrame::Ritual: {
      s.w = kEyeW + 2; s.h = kEyeH + 2; s.lids = {0, 0, 0, Arc::None};
      drawEye(lx, cy, s, true, identityColour_);
      drawEye(rx, cy, s, false, identityColour_);
      drawBorder(ritualPulse_.phase(now) < 128 ? 2 : 1, identityColour_);   // pulsing border
      break;
    }
    case IdentityFrame::Adoption: {
      const uint8_t p = identityTl_.phase(now);
      s.lids = lidsForMood(Mood::Happy);
      s.winkLeft = p < 128;                             // wink first, then both happy
      drawEye(lx, cy, s, true, identityColour_);
      drawEye(rx, cy, s, false, identityColour_);
      if (p >= 128) drawBorder(2, identityColour_);
      break;
    }
    case IdentityFrame::Error: {
      const uint8_t p = identityTl_.phase(now);
      const bool on = ((p * kErrorSegments / 256) % 2) == 0;
      if (on) {
        s.openness = kErrorOpenness; s.lids = {0, 0, 0, Arc::None};
        drawEye(lx, cy, s, true, colour_);
        drawEye(rx, cy, s, false, colour_);
        drawBorder(3, colour_);
      } else {
        drawEye(lx, cy, s, true, colour_);
        drawEye(rx, cy, s, false, colour_);
      }
      break;
    }
    case IdentityFrame::None: break;
  }
}

// ---- tick -----------------------------------------------------------------------------------

void EyesRenderer::tick() {
  const uint32_t now = clock_.nowMs();
  if (firstTick_) {
    firstTick_ = false;
    driftStartMs_ = now;
    scheduleBlink();
    scheduleGlance();
  }
  if (!lidsSettled_ && !lidMove_.active(now)) {
    lidsSettled_ = true;
    lidMove_.stop();
  }
  // Reactions: timed ones end with their timeline; a releasing purr ends with its release.
  if (reacting_ && !reactionTl_.active(now) && (reactionDurationMs(reaction_) > 0 || purrReleasing_)) {
    reacting_ = false;
    purrReleasing_ = false;
    reactionTl_.stop();
  }
  if (identity_ == IdentityFrame::Adoption || identity_ == IdentityFrame::Error) {
    if (!identityTl_.active(now)) clearIdentityFrame();
  }
  if (identity_ == IdentityFrame::Ritual && !ritualPulse_.active(now)) ritualPulse_.start(now, kRitualPulseMs);
  // Blink lifecycle: only while the eyes are open and settled, no reaction, no identity frame
  // (the blink is deferred, not lost: the deadline stays due).
  if (blinking_ && !blink_.active(now)) {
    blinking_ = false;
    blink_.stop();
    scheduleBlink();  // the 2–6 s interval is measured from the reopening
  }
  if (!blinking_ && eyesOpen_ && lidsSettled_ && !reacting_ && identity_ == IdentityFrame::None &&
      isDue(now, nextBlinkMs_)) {
    blink_.start(now, kBlinkTotalMs);
    blinking_ = true;
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

  frame_.clear(kBlack);
  if (identity_ != IdentityFrame::None) {
    drawIdentityFrame(now, moodShape(now), driftX, driftY);
  } else {
    Shape s = moodShape(now);
    if (reacting_) applyReaction(s, now);
    const int cy = kCentreY + lookY_ + driftY + s.dy;
    drawEye(kLeftCx + lookX_ + driftX + s.dx, cy, s, true, colour_);
    drawEye(kRightCx + lookX_ + driftX + s.dx, cy, s, false, colour_);
  }
  display_.present(frame_);
}

}  // namespace cubbies
