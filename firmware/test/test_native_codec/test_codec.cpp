// Frame codec: the 06 PRESENCE and CHAIN layouts, the allowlist (one test per drop reason), the 05b mood
// byte table, and a fuzz pass (REQ-NF-06: no crash, only valid frames accepted).
#include <unity.h>

#include "core/Identity.h"
#include "core/Prng.h"
#include "core/PresenceCodec.h"

using cubbies::Chain;
using cubbies::ChoirStart;
using cubbies::decodeChoir;
using cubbies::encodeChoir;
using cubbies::kMaxChoirLead;
using cubbies::decodeChain;
using cubbies::decodePresence;
using cubbies::encodeChain;
using cubbies::kMaxHop;
using cubbies::DecodeResult;
using cubbies::encodePresence;
using cubbies::kFrameSize;
using cubbies::Mood;
using cubbies::moodByte;
using cubbies::Presence;
using cubbies::Prng;
using cubbies::SleepState;

namespace {
uint8_t valid[kFrameSize] = {0x43, 0x42, 1, 0x01, 3, 2, 0x01, 0};
}

void setUp() {}
void tearDown() {}

static void test_encode_matches_the_06_layout() {
  uint8_t out[kFrameSize];
  encodePresence(Presence{3, 2, true}, out);
  TEST_ASSERT_EQUAL_HEX8_ARRAY(valid, out, kFrameSize);
  encodePresence(Presence{7, 5, false}, out);
  const uint8_t expected[kFrameSize] = {0x43, 0x42, 1, 0x01, 7, 5, 0x00, 0};
  TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, out, kFrameSize);
}

static void test_decode_round_trips_every_valid_field() {
  for (uint8_t pal = 0; pal < cubbies::kPaletteSize; ++pal)
    for (uint8_t mood = 0; mood <= cubbies::kMoodAsleep; ++mood)
      for (int adopted = 0; adopted < 2; ++adopted) {
        uint8_t buf[kFrameSize];
        encodePresence(Presence{pal, mood, adopted == 1}, buf);
        Presence p{};
        TEST_ASSERT_EQUAL(DecodeResult::Ok, decodePresence(buf, kFrameSize, p));
        TEST_ASSERT_EQUAL(pal, p.paletteIndex);
        TEST_ASSERT_EQUAL(mood, p.mood);
        TEST_ASSERT_EQUAL(adopted == 1, p.adopted);
      }
}

static DecodeResult decodeWith(size_t offset, uint8_t value) {
  uint8_t buf[kFrameSize];
  for (size_t i = 0; i < kFrameSize; ++i) buf[i] = valid[i];
  buf[offset] = value;
  Presence p{};
  return decodePresence(buf, kFrameSize, p);
}

static void test_wrong_length_is_dropped() {
  Presence p{};
  TEST_ASSERT_EQUAL(DecodeResult::Len, decodePresence(valid, kFrameSize - 1, p));
  TEST_ASSERT_EQUAL(DecodeResult::Len, decodePresence(valid, kFrameSize + 1, p));
  TEST_ASSERT_EQUAL(DecodeResult::Len, decodePresence(valid, 0, p));
  TEST_ASSERT_EQUAL(DecodeResult::Len, decodePresence(nullptr, kFrameSize, p));
}

static void test_wrong_magic_is_dropped() {
  TEST_ASSERT_EQUAL(DecodeResult::Magic, decodeWith(0, 0x44));
  TEST_ASSERT_EQUAL(DecodeResult::Magic, decodeWith(1, 0x00));
}

static void test_other_versions_are_dropped() {
  TEST_ASSERT_EQUAL(DecodeResult::Version, decodeWith(2, 0));
  TEST_ASSERT_EQUAL(DecodeResult::Version, decodeWith(2, 2));
  TEST_ASSERT_EQUAL(DecodeResult::Version, decodeWith(2, 0xFF));
}

static void test_reserved_types_are_dropped() {
  TEST_ASSERT_EQUAL(DecodeResult::Type, decodeWith(3, 0x00));
  TEST_ASSERT_EQUAL(DecodeResult::Type, decodeWith(3, 0x02));
  TEST_ASSERT_EQUAL(DecodeResult::Type, decodeWith(3, 0x03));
}

