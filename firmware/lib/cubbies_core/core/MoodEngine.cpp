#include "core/MoodEngine.h"

namespace cubbies {

MoodEngine::MoodEngine(IClock& clock) : clock_(clock) { restart(); }

void MoodEngine::restart() {}
MoodEffect MoodEngine::enter(Mood) { return MoodEffect::None; }
MoodEffect MoodEngine::stimulus(Stimulus) { return MoodEffect::None; }
MoodEffect MoodEngine::tick() { return MoodEffect::None; }

}  // namespace cubbies
