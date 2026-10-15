#include "core/SleepCycle.h"

namespace cubbies {

SleepCycle::SleepCycle(IClock& clock, MoodEngine& mood) : clock_(clock), mood_(mood) {}

Brightness SleepCycle::brightness() const { return Brightness::Full; }

SleepEffect SleepCycle::enter(SleepState s) {
  state_ = s;
  return SleepEffect::None;
}

SleepEffect SleepCycle::gesture(Gesture) { return SleepEffect::None; }

SleepEffect SleepCycle::sound(SoundClass) { return SleepEffect::None; }

SleepEffect SleepCycle::tick() { return SleepEffect::None; }

}  // namespace cubbies