static void test_out_of_range_fields_are_dropped() {
  TEST_ASSERT_EQUAL(DecodeResult::Range, decodeWith(4, cubbies::kPaletteSize));
  TEST_ASSERT_EQUAL(DecodeResult::Range, decodeWith(5, 6));
  TEST_ASSERT_EQUAL(DecodeResult::Range, decodeWith(6, 0x02));
  TEST_ASSERT_EQUAL(DecodeResult::Range, decodeWith(6, 0x81));
  TEST_ASSERT_EQUAL(DecodeResult::Range, decodeWith(7, 1));
}

// 05b table: Awake → mood, Dozing → 3, Waking → 3 from Dozing / 5 from Asleep, Asleep → 5.
static void test_mood_byte_follows_the_05b_table() {
  for (uint8_t m = 0; m < cubbies::kMoodCount; ++m)
    TEST_ASSERT_EQUAL(m, moodByte(SleepState::Awake, SleepState::Dozing, static_cast<Mood>(m)));
  TEST_ASSERT_EQUAL(3, moodByte(SleepState::Dozing, SleepState::Dozing, Mood::Happy));
  TEST_ASSERT_EQUAL(3, moodByte(SleepState::Waking, SleepState::Dozing, Mood::Happy));
  TEST_ASSERT_EQUAL(5, moodByte(SleepState::Waking, SleepState::Asleep, Mood::Happy));
  TEST_ASSERT_EQUAL(5, moodByte(SleepState::Asleep, SleepState::Dozing, Mood::Curious));
}

// Fuzz: random lengths and bytes never crash, and every accepted frame re-encodes to itself.
static void test_fuzzed_frames_are_never_misread() {
  Prng prng(0xC0FFEE, 4);
  uint32_t accepted = 0;
  for (int i = 0; i < 200000; ++i) {
    uint8_t buf[16];
    const size_t len = (i % 4 == 0) ? kFrameSize : prng.uniform(17);
    for (size_t b = 0; b < sizeof(buf); ++b) buf[b] = static_cast<uint8_t>(prng.next());
    if (i % 2 == 0) {  // bias half the inputs towards a valid header and small fields
      buf[0] = 0x43;
      buf[1] = 0x42;
      buf[2] = 1;
      buf[3] = 1;
      buf[4] &= 0x0F;
      buf[5] &= 0x07;
      buf[6] &= 0x03;
      buf[7] &= 0x01;
    }
    Presence p{};
    if (decodePresence(buf, len, p) != DecodeResult::Ok) continue;
    ++accepted;
    uint8_t again[kFrameSize];
    encodePresence(p, again);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(buf, again, kFrameSize);
  }
  TEST_ASSERT_TRUE(accepted > 0);
}

// CHAIN (06, G3): layout, round trip, allowlist.
static void test_chain_encode_matches_the_06_layout() {
  uint8_t out[kFrameSize];
  encodeChain(Chain{5, 0xAB, 2}, out);
  const uint8_t expected[kFrameSize] = {0x43, 0x42, 1, 0x02, 5, 0xAB, 2, 0};
  TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, out, kFrameSize);
}

static void test_chain_round_trips_every_valid_field() {
  for (uint8_t pal = 0; pal < cubbies::kPaletteSize; ++pal)
    for (int id = 0; id < 256; ++id)
      for (uint8_t hop = 0; hop <= kMaxHop; ++hop) {
        uint8_t buf[kFrameSize];
        encodeChain(Chain{pal, static_cast<uint8_t>(id), hop}, buf);
        Chain c{};
        TEST_ASSERT_EQUAL(DecodeResult::Ok, decodeChain(buf, kFrameSize, c));
        TEST_ASSERT_EQUAL_UINT8(pal, c.paletteIndex);
        TEST_ASSERT_EQUAL_UINT8(id, c.eventId);
        TEST_ASSERT_EQUAL_UINT8(hop, c.hop);
      }
}

static DecodeResult decodeChainWith(size_t offset, uint8_t value) {
  uint8_t buf[kFrameSize] = {0x43, 0x42, 1, 0x02, 1, 7, 0, 0};
  buf[offset] = value;
  Chain c{};
  return decodeChain(buf, kFrameSize, c);
}

static void test_chain_allowlist_drops_by_reason() {
  Chain c{};
  const uint8_t ok[kFrameSize] = {0x43, 0x42, 1, 0x02, 1, 7, 0, 0};
  TEST_ASSERT_EQUAL(DecodeResult::Len, decodeChain(ok, kFrameSize - 1, c));
  TEST_ASSERT_EQUAL(DecodeResult::Len, decodeChain(nullptr, kFrameSize, c));
  TEST_ASSERT_EQUAL(DecodeResult::Magic, decodeChainWith(0, 0x44));
  TEST_ASSERT_EQUAL(DecodeResult::Version, decodeChainWith(2, 2));
  TEST_ASSERT_EQUAL(DecodeResult::Type, decodeChainWith(3, 0x01));
  TEST_ASSERT_EQUAL(DecodeResult::Range, decodeChainWith(4, cubbies::kPaletteSize));
  TEST_ASSERT_EQUAL(DecodeResult::Range, decodeChainWith(6, kMaxHop + 1));
  TEST_ASSERT_EQUAL(DecodeResult::Range, decodeChainWith(7, 1));
}

