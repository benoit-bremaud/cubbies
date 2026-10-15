// Procedural eyes in the identity colour (S1, REQ-F-01, REQ-NF-08). Two rounded rectangles
// on black; lids shaped by the mood; blinks with 2–6 s of open eyes between them; idle glances
// from the Prng; the whole frame drifts one pixel per minute around a 4-point square (full loop
// 4 min) in every state, closed eyes included, against OLED burn-in.
// Owns the single framebuffer and the IDisplay port; knows nothing about sleep or sound.
// Memory: the framebuffer (12 KB) lives inside this object, so on the MCU the renderer must be
// a static or global in main.cpp, never a local variable (the loop task stack is 8 KB).
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
  void setBrightness(Brightness level);  // forwarded to the port; the policy is the sleep cycle's
  // Gaze offset in pixels, clamped to ±kMaxLook.
  void setLook(int8_t dx, int8_t dy);
  // Idle glances drive setLook(); disable while something else owns the gaze (reactions in F2,
  // deterministic frames in tests).
  void setIdleGlances(bool enabled);
  void closeEyes();   // sleep: lids close over kLidMoveMs and stay closed; no blinks while closed
  void openEyes();
  void blinkNow();    // forces a blink (also used by the reaction animations later)

  // Renders the current state and presents it. Call once per tick (kTickMs).
  void tick();

  // Timings (tuning values, FS1 and 03-component tick budget). Blink phases are aligned on the
  // 40 ms tick: starting at a tick, frames are half (0, 40 ms), closed (80, 120 ms), half
  // (160 ms), open again from 200 ms.
  static constexpr uint32_t kTickMs = 40;
  static constexpr uint32_t kBlinkMinMs = 2000;   // open eyes between two blinks, min
  static constexpr uint32_t kBlinkMaxMs = 6000;   // and max (FS1 "blink every 2–6 s")
  static constexpr uint32_t kBlinkHalfMs = 50;    // first half-closed phase (one tick, rounded up)
  static constexpr uint32_t kBlinkClosedMs = 80;  // closed hold (two ticks)
  static constexpr uint32_t kBlinkTotalMs = kBlinkHalfMs + kBlinkClosedMs + kTickMs;
  static constexpr uint32_t kLidMoveMs = 240;     // sleep lid move (six ticks)
  static constexpr uint32_t kDriftPeriodMs = 60000;
  static constexpr uint32_t kGlanceMinMs = 1500;
  static constexpr uint32_t kGlanceMaxMs = 5000;
  static constexpr int8_t kMaxLook = 6;

  // Geometry (tuning values; the OLED is 96×64).
  static constexpr int kEyeW = 26;
  static constexpr int kEyeH = 34;
  static constexpr int kEyeGap = 14;
  static constexpr int kEyeRadius = 9;

 private:
  struct Lids {        // fraction 0..255 of the eye height covered from each side
    uint8_t top;
    uint8_t bottom;
    int8_t topSlant;   // positive = inner corner lower (annoyed)
    bool smile;        // bottom carved by a circle: the upward arc of a happy eye
  };
  Lids lidsForMood(Mood mood) const;
  void drawEye(int cx, int cy, uint8_t openness, Lids lids, bool left);
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
  bool lidsSettled_ = true;        // no lid move in progress
  uint32_t lidMoveStartMs_ = 0;
  uint32_t nextBlinkMs_ = 0;
  uint32_t blinkStartMs_ = 0;
  bool blinking_ = false;
  uint32_t nextGlanceMs_ = 0;
  uint32_t driftStartMs_ = 0;
  bool firstTick_ = true;
};

}  // namespace cubbies
