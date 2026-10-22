// Chain reaction: one test per row of the social-group/05b transition table, on ChainReaction
// alone, then the wave over LoopbackRadio through three Social protocols (02b) and the echo bound.
#include <unity.h>

#include <initializer_list>

#include "FakeClock.h"
#include "LoopbackRadio.h"
#include "core/ChainReaction.h"
#include "core/PresenceCodec.h"
#include "core/SocialProtocol.h"

using cubbies::Chain;
using cubbies::ChainFire;
using cubbies::ChainReaction;
using cubbies::decodeChain;
using cubbies::DecodeResult;
using cubbies::encodePresence;
using cubbies::kFrameSize;
using cubbies::kMaxHop;
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
using C = ChainReaction;
constexpr uint32_t kT0 = 1000;
const Chain kEvent{4, 77, 0};
Prng gPrng(5, 5);

// Hears `c` at kT0 with `rank`, then fires at the exact deadline.
ChainFire hearAndFire(C& chain, const Chain& c, uint8_t rank, bool awake = true) {
  chain.hear(c, kT0, rank);
  return chain.tick(kT0 + C::kStepMs * (1 + rank), awake);
}
}  // namespace

void setUp() {}
void tearDown() {}

// Row 1: origin with a present neighbour sends {own palette, id, hop 0}.
static void test_row1_origin_sends_hop0_with_a_neighbour() {
  C chain;
  Chain out{};
  Prng expected(5, 5), used(5, 5);
  TEST_ASSERT_TRUE(chain.start(kT0, true, 2, 1, used, out));
  TEST_ASSERT_EQUAL_UINT8(2, out.paletteIndex);
  TEST_ASSERT_EQUAL_UINT8(expected.uniform(256), out.eventId);
  TEST_ASSERT_EQUAL_UINT8(0, out.hop);
  // The origin's Joy is its chain reaction: a foreign event within T_cooldown is not taken.
  chain.hear(Chain{5, 1, 0}, kT0 + 1000, 0);
  TEST_ASSERT_FALSE(chain.tick(kT0 + 5000, true).react);
}

// Row 1: a cube that is not Awake starts nothing, and a refused trigger draws no Prng value.
static void test_row1_not_awake_starts_nothing_and_keeps_the_prng() {
  C chain;
  Chain out{};
  Prng used(5, 5), untouched(5, 5);
  TEST_ASSERT_FALSE(chain.start(kT0, false, 2, 1, used, out));
  TEST_ASSERT_FALSE(chain.start(kT0, true, 2, 0, used, out));
  TEST_ASSERT_EQUAL_UINT32(untouched.next(), used.next());
}

// Row 2: alone, nothing is sent.
static void test_row2_alone_sends_nothing() {
  C chain;
  Chain out{};
  TEST_ASSERT_FALSE(chain.start(kT0, true, 2, 0, gPrng, out));
}

// Row 3: a second trigger within T_trigger sends nothing; at T_trigger it sends again.
static void test_row3_trigger_cooldown() {
  C chain;
  Chain out{};
  TEST_ASSERT_TRUE(chain.start(kT0, true, 2, 1, gPrng, out));
  TEST_ASSERT_FALSE(chain.start(kT0 + C::kTriggerMs - 1, true, 2, 1, gPrng, out));
  TEST_ASSERT_TRUE(chain.start(kT0 + C::kTriggerMs, true, 2, 1, gPrng, out));
}

// Row 4: an unseen key fires at T_step × (1 + rank), not one ms earlier.
static void test_row4_delay_follows_the_rank() {
  for (uint8_t rank = 0; rank < 16; ++rank) {
    C chain;
    chain.hear(kEvent, kT0, rank);
    const uint32_t due = kT0 + C::kStepMs * (1 + rank);
    TEST_ASSERT_FALSE(chain.tick(due - 1, true).react);
    TEST_ASSERT_TRUE(chain.tick(due, true).react);
  }
}

// Row 5: an Awake cube reacts once; nothing more on later ticks.
static void test_row5_awake_reacts_once() {
  C chain;
  TEST_ASSERT_TRUE(hearAndFire(chain, kEvent, 0).react);
  const ChainFire again = chain.tick(kT0 + 10000, true);
  TEST_ASSERT_FALSE(again.react);
  TEST_ASSERT_FALSE(again.relay);
}

