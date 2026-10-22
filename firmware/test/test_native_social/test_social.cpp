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
      : clock(startMs), radio(air, macOf(0xAA), clock), prng(42, 7), social(clock, radio, prng, macOf(0xAA)) {}
  Air air;
  FakeClock clock;
  LoopbackRadio radio;
  Prng prng;
  SocialProtocol social;
  uint32_t greetings = 0;
  uint8_t lastPalette = 0xFF;
  uint32_t firstGreetAt = 0;

  void hear(uint8_t n, uint8_t palette = 1, uint8_t mood = 0) {
    uint8_t buf[kFrameSize];
    cubbies::encodePresence(Presence{palette, mood, false}, buf);
    radio.inject(buf, kFrameSize, macOf(n));
  }
  SocialOutput step() {
    clock.advance(kTick);
    const SocialOutput o = social.tick(kSelf);
    return record(o);
  }
  // Ticks at an exact time (boundary tests).
  SocialOutput tickAt(uint32_t ms) {
    clock.set(ms);
    return record(social.tick(kSelf));
  }
  SocialOutput record(const SocialOutput& o) {
    if (o.newcomer) {
      if (greetings == 0) firstGreetAt = clock.nowMs();
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

// Free -> Waiting: a valid PRESENCE from an unknown MAC is stored and waits while a cooldown runs.
static void test_unknown_mac_is_stored_waiting() {
  Rig r;
  r.hear(1);
  r.step();  // greets 1, starts the cooldown
  const uint32_t heardAt = r.clock.nowMs();
  r.hear(2, 5, 4);
  r.step();
  const int s = r.slotOf(2);
  TEST_ASSERT_TRUE(s >= 0);
  TEST_ASSERT_EQUAL(EntryState::Waiting, r.stateOf(2));
  TEST_ASSERT_EQUAL(5, r.social.neighbour(s).paletteIndex);
  TEST_ASSERT_EQUAL(4, r.social.neighbour(s).mood);
  TEST_ASSERT_EQUAL(heardAt, r.social.neighbour(s).lastSeen);
}

// Waiting -> Waiting: a waiting entry is refreshed and still waits.
static void test_waiting_entry_is_refreshed() {
  Rig r;
  r.hear(1);
  r.step();
  r.hear(2, 1, 0);
  r.step();
  r.run(1000);
  const uint32_t heardAt = r.clock.nowMs();
  r.hear(2, 6, 3);
  r.step();
  const int s = r.slotOf(2);
  TEST_ASSERT_EQUAL(EntryState::Waiting, r.stateOf(2));
  TEST_ASSERT_EQUAL(6, r.social.neighbour(s).paletteIndex);
  TEST_ASSERT_EQUAL(3, r.social.neighbour(s).mood);
  TEST_ASSERT_EQUAL(heardAt, r.social.neighbour(s).lastSeen);
  TEST_ASSERT_EQUAL(1u, r.greetings);
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

// Greeted -> Greeted (refresh): mood and lastSeen follow.
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

// Waiting -> Waiting, cooldown not elapsed: greeted at exactly T_greet_cooldown, not before.
static void test_second_newcomer_waits_for_the_cooldown() {
  Rig r;
  r.hear(1);
  r.step();
  const uint32_t g = r.firstGreetAt;
  r.hear(2, 4);
  r.tickAt(g + S::kGreetCooldownMs - 1);
  TEST_ASSERT_EQUAL(EntryState::Waiting, r.stateOf(2));
  TEST_ASSERT_EQUAL(1u, r.greetings);
  r.tickAt(g + S::kGreetCooldownMs);
  TEST_ASSERT_EQUAL(2u, r.greetings);
  TEST_ASSERT_EQUAL(4, r.lastPalette);
}

// Waiting -> Waiting, another slot greeted this tick: one greeting per tick, lowest slot first.
static void test_newcomers_arriving_together_are_greeted_one_by_one() {
  Rig r;
  r.hear(1, 4);
  r.hear(2, 5);
  r.hear(3, 6);
  r.step();
  TEST_ASSERT_EQUAL(1u, r.greetings);
  TEST_ASSERT_EQUAL(4, r.lastPalette);
  for (uint8_t i = 0; i < 2; ++i) {  // keep them present while the cooldowns run
    r.hear(1, 4);
    r.hear(2, 5);
    r.hear(3, 6);
    r.run(S::kGreetCooldownMs);
    TEST_ASSERT_EQUAL(2u + i, r.greetings);
    TEST_ASSERT_EQUAL(5 + i, r.lastPalette);
  }
}

// Waiting -> Waiting, budget spent: a 7th greeting needs the oldest of the last 6 to be more
// than 10 min old (strict).
static void test_budget_caps_greetings_in_a_sliding_window() {
  Rig r;
  for (uint8_t n = 1; n <= 6; ++n) {
    r.hear(n);
    r.run(S::kGreetCooldownMs);
  }
  TEST_ASSERT_EQUAL(6u, r.greetings);
  const uint32_t g1 = r.firstGreetAt;
  r.clock.set(g1 + S::kBudgetWindowMs - 1000);
  r.hear(7);
  r.tickAt(g1 + S::kBudgetWindowMs - 500);
  TEST_ASSERT_EQUAL(EntryState::Waiting, r.stateOf(7));
  r.tickAt(g1 + S::kBudgetWindowMs);
  TEST_ASSERT_EQUAL(6u, r.greetings);
  r.tickAt(g1 + S::kBudgetWindowMs + 1);
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

// Absent -> Greeted: back after exactly T_regreet, no second hello.
static void test_absence_of_exactly_t_regreet_is_not_greeted_again() {
  Rig r;
  const uint32_t h = r.clock.nowMs();
  r.hear(1);
  r.step();
  r.tickAt(h + S::kTtlMs + 100);
  TEST_ASSERT_EQUAL(EntryState::Absent, r.stateOf(1));
  r.clock.set(h + S::kRegreetMs);
  r.hear(1);
  r.tickAt(h + S::kRegreetMs);
  TEST_ASSERT_EQUAL(EntryState::Greeted, r.stateOf(1));
  TEST_ASSERT_EQUAL(1u, r.greetings);
}

// Absent -> Waiting: back one millisecond past T_regreet, greeted again.
static void test_absence_past_t_regreet_is_greeted_again() {
  Rig r;
  const uint32_t h = r.clock.nowMs();
  r.hear(1);
  r.step();
  r.tickAt(h + S::kTtlMs + 100);
  r.clock.set(h + S::kRegreetMs + 1);
  r.hear(1);
  r.tickAt(h + S::kRegreetMs + 1);
  TEST_ASSERT_EQUAL(2u, r.greetings);
}

// An entry absent for longer than the 32-bit clock period still owes its re-greet (no aliasing).
static void test_absence_longer_than_the_clock_period_is_greeted_again() {
  Rig r;
  const uint32_t h = r.clock.nowMs();
  r.hear(1);
  r.step();
  r.tickAt(h + S::kTtlMs + 100);
  r.tickAt(h + S::kRegreetMs + 100);
  r.clock.set(h + 5000);  // 49.7 days later: same counter value as just after the first sight
  r.hear(1);
  r.tickAt(h + 5000);
  TEST_ASSERT_EQUAL(2u, r.greetings);
}

// Absent -> Free: past T_regreet the entry is owed a fresh greeting anyway, so its slot is freed
// and no Absent age can grow towards the 2^32 wrap and mislead the eviction order (PR #22).
static void test_absent_entry_is_freed_past_t_regreet() {
  Rig r;
  const uint32_t h = r.clock.nowMs();
  r.hear(1);
  r.step();
  r.tickAt(h + S::kTtlMs + 100);
  r.tickAt(h + S::kRegreetMs);
  TEST_ASSERT_EQUAL(EntryState::Absent, r.stateOf(1));
  r.tickAt(h + S::kRegreetMs + 1);
  TEST_ASSERT_EQUAL(-1, r.slotOf(1));
}

// The greeting ring does not alias either: old greetings never block after a full clock period.
static void test_old_greetings_do_not_block_after_the_clock_period() {
  Rig r;
  r.hear(1);
  r.step();
  const uint32_t g = r.firstGreetAt;
  r.tickAt(g + S::kBudgetWindowMs + 1);
  r.clock.set(g + 2000);  // wrapped: 2 s after the old greeting on the counter
  r.hear(2);
  r.tickAt(g + 2000);
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
  const uint8_t badType[kFrameSize] = {0x43, 0x42, 1, 3, 0, 0, 0, 0};  // 2 is CHAIN since G3
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

// Flood: 10 frames per tick from 200 MACs are decoded 4 per tick; a sustained stream keeps the
// table full (dropFull) and greetings stay at the budget. LoopbackRadio has no queue cap (the
// 8-slot drop-newest is the adapter's), so the sustained phase injects at the drain rate.
static void test_flood_is_drained_four_frames_per_tick() {
  Rig r;
  uint8_t next = 0;
  auto flood = [&](int count) {
    for (int i = 0; i < count; ++i) r.hear(static_cast<uint8_t>(1 + (next++ % 200)));
  };
  for (uint32_t k = 1; k <= 5; ++k) {
    flood(10);
    r.step();
    TEST_ASSERT_EQUAL(k * (10u - S::kMaxFramesPerTick), r.radio.pending());
  }
  for (uint32_t t = 0; t < S::kBudgetWindowMs - 10 * kTick; t += kTick) {
    flood(S::kMaxFramesPerTick);
    r.step();
  }
  TEST_ASSERT_EQUAL(S::kBudget, r.greetings);
  TEST_ASSERT_TRUE(r.social.stats().dropFull > 0);
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
  SocialProtocol a(clock, ra, pa, macOf(0xA)), b(clock, rb, pb, macOf(0xB));
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

// moodTally (social-group/05a rows 1, 9): present awake neighbours vote, Tired, asleep and
// absent ones do not.
static void test_mood_tally_counts_present_awake_non_tired_neighbours() {
  Rig r;
  r.hear(1, 1, 1);  // Happy
  r.hear(2, 1, 1);  // Happy
  r.hear(3, 1, 4);  // Annoyed
  r.hear(6, 1, 2);  // Bored
  r.hear(4, 1, 3);  // Tired: no vote
  r.hear(5, 1, 5);  // asleep: no vote
  r.step();
  r.step();
  TEST_ASSERT_EQUAL(EntryState::Waiting, r.stateOf(6));  // Waiting entries vote too
  cubbies::MoodTally t = r.social.moodTally();
  TEST_ASSERT_EQUAL_UINT8(2, t.votes[static_cast<uint8_t>(Mood::Happy)]);
  TEST_ASSERT_EQUAL_UINT8(1, t.votes[static_cast<uint8_t>(Mood::Annoyed)]);
  TEST_ASSERT_EQUAL_UINT8(1, t.votes[static_cast<uint8_t>(Mood::Bored)]);
  TEST_ASSERT_EQUAL_UINT8(0, t.votes[static_cast<uint8_t>(Mood::Tired)]);
  TEST_ASSERT_EQUAL_UINT8(0, t.votes[static_cast<uint8_t>(Mood::Curious)]);
  r.run(S::kTtlMs + S::kPresenceMs);  // everyone absent
  t = r.social.moodTally();
  for (uint8_t v : t.votes) TEST_ASSERT_EQUAL_UINT8(0, v);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_mood_tally_counts_present_awake_non_tired_neighbours);
  RUN_TEST(test_unknown_mac_is_stored_waiting);
  RUN_TEST(test_waiting_entry_is_refreshed);
  RUN_TEST(test_first_newcomer_is_greeted);
  RUN_TEST(test_known_mac_is_refreshed_without_a_second_greeting);
  RUN_TEST(test_second_newcomer_waits_for_the_cooldown);
  RUN_TEST(test_newcomers_arriving_together_are_greeted_one_by_one);
  RUN_TEST(test_budget_caps_greetings_in_a_sliding_window);
  RUN_TEST(test_silent_neighbour_goes_absent_after_the_ttl);
  RUN_TEST(test_newcomer_that_left_before_its_turn_is_greeted_on_return);
  RUN_TEST(test_absence_of_exactly_t_regreet_is_not_greeted_again);
  RUN_TEST(test_absence_past_t_regreet_is_greeted_again);
  RUN_TEST(test_absence_longer_than_the_clock_period_is_greeted_again);
  RUN_TEST(test_absent_entry_is_freed_past_t_regreet);
  RUN_TEST(test_old_greetings_do_not_block_after_the_clock_period);
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
