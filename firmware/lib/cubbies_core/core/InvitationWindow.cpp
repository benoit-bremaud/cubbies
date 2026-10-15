#include "core/InvitationWindow.h"

namespace cubbies {

InvitationWindow::InvitationWindow(IClock& clock, GestureClassifier& gestures)
    : clock_(clock), gestures_(gestures) {}

void InvitationWindow::start() {}

IdentityOutput InvitationWindow::tick() { return {}; }

}  // namespace cubbies