// Row 6: below kMaxHop it relays the same key with hop + 1.
static void test_row6_relays_with_hop_plus_one() {
  C chain;
  const ChainFire f = hearAndFire(chain, Chain{4, 77, 1}, 0);
  TEST_ASSERT_TRUE(f.relay);
  TEST_ASSERT_EQUAL_UINT8(4, f.frame.paletteIndex);
  TEST_ASSERT_EQUAL_UINT8(77, f.frame.eventId);
  TEST_ASSERT_EQUAL_UINT8(2, f.frame.hop);
}

// Row 7: at kMaxHop it reacts but does not relay.
static void test_row7_last_hop_does_not_relay() {
  C chain;
  const ChainFire f = hearAndFire(chain, Chain{4, 77, kMaxHop}, 0);
  TEST_ASSERT_TRUE(f.react);
  TEST_ASSERT_FALSE(f.relay);
}

// Row 8: the same key heard again, scheduled or done, is ignored.
static void test_row8_same_key_is_ignored() {
  C chain;
  chain.hear(kEvent, kT0, 0);
  chain.hear(Chain{4, 77, 1}, kT0 + 10, 0);  // while scheduled
  TEST_ASSERT_TRUE(chain.tick(kT0 + C::kStepMs, true).react);
  chain.hear(Chain{4, 77, 2}, kT0 + C::kCooldownMs + 1, 0);  // after firing, past the cooldown
  const ChainFire f = chain.tick(kT0 + 20000, true);
  TEST_ASSERT_FALSE(f.react);
  TEST_ASSERT_FALSE(f.relay);
}

// Row 9: the origin ignores its own event coming back.
static void test_row9_origin_ignores_its_own_event() {
  C chain;
  Chain out{};
  TEST_ASSERT_TRUE(chain.start(kT0, true, 2, 1, gPrng, out));
  chain.hear(Chain{2, out.eventId, 1}, kT0 + C::kCooldownMs + 300, 0);
  const ChainFire f = chain.tick(kT0 + 20000, true);
  TEST_ASSERT_FALSE(f.react);
  TEST_ASSERT_FALSE(f.relay);
}

// Row 10: a new key while one is scheduled is dropped and not remembered.
static void test_row10_one_pending_event_at_a_time() {
  C chain;
  chain.hear(kEvent, kT0, 0);
  chain.hear(Chain{5, 1, 0}, kT0 + 10, 0);
  const ChainFire f = chain.tick(kT0 + C::kStepMs, true);
  TEST_ASSERT_EQUAL_UINT8(77, f.frame.eventId);
  // Not remembered: heard again past the cooldown, it is taken.
  const uint32_t later = kT0 + C::kStepMs + C::kCooldownMs;
  chain.hear(Chain{5, 1, 1}, later, 0);
  TEST_ASSERT_TRUE(chain.tick(later + C::kStepMs, true).react);
}

// Row 11: not Awake → no reaction, but the relay still goes out.
static void test_row11_sleeping_cube_relays_without_reacting() {
  C chain;
  const ChainFire f = hearAndFire(chain, kEvent, 0, false);
  TEST_ASSERT_FALSE(f.react);
  TEST_ASSERT_TRUE(f.relay);
  TEST_ASSERT_EQUAL_UINT8(1, f.frame.hop);
}

// Row 12: a new event within T_cooldown of the last reaction is remembered, neither reacted to
// nor relayed.
static void test_row12_reaction_cooldown() {
  C chain;
  TEST_ASSERT_TRUE(hearAndFire(chain, kEvent, 0).react);
  const uint32_t fired = kT0 + C::kStepMs;
  chain.hear(Chain{5, 1, 0}, fired + C::kCooldownMs - 1, 0);
  const ChainFire f = chain.tick(fired + 10000, true);
  TEST_ASSERT_FALSE(f.react);
  TEST_ASSERT_FALSE(f.relay);
  // Remembered: the same key later is still ignored.
  chain.hear(Chain{5, 1, 1}, fired + C::kCooldownMs + 100, 0);
  TEST_ASSERT_FALSE(chain.tick(fired + 20000, true).react);
}

