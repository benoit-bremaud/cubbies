#include "core/Identity.h"

namespace cubbies {

namespace {

// Red, orange, yellow, green, cyan, blue, violet, pink (05b, provisional).
constexpr Rgb565 kPalette[kPaletteSize] = {
    rgb565(255, 40, 40),  rgb565(255, 140, 0), rgb565(255, 230, 0),  rgb565(40, 220, 60),
    rgb565(0, 220, 220),  rgb565(40, 90, 255), rgb565(160, 60, 255), rgb565(255, 90, 180),
};

constexpr uint32_t kFnvOffset = 0x811C9DC5u;
constexpr uint32_t kFnvPrime = 0x01000193u;

}  // namespace

Rgb565 paletteColour(uint8_t index) { return kPalette[index % kPaletteSize]; }

uint8_t defaultPaletteIndex(const Mac& mac) {
  uint32_t h = kFnvOffset;
  for (uint8_t b : mac) h = (h ^ b) * kFnvPrime;
  return static_cast<uint8_t>(h % kPaletteSize);
}

Identity resolveIdentity(const Mac& mac, const IStorage& storage) {
  uint8_t stored = 0;
  if (storage.readU8(kColourKey, stored) && stored < kPaletteSize) return Identity{mac, stored, true};
  return Identity{mac, defaultPaletteIndex(mac), false};
}

}  // namespace cubbies
