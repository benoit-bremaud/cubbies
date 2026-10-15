#include "core/GestureClassifier.h"

namespace cubbies {

GestureClassifier::GestureClassifier(IClock& clock, ITouchSource& touch) : clock_(clock), touch_(touch) {}

Gesture GestureClassifier::tick() {
  const uint32_t now = clock_.nowMs();
  const bool down = touch_.pressed();
  const uint32_t age = ageMs(now, sinceMs_);

  switch (state_) {
    case State::Idle:
      if (!down) return Gesture::None;
      state_ = State::Pressed;
      sinceMs_ = now;
      return Gesture::TouchDown;

    case State::Pressed:
    case State::Pressed2:
      if (down) {
        if (age < kStrokeMs) return Gesture::None;
        state_ = State::Stroking;
        return Gesture::Stroke;
      }
      if (age >= kTapMaxMs) {
        state_ = State::Idle;
        return Gesture::Released;
      }
      if (state_ == State::Pressed2) {
        state_ = State::Idle;
        return Gesture::DoubleTap;
      }
      state_ = State::WaitSecond;
      sinceMs_ = now;
      return Gesture::None;

    case State::WaitSecond:
      if (down) {
        state_ = State::Pressed2;
        sinceMs_ = now;
        return Gesture::TouchDown;
      }
      if (age < kDoubleTapMs) return Gesture::None;
      state_ = State::Idle;
      return Gesture::Tap;

    case State::Stroking:
      if (down) return Gesture::None;
      state_ = State::Idle;
      return Gesture::Released;

    case State::Consumed:
      if (!down) state_ = State::Idle;
      return Gesture::None;
  }
  return Gesture::None;
}

uint32_t GestureClassifier::holdMs() const {
  const bool held = state_ == State::Pressed || state_ == State::Pressed2 || state_ == State::Stroking;
  return held ? ageMs(clock_.nowMs(), sinceMs_) : 0;
}

void GestureClassifier::consumeCurrentPress() { state_ = State::Consumed; }

}  // namespace cubbies
