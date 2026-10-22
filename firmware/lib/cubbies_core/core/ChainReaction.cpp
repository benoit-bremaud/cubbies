#include "core/ChainReaction.h"

namespace cubbies {

// Red phase (ADR-0014): stubs, the tests of 05b fail first.
bool ChainReaction::start(uint32_t, uint8_t, uint8_t, uint8_t, Chain&) { return false; }
void ChainReaction::hear(const Chain&, uint32_t, uint8_t) {}
ChainFire ChainReaction::tick(uint32_t, bool) { return {}; }
bool ChainReaction::seen(const Chain&, uint32_t) const { return false; }
void ChainReaction::remember(const Chain&, uint32_t) {}

}  // namespace cubbies