// CHOIR (06, G4): layout, round trip, allowlist.
static void test_choir_encode_matches_the_06_layout() {
  uint8_t out[kFrameSize];
  encodeChoir(ChoirStart{5, 0, 50}, out);
  const uint8_t expected[kFrameSize] = {0x43, 0x42, 1, 0x03, 5, 0, 50, 0};
  TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, out, kFrameSize);
}

static void test_choir_round_trips_every_valid_field() {
  for (uint8_t pal = 0; pal < cubbies::kPaletteSize; ++pal)
    for (uint8_t lead = 1; lead <= kMaxChoirLead; ++lead) {
      uint8_t buf[kFrameSize];
      encodeChoir(ChoirStart{pal, 0, lead}, buf);
      ChoirStart c{};
      TEST_ASSERT_EQUAL(DecodeResult::Ok, decodeChoir(buf, kFrameSize, c));
      TEST_ASSERT_EQUAL_UINT8(pal, c.paletteIndex);
      TEST_ASSERT_EQUAL_UINT8(0, c.melody);
      TEST_ASSERT_EQUAL_UINT8(lead, c.lead);
    }
}

static DecodeResult decodeChoirWith(size_t offset, uint8_t value) {
  uint8_t buf[kFrameSize] = {0x43, 0x42, 1, 0x03, 1, 0, 50, 0};
  buf[offset] = value;
  ChoirStart c{};
  return decodeChoir(buf, kFrameSize, c);
}

static void test_choir_allowlist_drops_by_reason() {
  ChoirStart c{};
  const uint8_t ok[kFrameSize] = {0x43, 0x42, 1, 0x03, 1, 0, 50, 0};
  TEST_ASSERT_EQUAL(DecodeResult::Ok, decodeChoir(ok, kFrameSize, c));
  TEST_ASSERT_EQUAL(DecodeResult::Len, decodeChoir(ok, kFrameSize + 1, c));
  TEST_ASSERT_EQUAL(DecodeResult::Len, decodeChoir(nullptr, kFrameSize, c));
  TEST_ASSERT_EQUAL(DecodeResult::Magic, decodeChoirWith(1, 0x00));
  TEST_ASSERT_EQUAL(DecodeResult::Version, decodeChoirWith(2, 2));
  TEST_ASSERT_EQUAL(DecodeResult::Type, decodeChoirWith(3, 0x02));
  TEST_ASSERT_EQUAL(DecodeResult::Range, decodeChoirWith(4, cubbies::kPaletteSize));
  TEST_ASSERT_EQUAL(DecodeResult::Range, decodeChoirWith(5, 1));
  TEST_ASSERT_EQUAL(DecodeResult::Range, decodeChoirWith(6, 0));
  TEST_ASSERT_EQUAL(DecodeResult::Range, decodeChoirWith(6, kMaxChoirLead + 1));
  TEST_ASSERT_EQUAL(DecodeResult::Range, decodeChoirWith(7, 1));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_encode_matches_the_06_layout);
  RUN_TEST(test_decode_round_trips_every_valid_field);
  RUN_TEST(test_wrong_length_is_dropped);
  RUN_TEST(test_wrong_magic_is_dropped);
  RUN_TEST(test_other_versions_are_dropped);
  RUN_TEST(test_reserved_types_are_dropped);
  RUN_TEST(test_out_of_range_fields_are_dropped);
  RUN_TEST(test_mood_byte_follows_the_05b_table);
  RUN_TEST(test_fuzzed_frames_are_never_misread);
  RUN_TEST(test_chain_encode_matches_the_06_layout);
  RUN_TEST(test_chain_round_trips_every_valid_field);
  RUN_TEST(test_chain_allowlist_drops_by_reason);
  RUN_TEST(test_choir_encode_matches_the_06_layout);
  RUN_TEST(test_choir_round_trips_every_valid_field);
  RUN_TEST(test_choir_allowlist_drops_by_reason);
  return UNITY_END();
}
