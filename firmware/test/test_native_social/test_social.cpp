// Social protocol: one test per row of the social-presence/05a transition table, the broadcast
// period and jitter, the 4-frames-per-tick budget under a flood, the wrap, and two cubes
// greeting each other over LoopbackRadio (02).
#include <unity.h>

#include "FakeClock.h"
#include "LoopbackRadio.h"
#include "core/PresenceCodec.h"
#include "core/SocialProtocol.h"

using cubbies::EntryState;
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
constexpr uint32_t kTick = 40;
using S = SocialProtocol;

const SelfState kSelf{2, true, SleepState::Awake, SleepState::Dozing, Mood::Happy};

Mac macOf(uint8_t n) { return Mac{0x24, 0x6F, 0x28, 0x00, 0x00, n}; }

struct Rig {
  explicit Rig(uint32_t startMs = 1000)
      : clock(startMs), radio(air, macOf(0xAA), clock), prng(42, 7), social(clock, radio, prng) {}
  Air air;
  FakeClock clock;
  LoopbackRadio radio;
  Prng prng;
  SocialProtocol social;
  uint32_t greetings = 0;
  uint8_t lastPalette = 0xFF;

  void hear(uint8_t n, uint8_t palette = 1, uint8_t mood = 0) {
    uint8_t buf[kFrameSize];
    cubbies::encodePresence(Presence{palette, mood, false}, buf);
    radio.inject(buf, kFrameSize, macOf(n));
  }
  SocialOutput step() {
    clock.advance(kTick);
    const SocialOutput o = social.tick(kSelf);
    if (o.newcomer) {
      ++greetings;
      lastPalette = o.paletteIndex;
    }
    return o;
  }
  void run(uint32_t ms) {
    for (uint32_t t = 0; t < ms; t += kTick) step();
  }
  // Keeps neighbour n present (one frame every T_presence) for `ms`.
  void runHearing(uint8_t n, uint32_t ms) {
    for (uint32_t t = 0; t < ms; t += S::kPresenceMs) {
      hear(n);
      run(S::kPresenceMs);
    }
  }
  int slotOf(uint8_t n) const {
    for (uint8_t i = 0; i < S::kCapacity; ++i)
      if (social.neighbour(i).state != EntryState::Free && social.neighbour(i).mac == macOf(n)) return i;
    return -1;
  }
  EntryState stateOf(uint8_t n) const {
    const int s = slotOf(n);
    return s < 0 ? EntryState::Free : social.neighbour(static_cast<uint8_t>(s)).state;
  }
};
}  // namespace

void setUp() {}
void tearDown() {}

// Free -> Waiting: a valid PRESENCE from an unknown MAC is stored (greeted on the same tick
// since no cooldown is running, so the state is checked through the stored fields).
static void test_unknown_mac_is_stored() {
  Rig r;
  const uint32_t heardAt = r.clock.nowMs();
  r.hear(1, 5, 4);
  r.step();
  const int s = r.slotOf(1);
  TEST_ASSERT_TRUE(s >= 0);
  TEST_ASSERT_EQUAL(5, r.social.neighbour(s).paletteIndex);
  TEST_ASSERT_EQUAL(4, r.social.neighbour(s).mood);
  TEST_ASSERT_EQUAL(heardAt, r.social.neighbour(s).lastSeen);
}

// Waiting -> Greeted: the first newcomer is greeted at once with its palette index.
static void test_first_newcomer_is_greeted() {
  Rig r;
  r.hear(1, 6);
  const SocialOutput o = r.step();
  TEST_ASSERT_TRUE(o.newcomer);
  TEST_ASSERT_EQUAL(6, o.paletteIndex);
  TEST_ASSERT_EQUAL(EntryState::Greeted, r.stateOf(1));
}

// Waiting -> Waiting (refresh) and Greeted -> Greeted (refresh): mood and lastSeen follow.
static void test_known_mac_is_refreshed_without_a_second_greeting() {
  Rig r;
  r.hear(1, 1, 0);
  r.step();
  r.run(2000);
  const uint32_t heardAt = r.clock.nowMs();
  r.hear(1, 1, 3);
  r.step();
  TEST_ASSERT_EQUAL(1u, r.greetings);
  TEST_ASSERT_EQUAL(3, r.social.neighbour(r.slotOf(1)).mood);
  TEST_ASSERT_EQUAL(heardAt, r.social.neighbour(r.slotOf(1)).lastSeen);
}

