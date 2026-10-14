// Procedural eyes in the identity colour (S1, REQ-F-01, REQ-NF-08). Two rounded rectangles
// on black; lids shaped by the mood; blinks every 2–6 s; idle glances from the Prng; the whole
// frame drifts one pixel every 60 s against OLED burn-in in every state, closed eyes included.
// Owns the framebuffer and the IDisplay port; knows nothing about sleep or sound.
#pragma once

#include <cstdint>

#include "core/Mood.h"
#include "core/Prng.h"
#include "eyes/Framebuffer.h"
#include "ports/IClock.h"
#include "ports/IDisplay.h"

namespace cubbies {

class EyesRenderer {
 public:
  EyesRenderer(IClock& clock, Prng& prng, IDisplay& display);

  void setColour(Rgb565 colour);
  void setMood(Mood mood);
  void setBrightness(Brightness level);
  // Gaze offset in pixels, clamped to ±kMaxLook; the idle glances use the same mechanism.
  void setLook(int8_t dx, int8_t dy);
  void setIdleGlances(bool enabled);
  void closeEyes();   // sleep: lids close over kLidMoveMs and stay closed
  void openEyes();
  void blinkNow();    // forces a blink (also used by the reaction animations later)

  // Renders the current state and presents it. Call once per tick.
  void tick();
  const Framebuffer& frame() const { return frame_; }

  static constexpr uint32_t kBlinkMinMs = 2000;
  static constexpr uint32_t kBlinkMaxMs = 6000;
  static constexpr uint32_t kBlinkClosedMs = 80;
  static constexpr uint32_t kLidMoveMs = 240;
  static constexpr uint32_t kDriftPeriodMs = 60000;
  static constexpr uint32_t kGlanceMinMs = 1500;
  static constexpr uint32_t kGlanceMaxMs = 5000;
  static constexpr int8_t kMaxLook = 6;

 private:
  struct Lids {       // fraction 0..255 of the eye height covered from each side
    uint8_t top;
    uint8_t bottom;
    int8_t topSlant;   // positive = inner corner lower (annoyed), negative = inner corner higher
    bool smile;        // bottom carved by a circle: the upward arc of a happy eye
  };
  Lids lidsForMood(Mood mood) const;
  void drawEye(int cx, int cy, int w, int h, uint8_t openness, Lids lids, bool left);
  void scheduleBlink();
  void scheduleGlance();
  uint8_t currentOpenness() const;  // 255 open .. 0 closed, after blink and sleep

  IClock& clock_;
  Prng& prng_;
  IDisplay& display_;
  Framebuffer frame_;

  Rgb565 colour_;
  Mood mood_ = Mood::Curious;
  int8_t lookX_ = 0;
  int8_t lookY_ = 0;
  bool idleGlances_ = true;
  bool eyesOpen_ = true;
  uint32_t lidMoveStartMs_ = 0;
  uint32_t nextBlinkMs_ = 0;
  uint32_t blinkStartMs_ = 0;
  bool blinking_ = false;
  uint32_t nextGlanceMs_ = 0;
  uint32_t driftStartMs_ = 0;
  int8_t driftX_ = 0;
  int8_t driftY_ = 0;
  bool firstTick_ = true;
};

}  // namespace cubbies
