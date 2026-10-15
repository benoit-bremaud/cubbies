// Invitation window (identity/05a): after boot, waits T_settle_touch for the TTP223 to calibrate,
// then shows a question mark and a gauge for T_invite; a press held T_secret enters the ritual.
// Ticks the Gesture classifier itself and ignores its gestures.
#pragma once

#include <cstdint>

#include "core/GestureClassifier.h"
#include "core/IdentityOutput.h"
#include "ports/IClock.h"

namespace cubbies {

class InvitationWindow {
 public:
  InvitationWindow(IClock& clock, GestureClassifier& gestures);

  void start();             // once the eyes show the colour
  IdentityOutput tick();    // once per loop until entered() or declined()
  bool entered() const { return state_ == State::Entered; }
  bool declined() const { return state_ == State::Declined; }

  // Tuning values (02a).
  static constexpr uint32_t kSettleMs = 1000;  // twice the TTP223 0.5 s calibration
  static constexpr uint32_t kInviteMs = 4000;  // counted from the end of the settle
  static constexpr uint32_t kSecretMs = 2000;  // hold that enters the ritual

 private:
  enum class State : uint8_t { Idle, Settling, Inviting, Holding, Entered, Declined };

  IdentityOutput decline();

  IClock& clock_;
  GestureClassifier& gestures_;
  State state_ = State::Idle;
  uint32_t sinceMs_ = 0;  // start() in Settling, end of the settle afterwards
};

}  // namespace cubbies
