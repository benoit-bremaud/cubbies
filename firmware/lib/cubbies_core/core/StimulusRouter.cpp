#include "core/StimulusRouter.h"

namespace cubbies {

Route route(Gesture, SleepState) { return {}; }

Route route(SoundClass, SleepState) { return {}; }

Route routeNewcomer(SleepState) { return {}; }

Route effects(MoodEffect) { return {}; }

Route effects(SleepEffect) { return {}; }

}  // namespace cubbies