// Waiting -> Waiting, cooldown not elapsed: a second newcomer waits, then is greeted.
static void test_second_newcomer_waits_for_the_cooldown() {
  Rig r;
  r.hear(1);
  r.step();
  r.hear(2, 4);
  r.step();
  TEST_ASSERT_EQUAL(EntryState::Waiting, r.stateOf(2));
  r.run(S::kGreetCooldownMs - 2 * kTick);
  TEST_ASSERT_EQUAL(1u, r.greetings);
  r.run(2 * kTick);
  TEST_ASSERT_EQUAL(2u, r.greetings);
  TEST_ASSERT_EQUAL(4, r.lastPalette);
}

// Waiting -> Waiting, another slot greeted this tick: one greeting per tick, none skipped.
static void test_newcomers_arriving_together_are_greeted_one_by_one() {
  Rig r;
  r.hear(1);
  r.hear(2);
  r.hear(3);
  r.step();
  TEST_ASSERT_EQUAL(1u, r.greetings);
  for (int i = 0; i < 3; ++i) {  // keep them present while the cooldowns run
    r.hear(1);
    r.hear(2);
    r.hear(3);
    r.run(S::kGreetCooldownMs);
  }
  TEST_ASSERT_EQUAL(3u, r.greetings);
}

// Waiting -> Waiting, budget spent: at most 6 greetings in any 10 min, the 7th waits.
static void test_budget_caps_greetings_in_a_sliding_window() {
  Rig r;
  for (uint8_t n = 1; n <= 7; ++n) r.hear(n);
  // Keep all seven present; 6 greetings fit in 6 cooldowns, the 7th must wait for the window.
  for (uint32_t t = 0; t < S::kBudgetWindowMs - kTick; t += S::kPresenceMs) {
    for (uint8_t n = 1; n <= 7; ++n) r.hear(n);
    r.run(S::kPresenceMs);
  }
  TEST_ASSERT_EQUAL(6u, r.greetings);
  for (uint8_t n = 1; n <= 7; ++n) r.hear(n);
  r.run(S::kPresenceMs);
  TEST_ASSERT_EQUAL(7u, r.greetings);
}

// Waiting/Greeted -> Absent: silent after T_ttl without a frame.
static void test_silent_neighbour_goes_absent_after_the_ttl() {
  Rig r;
  r.hear(1);
  r.step();
  r.run(S::kTtlMs - kTick);
  TEST_ASSERT_EQUAL(EntryState::Greeted, r.stateOf(1));
  r.run(2 * kTick);
  TEST_ASSERT_EQUAL(EntryState::Absent, r.stateOf(1));
}

// Waiting -> Absent, then Absent (never greeted) -> Waiting: a newcomer that leaves while the
// budget is spent is still greeted when it comes back.
static void test_newcomer_that_left_before_its_turn_is_greeted_on_return() {
  Rig r;
  for (uint8_t n = 1; n <= 6; ++n) {  // spend the budget, one greeting per cooldown
    r.hear(n);
    r.run(S::kGreetCooldownMs);
  }
  TEST_ASSERT_EQUAL(6u, r.greetings);
  r.hear(7);
  r.step();
  TEST_ASSERT_EQUAL(EntryState::Waiting, r.stateOf(7));
  r.run(S::kTtlMs + kTick);
  TEST_ASSERT_EQUAL(EntryState::Absent, r.stateOf(7));
  r.run(S::kBudgetWindowMs);  // budget free again, 7 still away
  TEST_ASSERT_EQUAL(6u, r.greetings);
  r.hear(7);
  r.step();
  TEST_ASSERT_EQUAL(7u, r.greetings);
}

// Absent -> Greeted: back within T_regreet, no second hello.
static void test_short_absence_is_not_greeted_again() {
  Rig r;
  r.hear(1);
  r.step();
  r.run(S::kTtlMs + kTick);
  TEST_ASSERT_EQUAL(EntryState::Absent, r.stateOf(1));
  r.hear(1);
  r.run(S::kGreetCooldownMs);
  TEST_ASSERT_EQUAL(EntryState::Greeted, r.stateOf(1));
  TEST_ASSERT_EQUAL(1u, r.greetings);
}

// Absent -> Waiting: back after more than T_regreet, greeted again.
static void test_long_absence_is_greeted_again() {
  Rig r;
  r.hear(1);
  r.step();
  r.run(S::kRegreetMs + kTick);
  r.hear(1);
  r.step();
  TEST_ASSERT_EQUAL(2u, r.greetings);
}

