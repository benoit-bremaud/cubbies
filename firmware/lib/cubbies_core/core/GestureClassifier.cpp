#include "core/GestureClassifier.h"

namespace cubbies {

GestureClassifier::GestureClassifier(IClock& clock, ITouchSource& touch) : clock_(clock), touch_(touch) {}

Gesture GestureClassifier::tick() { return Gesture::None; }

}  // namespace cubbies