// Row 13: past 8 keys the oldest is forgotten (keys heard during the cooldown are remembered,
// so the ring fills well within T_seen).
static void test_row13_ring_overwrites_the_oldest() {
  C chain;
  TEST_ASSERT_TRUE(hearAndFire(chain, kEvent, 0).react);
  const uint32_t fired = kT0 + C::kStepMs;
  for (uint8_t id = 0; id < C::kSeenKeys; ++id) chain.hear(Chain{1, id, 0}, fired + 100, 0);
  const uint32_t after = fired + C::kCooldownMs;
  chain.hear(Chain{1, 0, 1}, after, 0);  // the oldest of the 8 kept keys: still remembered
  TEST_ASSERT_FALSE(chain.tick(after + C::kStepMs, true).react);
  chain.hear(Chain{4, 77, 1}, after, 0);  // overwritten by the 8 newer keys: new again
  TEST_ASSERT_TRUE(chain.tick(after + C::kStepMs, true).react);
  const uint32_t later = after + C::kStepMs + C::kCooldownMs;
  chain.hear(Chain{1, 7, 1}, later, 0);  // the newest is still remembered
  TEST_ASSERT_FALSE(chain.tick(later + C::kStepMs, true).react);
}

// Row 14: a key older than T_seen is new again, across the 2^32 ms wrap too.
static void test_row14_key_expires_after_t_seen_across_the_wrap() {
  C chain;
  const uint32_t start = 0xFFFFFF00u;
  chain.hear(kEvent, start, 0);
  TEST_ASSERT_TRUE(chain.tick(start + C::kStepMs, true).react);
  chain.hear(kEvent, start + C::kSeenMs - 1, 0);
  TEST_ASSERT_FALSE(chain.tick(start + C::kSeenMs + 1000, true).react);
  const uint32_t later = start + C::kSeenMs;
  chain.hear(kEvent, later, 0);
  TEST_ASSERT_TRUE(chain.tick(later + C::kStepMs, true).react);
}

// ---- Through the Social protocol (02b) ----

namespace {
constexpr uint32_t kTick = 40;
Mac macOf(uint8_t n) { return Mac{0x24, 0x6F, 0x28, 0x00, 0x00, n}; }

struct Cube {
  Cube(Air& air, FakeClock& clock, uint8_t n, uint8_t palette)
      : radio(air, macOf(n), clock), prng(n, 3), social(clock, radio, prng, macOf(n)),
        self{palette, true, SleepState::Awake, SleepState::Dozing, Mood::Curious} {}
  LoopbackRadio radio;
  Prng prng;
  SocialProtocol social;
  SelfState self;
  uint32_t reactedAt = 0;
  uint32_t reactions = 0;
};

// Three cubes that have heard each other's presence.
struct Fleet {
  Fleet() : clock(kT0), a(air, clock, 0x10, 0), b(air, clock, 0x20, 1), c(air, clock, 0x30, 2) {
    for (int i = 0; i < 3; ++i) step();
  }
  Air air;
  FakeClock clock;
  Cube a, b, c;
  void step() {
    clock.advance(kTick);
    for (Cube* k : {&a, &b, &c}) {
      const SocialOutput o = k->social.tick(k->self);
      if (o.chain) {
        ++k->reactions;
        k->reactedAt = clock.nowMs();
      }
    }
  }
  // CHAIN frames sent by one cube; `lastHop` gets the hop of the last one.
  static size_t chainFramesSent(const Cube& k, uint8_t* lastHop = nullptr) {
    size_t n = 0;
    for (const auto& f : k.radio.sent) {
      Chain ch{};
      if (decodeChain(f.data(), f.size(), ch) != DecodeResult::Ok) continue;
      ++n;
      if (lastHop != nullptr) *lastHop = ch.hop;
    }
    return n;
  }
};
}  // namespace

// Row 2 through the protocol: alone, startChain sends nothing.
static void test_protocol_alone_start_sends_nothing() {
  Air air;
  FakeClock clock(kT0);
  Cube a(air, clock, 0x10, 0);
  a.social.tick(a.self);
  const size_t before = a.radio.sent.size();
  TEST_ASSERT_FALSE(a.social.startChain(a.self));
  TEST_ASSERT_EQUAL(before, a.radio.sent.size());
}