// Absent (oldest) -> Waiting: a full table evicts the absent entry with the oldest lastSeen.
static void test_full_table_evicts_the_oldest_absent_entry() {
  Rig r;
  r.hear(1);
  r.step();
  r.run(1000);
  r.hear(2);
  r.step();
  for (uint8_t n = 3; n <= S::kCapacity; ++n) r.hear(n);
  r.run(S::kTtlMs + 2000);  // 1 and 2 absent, others too, 1 the oldest
  for (uint8_t n = 3; n <= S::kCapacity; ++n) r.hear(n);
  r.run(4 * kTick);  // 14 frames, 4 per tick
  r.hear(99);
  r.step();
  TEST_ASSERT_EQUAL(-1, r.slotOf(1));
  TEST_ASSERT_TRUE(r.slotOf(2) >= 0);
  TEST_ASSERT_TRUE(r.slotOf(99) >= 0);
  TEST_ASSERT_EQUAL(0u, r.social.stats().dropFull);
}

// No free or absent slot: the newcomer is dropped and counted, present neighbours stay.
static void test_full_table_of_present_entries_drops_the_newcomer() {
  Rig r;
  for (uint8_t n = 1; n <= S::kCapacity; ++n) r.hear(n);
  r.run(S::kCapacity / S::kMaxFramesPerTick * kTick);
  r.hear(99);
  r.step();
  TEST_ASSERT_EQUAL(-1, r.slotOf(99));
  TEST_ASSERT_EQUAL(1u, r.social.stats().dropFull);
  for (uint8_t n = 1; n <= S::kCapacity; ++n) TEST_ASSERT_TRUE(r.slotOf(n) >= 0);
}

// Allowlist failures are dropped silently and counted by reason; the table stays empty.
static void test_invalid_frames_are_counted_by_reason() {
  Rig r;
  const uint8_t shortFrame[3] = {0x43, 0x42, 1};
  const uint8_t badMagic[kFrameSize] = {0, 0x42, 1, 1, 0, 0, 0, 0};
  const uint8_t badVersion[kFrameSize] = {0x43, 0x42, 2, 1, 0, 0, 0, 0};
  const uint8_t badType[kFrameSize] = {0x43, 0x42, 1, 2, 0, 0, 0, 0};
  const uint8_t badRange[kFrameSize] = {0x43, 0x42, 1, 1, 9, 0, 0, 0};
  r.radio.inject(shortFrame, sizeof(shortFrame), macOf(1));
  r.radio.inject(badMagic, kFrameSize, macOf(2));
  r.radio.inject(badVersion, kFrameSize, macOf(3));
  r.radio.inject(badType, kFrameSize, macOf(4));
  r.radio.inject(badRange, kFrameSize, macOf(5));
  r.run(3 * kTick);
  const auto& st = r.social.stats();
  TEST_ASSERT_EQUAL(1u, st.dropLen);
  TEST_ASSERT_EQUAL(1u, st.dropMagic);
  TEST_ASSERT_EQUAL(1u, st.dropVersion);
  TEST_ASSERT_EQUAL(1u, st.dropType);
  TEST_ASSERT_EQUAL(1u, st.dropRange);
  for (uint8_t i = 0; i < S::kCapacity; ++i) TEST_ASSERT_EQUAL(EntryState::Free, r.social.neighbour(i).state);
  TEST_ASSERT_EQUAL(0u, r.greetings);
}

// Flood: 1 000 frames in one burst are decoded 4 per tick, and greetings stay capped.
static void test_flood_is_drained_four_frames_per_tick() {
  Rig r;
  for (int i = 0; i < 1000; ++i) r.hear(static_cast<uint8_t>(i));
  r.step();
  TEST_ASSERT_EQUAL(1000u - S::kMaxFramesPerTick, r.radio.pending());
  r.run(S::kBudgetWindowMs - kTick);
  TEST_ASSERT_TRUE(r.greetings <= S::kBudget);
}

// Broadcast: on the first tick, then every 5 s ± 1 s, carrying the encoded SelfState.
static void test_presence_is_broadcast_with_jitter() {
  Rig r;
  r.step();
  TEST_ASSERT_EQUAL(1u, r.radio.sent.size());
  uint8_t expected[kFrameSize];
  cubbies::encodePresence(Presence{2, 1, true}, expected);
  TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, r.radio.sent[0].data(), kFrameSize);
  uint32_t last = r.clock.nowMs();
  size_t seen = 1;
  bool varied = false;
  uint32_t firstGap = 0;
  for (int i = 0; i < 20000 / kTick * 10; ++i) {
    r.step();
    if (r.radio.sent.size() == seen) continue;
    const uint32_t gap = r.clock.nowMs() - last;
    TEST_ASSERT_TRUE(gap >= S::kPresenceMs - S::kJitterMs);
    TEST_ASSERT_TRUE(gap <= S::kPresenceMs + S::kJitterMs + kTick);
    if (firstGap == 0) firstGap = gap;
    else if (gap != firstGap) varied = true;
    last = r.clock.nowMs();
    seen = r.radio.sent.size();
  }
  TEST_ASSERT_TRUE(seen > 30);
  TEST_ASSERT_TRUE(varied);
}

