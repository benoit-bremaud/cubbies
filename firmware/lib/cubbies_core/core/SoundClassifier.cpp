#include "core/SoundClassifier.h"

namespace cubbies {

SoundClassifier::SoundClassifier(IClock& clock, IMic& mic) : clock_(clock), mic_(mic) {}

SoundClass SoundClassifier::tick() { return SoundClass::None; }

}  // namespace cubbies
