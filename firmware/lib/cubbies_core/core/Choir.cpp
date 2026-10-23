#include "core/Choir.h"

namespace cubbies {

bool Choir::start(uint32_t, bool, uint8_t, uint8_t, ChoirStart&) { return false; }

void Choir::hear(const ChoirStart&, uint32_t) {}

ChoirCue Choir::tick(uint32_t, bool, uint8_t) { return {}; }

bool Choir::cooling(uint32_t) const { return false; }

}  // namespace cubbies
