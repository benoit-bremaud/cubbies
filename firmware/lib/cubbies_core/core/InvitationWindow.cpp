#include "core/InvitationWindow.h"

namespace cubbies {

namespace {

IdentityOutput invitation(uint32_t holdMs) {
  IdentityOutput o;
  o.frame = IdentityFrame::Invitation;
  const uint32_t h = holdMs < InvitationWindow::kSecretMs ? holdMs : InvitationWindow::kSecretMs;
  o.progress = static_cast<uint8_t>(h * 255u / InvitationWindow::kSecretMs);  // clamped: late tick
  return o;
}

}  // namespace

// Hands the classifier back clean: a pending Tap or a finger still down must not reach the
// normal loop as a gesture.
IdentityOutput InvitationWindow::decline() {
  gestures_.consumeCurrentPress();
  state_ = State::Declined;
  IdentityOutput o;
  o.frame = IdentityFrame::Clear;
  return o;
}

InvitationWindow::InvitationWindow(IClock& clock, GestureClassifier& gestures)
    : clock_(clock), gestures_(gestures) {}

void InvitationWindow::start() {
  state_ = State::Settling;
  sinceMs_ = clock_.nowMs();
}

IdentityOutput InvitationWindow::tick() {
  gestures_.tick();  // samples the pad; gestures are not used here
  const uint32_t now = clock_.nowMs();
  const uint32_t age = ageMs(now, sinceMs_);
  const uint32_t hold = gestures_.holdMs();

  switch (state_) {
    case State::Settling:
      if (age < kSettleMs) return {};
      // Always: a press may have been calibrated away, or its edge is this very tick
      // (holdMs() still 0); with the pad released the classifier is back to Idle next tick.
      gestures_.consumeCurrentPress();
      state_ = State::Inviting;
      sinceMs_ = now;
      return invitation(0);

    case State::Inviting:
      // The press first: its edge tick reads holdMs() == 0, so a press begun on the last tick
      // inside T_invite is only seen held on the next one.
      if (hold > 0) {
        state_ = State::Holding;
        return invitation(hold);
      }
      if (age < kInviteMs) return {};
      return decline();

    case State::Holding:
      if (hold == 0) {
        if (age >= kInviteMs) return decline();
        state_ = State::Inviting;
        return invitation(0);
      }
      if (hold < kSecretMs) return invitation(hold);
      gestures_.consumeCurrentPress();
      state_ = State::Entered;
      return {};

    case State::Idle:
    case State::Entered:
    case State::Declined:
      return {};
  }
  return {};
}

}  // namespace cubbies
