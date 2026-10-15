// Identity rules: the resolution branches of 02a, the MAC hash and the palette (05b).
#include <unity.h>

#include "FakeStorage.h"
#include "core/Identity.h"

using cubbies::defaultPaletteIndex;
using cubbies::kColourKey;
using cubbies::kPaletteSize;
using cubbies::Mac;
using cubbies::paletteColour;
using cubbies::resolveIdentity;
using cubbies::test::FakeStorage;

namespace {
// Placeholder MACs, not real devices. Expected indices are FNV-1a 32-bit mod 8, computed by hand.
constexpr Mac kMacA{0x24, 0x0A, 0xC4, 0x12, 0x34, 0x56};  // 0x9c72e737 -> 7
constexpr Mac kMacZero{0, 0, 0, 0, 0, 0};                  // 0x7f77879d -> 5
}  // namespace

void setUp() {}
void tearDown() {}

// The hash is the documented FNV-1a, so the default survives a reflash.
static void test_default_index_is_fnv1a_mod_palette() {
  TEST_ASSERT_EQUAL_UINT8(7, defaultPaletteIndex(kMacA));
  TEST_ASSERT_EQUAL_UINT8(5, defaultPaletteIndex(kMacZero));
}

// Stored value valid: adopted, stored index wins over the MAC default.
static void test_valid_stored_index_is_adopted() {
  FakeStorage s;
  s.values[kColourKey] = 2;
  const auto id = resolveIdentity(kMacA, s);
  TEST_ASSERT_EQUAL_UINT8(2, id.paletteIndex);
  TEST_ASSERT_TRUE(id.adopted);
  TEST_ASSERT_EQUAL_UINT8_ARRAY(kMacA.data(), id.mac.data(), 6);
}

// Not found: MAC default, not adopted, and the default is never written.
static void test_missing_value_falls_back_to_mac_without_writing() {
  FakeStorage s;
  const auto id = resolveIdentity(kMacA, s);
  TEST_ASSERT_EQUAL_UINT8(7, id.paletteIndex);
  TEST_ASSERT_FALSE(id.adopted);
  TEST_ASSERT_EQUAL(0, s.writes);
}

// Out of range after a palette change: MAC default, not adopted.
static void test_out_of_range_value_falls_back_to_mac() {
  FakeStorage s;
  s.values[kColourKey] = kPaletteSize;
  const auto id = resolveIdentity(kMacZero, s);
  TEST_ASSERT_EQUAL_UINT8(5, id.paletteIndex);
  TEST_ASSERT_FALSE(id.adopted);
}

// Eight distinct, non-black colours.
static void test_palette_has_eight_distinct_colours() {
  for (uint8_t i = 0; i < kPaletteSize; ++i) {
    TEST_ASSERT_NOT_EQUAL(0, paletteColour(i).value);
    for (uint8_t j = 0; j < i; ++j) TEST_ASSERT_NOT_EQUAL(paletteColour(j).value, paletteColour(i).value);
  }
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_default_index_is_fnv1a_mod_palette);
  RUN_TEST(test_valid_stored_index_is_adopted);
  RUN_TEST(test_missing_value_falls_back_to_mac_without_writing);
  RUN_TEST(test_out_of_range_value_falls_back_to_mac);
  RUN_TEST(test_palette_has_eight_distinct_colours);
  return UNITY_END();
}