// The mood byte on the air follows the SelfState sleep state (05b via the codec).
static void test_asleep_cube_broadcasts_mood_five() {
  Rig r;
  r.clock.advance(kTick);
  r.social.tick(SelfState{0, false, SleepState::Asleep, SleepState::Dozing, Mood::Happy});
  TEST_ASSERT_EQUAL(1u, r.radio.sent.size());
  TEST_ASSERT_EQUAL(5, r.radio.sent[0][5]);
}

// Every timer works across the 32-bit wrap.
static void test_timers_survive_the_clock_wrap() {
  Rig r(0xFFFFFFFFu - 3000);
  r.hear(1);
  r.step();
  r.hear(2);
  r.step();
  r.run(S::kGreetCooldownMs);  // crosses the wrap
  TEST_ASSERT_EQUAL(2u, r.greetings);
  r.run(S::kTtlMs + kTick);
  TEST_ASSERT_EQUAL(EntryState::Absent, r.stateOf(1));
  const size_t before = r.radio.sent.size();
  r.run(S::kPresenceMs + S::kJitterMs + kTick);
  TEST_ASSERT_TRUE(r.radio.sent.size() > before);
}

// Two cubes on one LoopbackRadio air greet each other once, with no handshake (02).
static void test_two_cubes_greet_each_other_once() {
  Air air;
  FakeClock clock(5000);
  LoopbackRadio ra(air, macOf(0xA), clock), rb(air, macOf(0xB), clock);
  Prng pa(1, 1), pb(2, 2);
  SocialProtocol a(clock, ra, pa), b(clock, rb, pb);
  const SelfState sa{3, true, SleepState::Awake, SleepState::Dozing, Mood::Curious};
  const SelfState sb{6, false, SleepState::Awake, SleepState::Dozing, Mood::Curious};
  uint32_t aGreets = 0, bGreets = 0;
  uint8_t aSaw = 0xFF, bSaw = 0xFF;
  for (uint32_t t = 0; t < 120000; t += kTick) {
    clock.advance(kTick);
    const SocialOutput oa = a.tick(sa);
    const SocialOutput ob = b.tick(sb);
    if (oa.newcomer) ++aGreets, aSaw = oa.paletteIndex;
    if (ob.newcomer) ++bGreets, bSaw = ob.paletteIndex;
  }
  TEST_ASSERT_EQUAL(1u, aGreets);
  TEST_ASSERT_EQUAL(1u, bGreets);
  TEST_ASSERT_EQUAL(6, aSaw);
  TEST_ASSERT_EQUAL(3, bSaw);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_unknown_mac_is_stored);
  RUN_TEST(test_first_newcomer_is_greeted);
  RUN_TEST(test_known_mac_is_refreshed_without_a_second_greeting);
  RUN_TEST(test_second_newcomer_waits_for_the_cooldown);
  RUN_TEST(test_newcomers_arriving_together_are_greeted_one_by_one);
  RUN_TEST(test_budget_caps_greetings_in_a_sliding_window);
  RUN_TEST(test_silent_neighbour_goes_absent_after_the_ttl);
  RUN_TEST(test_newcomer_that_left_before_its_turn_is_greeted_on_return);
  RUN_TEST(test_short_absence_is_not_greeted_again);
  RUN_TEST(test_long_absence_is_greeted_again);
  RUN_TEST(test_full_table_evicts_the_oldest_absent_entry);
  RUN_TEST(test_full_table_of_present_entries_drops_the_newcomer);
  RUN_TEST(test_invalid_frames_are_counted_by_reason);
  RUN_TEST(test_flood_is_drained_four_frames_per_tick);
  RUN_TEST(test_presence_is_broadcast_with_jitter);
  RUN_TEST(test_asleep_cube_broadcasts_mood_five);
  RUN_TEST(test_timers_survive_the_clock_wrap);
  RUN_TEST(test_two_cubes_greet_each_other_once);
  return UNITY_END();
}
