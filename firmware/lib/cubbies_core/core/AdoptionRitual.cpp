#include "core/AdoptionRitual.h"

namespace cubbies {

AdoptionRitual::AdoptionRitual(IClock& clock, IStorage& storage) : clock_(clock), storage_(storage) {}

IdentityOutput AdoptionRitual::start(const Identity& identity) {
  identity_ = identity;
  return {};
}

IdentityOutput AdoptionRitual::tick(Gesture) { return {}; }

}  // namespace cubbies
