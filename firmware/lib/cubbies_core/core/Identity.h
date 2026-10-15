// Identity rules (02a, 05b): the palette, the MAC-derived default colour and the boot-time
// resolution against the stored adoption. Deterministic: no clock, no randomness.
#pragma once

#include <array>
#include <cstdint>

#include "eyes/Framebuffer.h"
#include "ports/IStorage.h"
#include "ports/Mac.h"

namespace cubbies {

constexpr uint8_t kPaletteSize = 8;
constexpr const char* kColourKey = "identity.colour";

struct Identity {
  Mac mac;
  uint8_t paletteIndex;
  bool adopted;  // true only when the colour was read from storage
};

// Provisional OLED colours (05b), tuned on the bench. `index` must be < kPaletteSize.
Rgb565 paletteColour(uint8_t index);

// FNV-1a 32-bit over the six MAC bytes, mod kPaletteSize: stable across reflashes (02a).
uint8_t defaultPaletteIndex(const Mac& mac);

// Stored index when valid, else the MAC default (never written back, 02a).
Identity resolveIdentity(const Mac& mac, const IStorage& storage);

}  // namespace cubbies
