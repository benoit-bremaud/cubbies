// Procedural eyes in the identity colour (S1, REQ-F-01, REQ-NF-08). Two rounded rectangles
// on black; lids shaped by the mood; blinks with 2–6 s of open eyes between them; idle glances
// from the Prng; the whole frame drifts one pixel per minute around a 4-point square (full loop
// 4 min) in every state, identity frames and closed eyes included, against OLED burn-in.
// Three layers, highest wins: identity frame > reaction overlay > mood frame.
//  - Identity frames replace the face: Invitation and Ritual hold until clearIdentityFrame();
//    Adoption and Error end on their own (kAdoptionMs, kErrorMs). A running reaction does not
//    alter an identity frame.
//  - Reactions reshape the mood frame; timed ones end after reactionDurationMs(), TouchDown and
//    Purr are held until stopReaction().
// Blinks are deferred (not dropped) while a reaction or an identity frame is showing.
// Owns the single framebuffer and the IDisplay port; knows nothing about sleep or sound.
// Memory: the framebuffer (12 KB) lives inside this object, so on the MCU the renderer must be
// a static or global in main.cpp, never a local variable (the loop task stack is 8 KB).
#pragma once

#include <cstdint>

#include "core/Mood.h"
#include "core/Prng.h"
#include "eyes/Framebuffer.h"
#include "eyes/Reaction.h"
#include "eyes/Timeline.h"
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
  // Idle glances drive setLook(); disable while something else owns the gaze (reactions,
  // deterministic frames in tests).
  void setIdleGlances(bool enabled);
  void closeEyes();   // sleep: lids close over kLidMoveMs and stay closed; no blinks while closed
  void openEyes();
  void blinkNow();    // forces a blink

  // Reactions (overlay). play() restarts a timed reaction; replaying a held one is a no-op.
  void play(Reaction reaction);
  void stopReaction();  // ends a held reaction (TouchDown, Purr); Purr releases over kLidMoveMs
  // Duration of the overlay in ms; 0 = held until stopReaction() (TouchDown, Purr). Tuning values.
  static uint32_t reactionDurationMs(Reaction reaction);

  // Identity frames (02a / 02b). Held frames stay until clearIdentityFrame().
  void showInvitation(uint8_t progress);        // question mark + gauge 0..255; call again to update; held
  void showRitualFrame(Rgb565 candidate);       // wide eyes in the candidate colour, pulsing border; held
  void playAdoption(Rgb565 chosen);             // wink then happy arcs with a border, kAdoptionMs, then clears
  void showError();                             // three on/off flashes over kErrorMs, then clears
  void clearIdentityFrame();                    // back to the mood frame (ends a held frame early)

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
  static constexpr uint32_t kLidMoveMs = 240;     // sleep lid move and purr ramp / release (six ticks)
  static constexpr uint32_t kDriftPeriodMs = 60000;
  static constexpr uint32_t kGlanceMinMs = 1500;
  static constexpr uint32_t kGlanceMaxMs = 5000;
  static constexpr uint32_t kAdoptionMs = 900;
  static constexpr uint32_t kErrorMs = 600;
  static constexpr uint32_t kRitualPulseMs = 800;
  static constexpr int8_t kMaxLook = 6;

  // Geometry (tuning values; the OLED is 96×64).
  static constexpr int kEyeW = 26;
  static constexpr int kEyeH = 34;
  static constexpr int kEyeGap = 14;
  static constexpr int kEyeRadius = 9;

 private:
  enum class Arc : uint8_t { None, Smile, SmileClosed };  // bottom edge: flat, carved, carved even when nearly shut
  struct Lids {        // fraction 0..255 of the eye height covered from each side
    uint8_t top;
    uint8_t bottom;
    int8_t topSlant;   // positive = inner corner lower (annoyed)
    Arc arc;
  };
  // What one frame draws, after the three layers are resolved.
  struct Shape {
    Lids lids;
    int w;             // eye width and height, after scaling
    int h;
    int dx;            // extra gaze offset added to lookX_/lookY_
    int dy;
    uint8_t openness;  // 255 open .. 0 closed
    bool winkLeft;     // left eye drawn closed (adoption)
  };
  enum class IdentityFrame : uint8_t { None, Invitation, Ritual, Adoption, Error };

  static Lids lidsForMood(Mood mood);
  Shape moodShape(uint32_t now) const;
  void applyReaction(Shape& s, uint32_t now) const;
  uint8_t purrTarget(uint32_t now) const;
  void drawEye(int cx, int cy, const Shape& s, bool left, Rgb565 colour);
  void drawIdentityFrame(uint32_t now, Shape s, int driftX, int driftY);
  void drawQuestionMark(int cx, int cy, Rgb565 colour);
  void drawGauge(uint8_t progress, Rgb565 colour);
  void drawBorder(int thickness, Rgb565 colour);
  void scheduleBlink();
  void scheduleGlance();
  uint8_t currentOpenness(uint32_t now) const;  // 255 open .. 0 closed, after blink and sleep

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
  Timeline lidMove_;
  bool lidsSettled_ = true;
  Timeline blink_;
  bool blinking_ = false;
  uint32_t nextBlinkMs_ = 0;
  uint32_t nextGlanceMs_ = 0;
  uint32_t driftStartMs_ = 0;
  bool firstTick_ = true;

  bool reacting_ = false;
  Reaction reaction_ = Reaction::TouchDown;
  Timeline reactionTl_;   // timed reactions, and the purr ramp in / release
  bool purrReleasing_ = false;
  uint8_t purrFrom_ = 0;  // openness reached when the release started (a purr may be released mid-ramp)

  IdentityFrame identity_ = IdentityFrame::None;
  uint8_t invitationProgress_ = 0;
  Rgb565 identityColour_{0};
  Timeline identityTl_;   // adoption and error
  Timeline ritualPulse_;  // restarted every kRitualPulseMs while the ritual frame shows
};

}  // namespace cubbies