// Rows 1, 4–6, 8–9: A double-taps, B (lower MAC of the two) reacts first, C one step later,
// each once, hop ≤ 500 ms (FS8); the origin never reacts to its own event.
static void test_protocol_wave_in_mac_order_each_once() {
  Fleet f;
  const uint32_t tapAt = f.clock.nowMs();
  TEST_ASSERT_TRUE(f.a.social.startChain(f.a.self));
  for (int i = 0; i < 200; ++i) f.step();  // 8 s
  TEST_ASSERT_EQUAL_UINT32(0, f.a.reactions);
  TEST_ASSERT_EQUAL_UINT32(1, f.b.reactions);
  TEST_ASSERT_EQUAL_UINT32(1, f.c.reactions);
  TEST_ASSERT_TRUE(f.b.reactedAt < f.c.reactedAt);
  TEST_ASSERT_UINT32_WITHIN(2 * kTick, ChainReaction::kStepMs * 2, f.b.reactedAt - tapAt);
  TEST_ASSERT_UINT32_WITHIN(2 * kTick, ChainReaction::kStepMs, f.c.reactedAt - f.b.reactedAt);
}

// No echo storm: one event costs at most one CHAIN frame per cube.
static void test_protocol_one_frame_per_cube_per_event() {
  Fleet f;
  TEST_ASSERT_TRUE(f.a.social.startChain(f.a.self));
  for (int i = 0; i < 1000; ++i) f.step();  // 40 s, past T_seen
  TEST_ASSERT_EQUAL(1, Fleet::chainFramesSent(f.a));
  TEST_ASSERT_EQUAL(1, Fleet::chainFramesSent(f.b));
  TEST_ASSERT_EQUAL(1, Fleet::chainFramesSent(f.c));
}

// A sleeping neighbour relays but does not report a reaction (row 11).
static void test_protocol_sleeping_cube_does_not_report() {
  for (SleepState st : {SleepState::Dozing, SleepState::Asleep, SleepState::Waking}) {
    Fleet f;
    f.b.self.sleep = st;
    TEST_ASSERT_TRUE(f.a.social.startChain(f.a.self));
    for (int i = 0; i < 200; ++i) f.step();
    TEST_ASSERT_EQUAL_UINT32(0, f.b.reactions);
    TEST_ASSERT_EQUAL_UINT32(1, f.c.reactions);
    uint8_t hop = 0xFF;
    TEST_ASSERT_EQUAL(1, Fleet::chainFramesSent(f.b, &hop));  // B relayed
    TEST_ASSERT_EQUAL_UINT8(1, hop);
  }
}

// Malformed CHAIN frames are dropped and counted, never scheduled.
static void test_protocol_malformed_chain_is_dropped() {
  Air air;
  FakeClock clock(kT0);
  Cube a(air, clock, 0x10, 0);
  uint8_t bad[kFrameSize] = {0x43, 0x42, 1, 0x02, 1, 7, kMaxHop + 1, 0};
  a.radio.inject(bad, kFrameSize, macOf(0x20));
  a.social.tick(a.self);
  TEST_ASSERT_EQUAL_UINT32(1, a.social.stats().dropRange);
  for (int i = 0; i < 100; ++i) {
    clock.advance(kTick);
    TEST_ASSERT_FALSE(a.social.tick(a.self).chain);
  }
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_row1_origin_sends_hop0_with_a_neighbour);
  RUN_TEST(test_row1_not_awake_starts_nothing_and_keeps_the_prng);
  RUN_TEST(test_row2_alone_sends_nothing);
  RUN_TEST(test_row3_trigger_cooldown);
  RUN_TEST(test_row4_delay_follows_the_rank);
  RUN_TEST(test_row5_awake_reacts_once);
  RUN_TEST(test_row6_relays_with_hop_plus_one);
  RUN_TEST(test_row7_last_hop_does_not_relay);
  RUN_TEST(test_row8_same_key_is_ignored);
  RUN_TEST(test_row9_origin_ignores_its_own_event);
  RUN_TEST(test_row10_one_pending_event_at_a_time);
  RUN_TEST(test_row11_sleeping_cube_relays_without_reacting);
  RUN_TEST(test_row12_reaction_cooldown);
  RUN_TEST(test_row13_ring_overwrites_the_oldest);
  RUN_TEST(test_row14_key_expires_after_t_seen_across_the_wrap);
  RUN_TEST(test_protocol_alone_start_sends_nothing);
  RUN_TEST(test_protocol_wave_in_mac_order_each_once);
  RUN_TEST(test_protocol_one_frame_per_cube_per_event);
  RUN_TEST(test_protocol_sleeping_cube_does_not_report);
  RUN_TEST(test_protocol_malformed_chain_is_dropped);
  return UNITY_END();
}
