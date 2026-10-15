// Gesture classifier (05c): turns the one-bit touch level read each tick into TouchDown, Tap,
// DoubleTap, Stroke and Released, timed on the injected clock. At most one gesture per tick.
// A short press emits no Released: the touch-down stays held until Tap or DoubleTap replaces it.
// holdMs() and consumeCurrentPress() serve the invitation window (identity/05a).
#pragma once

#include <cstdint>

#include "ports/IClock.h"
#include "ports/ITouchSource.h"

namespace cubbies {

enum class Gesture : uint8_t { None, TouchDown, Tap, DoubleTap, Stroke, Released };

class GestureClassifier {
 public:
  GestureClassifier(IClock& clock, ITouchSource& touch);

  // Reads the pad and returns the gesture completed on this tick, if any. Call once per tick.
  Gesture tick();

  // Age of the current press in ms, 0 when released or consumed.
  uint32_t holdMs() const;
  // Swallows the rest of the current press (nothing is emitted until the pad is released) and
  // any pending Tap; with the pad released it returns to Idle at once.
  void consumeCurrentPress();

  // Tuning values (05c).
  static constexpr uint32_t kTapMaxMs = 400;     // longer presses are no tap
  static constexpr uint32_t kDoubleTapMs = 250;  // wait for a second press after a short one
  static constexpr uint32_t kStrokeMs = 1500;    // contact long enough to be a stroke

 private:
  enum class State : uint8_t { Idle, Pressed, WaitSecond, Pressed2, Stroking, Consumed };

  IClock& clock_;
  ITouchSource& touch_;
  State state_ = State::Idle;
  uint32_t sinceMs_ = 0;  // pressed edge in Pressed / Pressed2 / Stroking, release in WaitSecond
};

}  // namespace cubbies
