#include "core/Identity.h"

namespace cubbies {

Rgb565 paletteColour(uint8_t) { return kBlack; }

uint8_t defaultPaletteIndex(const Mac&) { return 0; }

Identity resolveIdentity(const Mac& mac, const IStorage&) { return Identity{mac, 0, false}; }

}  // namespace cubbies
