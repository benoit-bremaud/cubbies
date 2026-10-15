#include "core/SocialProtocol.h"

namespace cubbies {

SocialProtocol::SocialProtocol(IClock& clock, IRadio& radio, Prng& prng)
    : clock_(clock), radio_(radio), prng_(prng) {}

SocialOutput SocialProtocol::tick(const SelfState&) { return {}; }

}  // namespace cubbies
