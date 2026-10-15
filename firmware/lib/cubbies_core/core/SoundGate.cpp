#include "core/SoundGate.h"

namespace cubbies {

SoundGate::SoundGate(IClock& clock, IAudio& audio) : clock_(clock), audio_(audio) {}

uint8_t SoundGate::rank(SoundId) { return 0; }

uint32_t SoundGate::durationMs(SoundId) { return 0; }

bool SoundGate::play(SoundId) { return false; }

void SoundGate::setSleepState(SleepState s) { sleep_ = s; }

void SoundGate::tick() {}

}  // namespace cubbies
