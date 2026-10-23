// Choir: one test per row of the social-group/05c transition table, on Choir alone, then the start
// over LoopbackRadio through two Social protocols with different tick phases (02c, FS8 drift).
// Rows 11 and 12 (sleep stops the voice, reactions are dropped) are Sound gate tests.
#include <unity.h>

#include "FakeClock.h"
#include "LoopbackRadio.h"
#include "core/Choir.h"
#include "core/PresenceCodec.h"
#include "core/SocialProtocol.h"

using cubbies::Choir;
using cubbies::ChoirCue;
using cubbies::ChoirStart;
using cubbies::decodeChoir;
using cubbies::DecodeResult;
using cubbies::encodeChoir;
using cubbies::encodePresence;
using cubbies::kFrameSize;
using cubbies::Mac;
using cubbies::Mood;
using cubbies::Presence;
using cubbies::Prng;
using cubbies::SelfState;
using cubbies::SleepState;
using cubbies::SocialOutput;
using cubbies::SocialProtocol;
using cubbies::test::Air;
using cubbies::test::FakeClock;
using cubbies::test::LoopbackRadio;

namespace {
constexpr uint32_t kT0 = 1000;
constexpr uint32_t kTick = 40;
const ChoirStart kStart{4, 0, Choir::kLead};
}  // namespace

void setUp() {}
void tearDown() {}

// Row 1: conductor with a present neighbour sends {own palette, melody 0, lead 50} and sings its
// voice after the full lead.
static void test_row1_conductor_sends_and_sings_after_the_lead() {
  Choir c;
  ChoirStart out{};
  TEST_ASSERT_TRUE(c.start(kT0, true, 2, 1, out));
  TEST_ASSERT_EQUAL_UINT8(2, out.paletteIndex);
  TEST_ASSERT_EQUAL_UINT8(0, out.melody);
  TEST_ASSERT_EQUAL_UINT8(50, out.lead);
  const ChoirCue cue = c.tick(kT0, true, 1);
  TEST_ASSERT_TRUE(cue.sing);
  TEST_ASSERT_EQUAL_UINT8(1, cue.voice);
  TEST_ASSERT_EQUAL_UINT32(Choir::kLeadMs, cue.restMs);
  TEST_ASSERT_FALSE(c.tick(kT0 + kTick, true, 1).sing);  // once
}

// Row 1: a cube that is not Awake conducts nothing.
static void test_row1_not_awake_conducts_nothing() {
  Choir c;
  ChoirStart out{};
  TEST_ASSERT_FALSE(c.start(kT0, false, 2, 1, out));
  TEST_ASSERT_FALSE(c.tick(kT0, true, 0).sing);
}

// Row 2: alone, nothing is sent.
static void test_row2_alone_sends_nothing() {
  Choir c;
  ChoirStart out{};
  TEST_ASSERT_FALSE(c.start(kT0, true, 2, 0, out));
  TEST_ASSERT_FALSE(c.tick(kT0, true, 0).sing);
}

// Row 3: a stroke within T_choir of the last start (conducted or sung) sends nothing; at T_choir it
// sends again.
static void test_row3_conductor_cooldown() {
  Choir c;
  ChoirStart out{};
  TEST_ASSERT_TRUE(c.start(kT0, true, 2, 1, out));
  c.tick(kT0, true, 0);
  TEST_ASSERT_FALSE(c.start(kT0 + Choir::kCooldownMs - 1, true, 2, 1, out));
  TEST_ASSERT_TRUE(c.start(kT0 + Choir::kCooldownMs, true, 2, 1, out));

  Choir sung;
  sung.hear(kStart, kT0);
  sung.tick(kT0 + kTick, true, 0);
  TEST_ASSERT_FALSE(sung.start(kT0 + kTick + 1, true, 2, 1, out));
}

// Row 4: a receiver sings after the rest left from its receive time, whatever the tick's phase.
static void test_row4_receiver_rest_is_anchored_on_the_receive_time() {
  Choir c;
  c.hear(kStart, kT0);
  const ChoirCue cue = c.tick(kT0 + 37, true, 0);
  TEST_ASSERT_TRUE(cue.sing);
  TEST_ASSERT_EQUAL_UINT32(Choir::kLeadMs - 37, cue.restMs);
  TEST_ASSERT_FALSE(c.tick(kT0 + 77, true, 0).sing);
}

// Row 4: the lead on the air sets the rest (10 ms units).
static void test_row4_lead_comes_from_the_frame() {
  Choir c;
  c.hear(ChoirStart{4, 0, 7}, kT0);
  TEST_ASSERT_EQUAL_UINT32(70, c.tick(kT0, true, 0).restMs);
}

// Row 5: a cube that is not Awake (Dozing, Asleep, Waking: main.cpp passes awake = false) stays
// silent and its cooldown does not start.
static void test_row5_not_awake_stays_silent() {
  Choir c;
  c.hear(kStart, kT0);
  TEST_ASSERT_FALSE(c.tick(kT0, false, 0).sing);
  c.hear(kStart, kT0 + 1000);
  TEST_ASSERT_TRUE(c.tick(kT0 + 1000, true, 0).sing);
}

// Row 6: a second CHOIR frame while a start is pending is ignored.
static void test_row6_second_frame_while_pending_is_ignored() {
  Choir c;
  c.hear(kStart, kT0);
  c.hear(ChoirStart{5, 0, 10}, kT0 + 5);
  const ChoirCue cue = c.tick(kT0 + 10, true, 0);
  TEST_ASSERT_EQUAL_UINT32(Choir::kLeadMs - 10, cue.restMs);
  TEST_ASSERT_FALSE(c.tick(kT0 + 50, true, 0).sing);
}

// Row 7: a CHOIR frame within T_choir of the last start is ignored; at T_choir it is taken.
static void test_row7_receiver_cooldown() {
  Choir c;
  c.hear(kStart, kT0);
  c.tick(kT0, true, 0);
  c.hear(kStart, kT0 + Choir::kCooldownMs - 1);
  TEST_ASSERT_FALSE(c.tick(kT0 + Choir::kCooldownMs - 1, true, 0).sing);
  c.hear(kStart, kT0 + Choir::kCooldownMs);
  TEST_ASSERT_TRUE(c.tick(kT0 + Choir::kCooldownMs, true, 0).sing);
}

// Row 8: a frame drained at or after its start time is too late: silent, cooldown not started.
static void test_row8_late_frame_is_silent() {
  Choir c;
  c.hear(kStart, kT0);
  TEST_ASSERT_FALSE(c.tick(kT0 + Choir::kLeadMs, true, 0).sing);
  c.hear(kStart, kT0 + 1000);
  TEST_ASSERT_TRUE(c.tick(kT0 + 1000, true, 0).sing);
}

// Rows 4, 7, 8 across the 2^32 ms wrap.
static void test_rows_4_7_8_survive_the_clock_wrap() {
  const uint32_t t = 0xFFFFFF00u;
  Choir c;
  c.hear(kStart, t);
  const ChoirCue cue = c.tick(t + 300, true, 0);  // wraps
  TEST_ASSERT_TRUE(cue.sing);
  TEST_ASSERT_EQUAL_UINT32(Choir::kLeadMs - 300, cue.restMs);
  c.hear(kStart, t + 1000);
  TEST_ASSERT_FALSE(c.tick(t + 1000, true, 0).sing);
}

// Row 10: voice = rank mod 3.
static void test_row10_voice_is_rank_mod_3() {
  for (uint8_t rank = 0; rank < 7; ++rank) {
    Choir c;
    c.hear(kStart, kT0);
    TEST_ASSERT_EQUAL_UINT8(rank % Choir::kVoices, c.tick(kT0, true, rank).voice);
  }
}

// — Over the radio (02c) —

namespace {
Mac mac(uint8_t last) { return Mac{{0x10, 0, 0, 0, 0, last}}; }

struct Cube {
  Cube(Air& air, uint8_t last, FakeClock& clock, uint64_t seed)
      : radio(air, mac(last), clock), prng(seed, seed), social(clock, radio, prng, mac(last)) {}
  LoopbackRadio radio;
  Prng prng;
  SocialProtocol social;
  SelfState self{static_cast<uint8_t>(0), true, SleepState::Awake, SleepState::Awake, Mood::Curious};
};

// Makes `cube` see `from` as present.
void announce(Cube& cube, uint8_t from, FakeClock& clock) {
  uint8_t f[kFrameSize];
  encodePresence(Presence{1, 0, true}, f);
  cube.radio.inject(f, kFrameSize, mac(from));
  cube.social.tick(cube.self);
  (void)clock;
}

size_t choirFrames(const LoopbackRadio& r) {
  size_t n = 0;
  for (const auto& f : r.sent) {
    ChoirStart c{};
    if (decodeChoir(f.data(), f.size(), c) == DecodeResult::Ok) ++n;
  }
  return n;
}
}  // namespace

// Rows 1, 4, 10 end to end: the conductor and a receiver ticking 23 ms out of phase start their
// voices on the same absolute millisecond (FS8 drift ≤ 50 ms, here 0 over a latency-free air).
static void test_protocol_two_cubes_start_on_the_same_ms() {
  Air air;
  FakeClock ca(kT0), cb(kT0 + 23);  // B's clock reads 23 ms more: its tick lands 23 ms "later"
  Cube a(air, 0x20, ca, 1), b(air, 0x10, cb, 2);
  announce(a, 0x10, ca);
  announce(b, 0x20, cb);
  a.radio.sent.clear();

  TEST_ASSERT_TRUE(a.social.startChoir(a.self));
  const SocialOutput oa = a.social.tick(a.self);
  TEST_ASSERT_TRUE(oa.choir.sing);
  TEST_ASSERT_EQUAL_UINT8(1, oa.choir.voice);  // B has a lower MAC
  const uint32_t aStart = ca.nowMs() + oa.choir.restMs;

  cb.advance(17);  // B drains the frame on its next tick
  const SocialOutput ob = b.social.tick(b.self);
  TEST_ASSERT_TRUE(ob.choir.sing);
  TEST_ASSERT_EQUAL_UINT8(0, ob.choir.voice);
  const uint32_t bStart = cb.nowMs() + ob.choir.restMs;
  TEST_ASSERT_EQUAL_UINT32(aStart + 23, bStart);  // same instant: B's clock is 23 ms ahead
}

// Row 2 through the protocol: alone, no frame.
static void test_protocol_alone_sends_nothing() {
  Air air;
  FakeClock ca(kT0);
  Cube a(air, 0x20, ca, 1);
  a.social.tick(a.self);
  a.radio.sent.clear();
  TEST_ASSERT_FALSE(a.social.startChoir(a.self));
  TEST_ASSERT_FALSE(a.social.tick(a.self).choir.sing);
  TEST_ASSERT_EQUAL(0, choirFrames(a.radio));
}

// Row 9: one frame per choir, receivers never transmit CHOIR.
static void test_row9_receivers_never_relay() {
  Air air;
  FakeClock ca(kT0), cb(kT0), cc(kT0);
  Cube a(air, 0x30, ca, 1), b(air, 0x10, cb, 2), c(air, 0x20, cc, 3);
  announce(a, 0x10, ca);
  announce(b, 0x30, cb);
  announce(c, 0x30, cc);
  TEST_ASSERT_TRUE(a.social.startChoir(a.self));
  for (int i = 0; i < 20; ++i) {
    ca.advance(kTick);
    cb.advance(kTick);
    cc.advance(kTick);
    a.social.tick(a.self);
    b.social.tick(b.self);
    c.social.tick(c.self);
  }
  TEST_ASSERT_EQUAL(1, choirFrames(a.radio));
  TEST_ASSERT_EQUAL(0, choirFrames(b.radio));
  TEST_ASSERT_EQUAL(0, choirFrames(c.radio));
}

// Malformed CHOIR frames are dropped by reason (06 allowlist) and never sing.
static void test_protocol_malformed_choir_is_dropped() {
  Air air;
  FakeClock cb(kT0);
  Cube b(air, 0x10, cb, 2);
  uint8_t f[kFrameSize];
  encodeChoir(ChoirStart{1, 1, 50}, f);  // melody 1 is not allowed
  b.radio.inject(f, kFrameSize, mac(0x20));
  TEST_ASSERT_FALSE(b.social.tick(b.self).choir.sing);
  TEST_ASSERT_EQUAL_UINT32(1, b.social.stats().dropRange);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_row1_conductor_sends_and_sings_after_the_lead);
  RUN_TEST(test_row1_not_awake_conducts_nothing);
  RUN_TEST(test_row2_alone_sends_nothing);
  RUN_TEST(test_row3_conductor_cooldown);
  RUN_TEST(test_row4_receiver_rest_is_anchored_on_the_receive_time);
  RUN_TEST(test_row4_lead_comes_from_the_frame);
  RUN_TEST(test_row5_not_awake_stays_silent);
  RUN_TEST(test_row6_second_frame_while_pending_is_ignored);
  RUN_TEST(test_row7_receiver_cooldown);
  RUN_TEST(test_row8_late_frame_is_silent);
  RUN_TEST(test_rows_4_7_8_survive_the_clock_wrap);
  RUN_TEST(test_row10_voice_is_rank_mod_3);
  RUN_TEST(test_protocol_two_cubes_start_on_the_same_ms);
  RUN_TEST(test_protocol_alone_sends_nothing);
  RUN_TEST(test_row9_receivers_never_relay);
  RUN_TEST(test_protocol_malformed_choir_is_dropped);
  return UNITY_END();
}
