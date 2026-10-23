// Mood contagion (social-group/05a): one test per row of the transition table, the two-cube
// worked example and the clock wrap. The tally is built by hand; its source (the neighbour
// table) is tested in test_native_social.
#include <unity.h>

#include "FakeClock.h"
#include "core/Mood.h"
#include "core/MoodEngine.h"

using cubbies::Mood;
using cubbies::MoodEffect;
using cubbies::MoodEngine;
using cubbies::MoodTally;
using cubbies::Stimulus;
using cubbies::test::FakeClock;

namespace {
constexpr uint32_t kTick = 40;
using E = MoodEngine;

MoodTally tallyOf(Mood m, uint8_t votes = 1) {
  MoodTally t;
  t.votes[static_cast<uint8_t>(m)] = votes;
  return t;
}

struct Rig {
  explicit Rig(uint32_t startMs = 1000) : clock(startMs), engine(clock) {}
  FakeClock clock;
  MoodEngine engine;

  // Ticks contagion then the timers, like main.cpp; returns the last contagion effect.
  MoodEffect run(uint32_t ms, const MoodTally& t) {
    MoodEffect last = MoodEffect::None;
    for (uint32_t s = 0; s < ms; s += kTick) {
      clock.advance(kTick);
      const MoodEffect e = engine.contagion(t);
      if (e != MoodEffect::None) last = e;
      engine.tick();
    }
    return last;
  }
};
}  // namespace

void setUp() {}
void tearDown() {}

// Row 1: no vote (alone) — nothing changes.
static void test_alone_nothing_changes() {
  Rig r;
  r.run(E::kContagionMs * 3, MoodTally{});
  TEST_ASSERT_EQUAL(Mood::Curious, r.engine.mood());
  Mood m;
  TEST_ASSERT_FALSE(cubbies::pulledMood(MoodTally{}, m));
}

// Row 2: the plurality is my own mood — nothing changes.
static void test_plurality_equal_to_own_mood_changes_nothing() {
  Rig r;
  r.run(E::kContagionMs * 3, tallyOf(Mood::Curious, 3));
  TEST_ASSERT_EQUAL(Mood::Curious, r.engine.mood());
  r.run(E::kContagionMs + 2 * kTick, tallyOf(Mood::Happy));  // no bogus pull used the refractory
  TEST_ASSERT_EQUAL(Mood::Happy, r.engine.mood());
}

// Rows 3 and 4: held < T_contagion unchanged, held ≥ T_contagion pulled.
static void test_candidate_held_for_t_contagion_pulls() {
  Rig r;
  r.run(E::kContagionMs - kTick, tallyOf(Mood::Happy));
  TEST_ASSERT_EQUAL(Mood::Curious, r.engine.mood());
  r.run(2 * kTick, tallyOf(Mood::Happy));
  TEST_ASSERT_EQUAL(Mood::Happy, r.engine.mood());
}

// Row 4: the entry effect applies (Grumpy for Annoyed).
static void test_pull_into_annoyed_grumbles() {
  Rig r;
  TEST_ASSERT_EQUAL(MoodEffect::Grumpy, r.run(E::kContagionMs + 2 * kTick, tallyOf(Mood::Annoyed)));
  TEST_ASSERT_EQUAL(Mood::Annoyed, r.engine.mood());
}

// Row 4: the pulled mood's natural timers restart from the pull.
static void test_pulled_mood_lasts_its_natural_duration() {
  Rig r;
  r.run(E::kContagionMs + 2 * kTick, tallyOf(Mood::Happy));
  TEST_ASSERT_EQUAL(Mood::Happy, r.engine.mood());
  r.run(E::kSettleMs - 4 * kTick, MoodTally{});
  TEST_ASSERT_EQUAL(Mood::Happy, r.engine.mood());
  r.run(4 * kTick, MoodTally{});
  TEST_ASSERT_EQUAL(Mood::Curious, r.engine.mood());
}

// Row 5: a changing candidate restarts the hold.
static void test_changing_candidate_restarts_the_hold() {
  Rig r;
  r.run(E::kContagionMs - 1000, tallyOf(Mood::Happy));
  r.run(E::kContagionMs - 1000, tallyOf(Mood::Bored));
  TEST_ASSERT_EQUAL(Mood::Curious, r.engine.mood());
  r.run(2000, tallyOf(Mood::Bored));
  TEST_ASSERT_EQUAL(Mood::Bored, r.engine.mood());
}

// Row 5 (Holding -> NoCandidate): losing the vote drops the hold.
static void test_losing_the_candidate_drops_the_hold() {
  Rig r;
  r.run(E::kContagionMs - 1000, tallyOf(Mood::Happy));
  r.run(kTick, MoodTally{});
  r.run(E::kContagionMs - 1000, tallyOf(Mood::Happy));
  TEST_ASSERT_EQUAL(Mood::Curious, r.engine.mood());
}

// Row 4: a pull restarts the idle timer too, so a cube pulled into Curious is not bored at once.
static void test_pull_restarts_the_idle_timer() {
  Rig r;
  r.engine.stimulus(Stimulus::Tap);  // Happy, idle timer from now
  r.run(E::kContagionMs + 2 * kTick, tallyOf(Mood::Curious));
  TEST_ASSERT_EQUAL(Mood::Curious, r.engine.mood());
  r.run(E::kBoredMs - 2000, MoodTally{});
  TEST_ASSERT_EQUAL(Mood::Curious, r.engine.mood());
}

// Row 6: an owner stimulus postpones the pull; NewcomerSeen does not.
static void test_owner_attention_postpones_the_pull() {
  Rig r;
  r.run(E::kContagionMs - 1000, tallyOf(Mood::Bored));
  r.engine.stimulus(Stimulus::Voice);
  r.run(E::kContagionMs - kTick, tallyOf(Mood::Bored));
  TEST_ASSERT_EQUAL(Mood::Curious, r.engine.mood());
  r.run(2 * kTick, tallyOf(Mood::Bored));
  TEST_ASSERT_EQUAL(Mood::Bored, r.engine.mood());

  Rig n;
  n.run(E::kContagionMs - 1000, tallyOf(Mood::Annoyed));
  n.engine.stimulus(Stimulus::NewcomerSeen);
  n.run(1000 + 2 * kTick, tallyOf(Mood::Annoyed));
  TEST_ASSERT_EQUAL(Mood::Annoyed, n.engine.mood());
}

// Row 6: a touch-down breaks the calm at once, before its gesture is classified.
static void test_touch_down_postpones_the_pull() {
  Rig r;
  r.run(E::kContagionMs + 1000, MoodTally{});
  r.run(E::kContagionMs - kTick, tallyOf(Mood::Annoyed));  // held, the next tick would pull
  r.engine.touchDown();
  TEST_ASSERT_EQUAL(MoodEffect::None, r.run(kTick, tallyOf(Mood::Annoyed)));
  TEST_ASSERT_EQUAL(Mood::Curious, r.engine.mood());
}

// Row 7: at most one pull per T_refractory.
static void test_refractory_blocks_a_second_pull() {
  Rig r;
  r.run(E::kContagionMs + 2 * kTick, tallyOf(Mood::Happy));
  TEST_ASSERT_EQUAL(Mood::Happy, r.engine.mood());
  const uint32_t pulledAt = r.clock.nowMs() - kTick;  // the pull happened on the previous tick
  r.run(pulledAt + E::kRefractoryMs - kTick - r.clock.nowMs(), tallyOf(Mood::Annoyed));
  TEST_ASSERT_EQUAL(Mood::Curious, r.engine.mood());  // settled, never pulled, one tick early
  r.run(kTick, tallyOf(Mood::Annoyed));
  TEST_ASSERT_EQUAL(Mood::Annoyed, r.engine.mood());  // exactly T_refractory after the pull
}

// Row 11: a restart (wake) keeps the refractory.
static void test_restart_keeps_the_refractory() {
  Rig r;
  r.run(E::kContagionMs + 2 * kTick, tallyOf(Mood::Happy));
  r.engine.restart();
  r.run(E::kContagionMs * 2, tallyOf(Mood::Annoyed));
  TEST_ASSERT_EQUAL(Mood::Curious, r.engine.mood());
}

// Row 8: ties are broken Happy > Annoyed > Curious > Bored.
static void test_ties_follow_the_priority() {
  Mood m;
  MoodTally t;
  t.votes[static_cast<uint8_t>(Mood::Bored)] = 2;
  t.votes[static_cast<uint8_t>(Mood::Curious)] = 2;
  TEST_ASSERT_TRUE(cubbies::pulledMood(t, m));
  TEST_ASSERT_EQUAL(Mood::Curious, m);
  t.votes[static_cast<uint8_t>(Mood::Annoyed)] = 2;
  TEST_ASSERT_TRUE(cubbies::pulledMood(t, m));
  TEST_ASSERT_EQUAL(Mood::Annoyed, m);
  t.votes[static_cast<uint8_t>(Mood::Happy)] = 2;
  TEST_ASSERT_TRUE(cubbies::pulledMood(t, m));
  TEST_ASSERT_EQUAL(Mood::Happy, m);
  t.votes[static_cast<uint8_t>(Mood::Bored)] = 3;  // a strict plurality beats the priority
  TEST_ASSERT_TRUE(cubbies::pulledMood(t, m));
  TEST_ASSERT_EQUAL(Mood::Bored, m);
}

// Rows 9 and 10: Tired never votes, so it is never a candidate.
static void test_tired_never_votes() {
  Mood m;
  TEST_ASSERT_FALSE(cubbies::pulledMood(tallyOf(Mood::Tired, 9), m));
  MoodTally t = tallyOf(Mood::Tired, 9);
  t.votes[static_cast<uint8_t>(Mood::Bored)] = 1;
  TEST_ASSERT_TRUE(cubbies::pulledMood(t, m));
  TEST_ASSERT_EQUAL(Mood::Bored, m);
}

// Row 11: a restart (wake) drops the hold.
static void test_restart_drops_the_hold() {
  Rig r;
  r.run(E::kContagionMs - 1000, tallyOf(Mood::Happy));
  r.engine.restart();
  r.run(E::kContagionMs - 1000, tallyOf(Mood::Happy));
  TEST_ASSERT_EQUAL(Mood::Curious, r.engine.mood());
  r.run(1000 + 2 * kTick, tallyOf(Mood::Happy));
  TEST_ASSERT_EQUAL(Mood::Happy, r.engine.mood());
}

// Row 12 and FS8: two cubes, A petted; one ripple of joy each, both Curious within 60 s.
static void test_two_cubes_converge_without_ping_pong() {
  FakeClock clock(1000);
  MoodEngine a(clock);
  MoodEngine b(clock);
  a.stimulus(Stimulus::Tap);
  int pullsA = 0;
  int pullsB = 0;
  Mood seenByA = b.mood();  // the presence frames lag one T_presence behind
  Mood seenByB = a.mood();
  for (uint32_t t = 0; t < 60000; t += kTick) {
    clock.advance(kTick);
    if (t % 5000 == 0) {
      seenByA = b.mood();
      seenByB = a.mood();
    }
    const Mood beforeA = a.mood();
    const Mood beforeB = b.mood();
    a.contagion(tallyOf(seenByA));
    b.contagion(tallyOf(seenByB));
    if (a.mood() != beforeA) ++pullsA;
    if (b.mood() != beforeB) ++pullsB;
    a.tick();
    b.tick();
  }
  TEST_ASSERT_EQUAL(Mood::Curious, a.mood());
  TEST_ASSERT_EQUAL(Mood::Curious, b.mood());
  TEST_ASSERT_EQUAL(1, pullsA);  // pulled back once at about 16 s (worked example)
  TEST_ASSERT_EQUAL(1, pullsB);
}

// Ages survive the 32-bit wrap.
static void test_contagion_survives_the_clock_wrap() {
  Rig r(0xFFFFFFFFu - 2000);
  r.run(E::kContagionMs + 2 * kTick, tallyOf(Mood::Happy));
  TEST_ASSERT_EQUAL(Mood::Happy, r.engine.mood());
}

// ---- Collective sleep (social-group/05d): the yawn of a drowsy neighbour. ----

MoodTally drowsyTally(uint8_t drowsy = 1) {
  MoodTally t;
  t.drowsy = drowsy;
  return t;
}

// Drives a fresh engine to Bored with nobody around (T_bored of idleness).
static void makeBored(Rig& r) {
  r.run(E::kBoredMs, MoodTally{});
  TEST_ASSERT_EQUAL(Mood::Bored, r.engine.mood());
}

// 05d row 1: no drowsy neighbour — Bored stays Bored.
static void test_yawn_row1_no_drowsy_neighbour_keeps_bored() {
  Rig r;
  makeBored(r);
  r.run(E::kContagionMs * 3, MoodTally{});
  TEST_ASSERT_EQUAL(Mood::Bored, r.engine.mood());
}

// 05d row 2: a cube that is not Bored never catches the yawn.
static void test_yawn_row2_only_bored_catches() {
  Rig r;
  r.run(E::kContagionMs * 3, drowsyTally());
  TEST_ASSERT_EQUAL(Mood::Curious, r.engine.mood());
  r.engine.stimulus(Stimulus::Tap);
  r.run(E::kContagionMs + 2 * kTick, drowsyTally());
  TEST_ASSERT_EQUAL(Mood::Happy, r.engine.mood());
  r.engine.stimulus(Stimulus::Loud);
  r.run(E::kContagionMs + 2 * kTick, drowsyTally());
  TEST_ASSERT_EQUAL(Mood::Annoyed, r.engine.mood());
}

// 05d rows 3 and 4: held < T_contagion unchanged, then Tired with a yawn.
static void test_yawn_rows3_4_held_drowsy_neighbour_makes_bored_tired() {
  Rig r;
  makeBored(r);
  TEST_ASSERT_EQUAL(MoodEffect::None, r.run(E::kContagionMs - kTick, drowsyTally()));
  TEST_ASSERT_EQUAL(Mood::Bored, r.engine.mood());
  TEST_ASSERT_EQUAL(MoodEffect::Yawn, r.run(2 * kTick, drowsyTally()));
  TEST_ASSERT_EQUAL(Mood::Tired, r.engine.mood());
}

// 05d row 5: losing every drowsy neighbour drops the hold; a new one restarts it.
static void test_yawn_row5_losing_the_drowsy_neighbour_drops_the_hold() {
  Rig r;
  makeBored(r);
  r.run(E::kContagionMs - 10 * kTick, drowsyTally());
  r.run(kTick, MoodTally{});
  r.run(E::kContagionMs - kTick, drowsyTally());
  TEST_ASSERT_EQUAL(Mood::Bored, r.engine.mood());
  r.run(2 * kTick, drowsyTally());
  TEST_ASSERT_EQUAL(Mood::Tired, r.engine.mood());
}

// 05d row 6: the owner's attention moves Bored to Curious, so the yawn never comes.
static void test_yawn_row6_owner_attention_wins() {
  Rig r;
  makeBored(r);
  r.run(E::kContagionMs / 2, drowsyTally());
  r.engine.stimulus(Stimulus::Voice);
  r.run(E::kContagionMs * 3, drowsyTally());
  TEST_ASSERT_EQUAL(Mood::Curious, r.engine.mood());
}

// 05d row 6: a touch-down alone breaks the calm and postpones the yawn.
static void test_yawn_row6_touch_down_postpones() {
  Rig r;
  makeBored(r);
  r.run(E::kContagionMs + 2 * kTick, MoodTally{});
  r.engine.touchDown();
  r.run(E::kContagionMs - 2 * kTick, drowsyTally());
  TEST_ASSERT_EQUAL(Mood::Bored, r.engine.mood());
  r.run(4 * kTick, drowsyTally());
  TEST_ASSERT_EQUAL(Mood::Tired, r.engine.mood());
}

// 05d row 6b: a touch-down late in the hold breaks the calm; the yawn waits a fresh calm.
static void test_yawn_row6b_touch_down_mid_hold_waits_fresh_calm() {
  Rig r;
  makeBored(r);
  r.run(E::kContagionMs - 2 * kTick, drowsyTally());
  r.engine.touchDown();
  r.run(E::kContagionMs - 2 * kTick, drowsyTally());
  TEST_ASSERT_EQUAL(Mood::Bored, r.engine.mood());
  r.run(4 * kTick, drowsyTally());
  TEST_ASSERT_EQUAL(Mood::Tired, r.engine.mood());
}

// 05d row 9: a Tired cube gets no extra yawn from its neighbours beyond its own timer.
static void test_yawn_row9_tired_gets_no_extra_yawn() {
  Rig r;
  makeBored(r);
  r.run(E::kContagionMs + 2 * kTick, drowsyTally());
  TEST_ASSERT_EQUAL(Mood::Tired, r.engine.mood());
  TEST_ASSERT_EQUAL(MoodEffect::None, r.run(E::kYawnMs - 4 * kTick, drowsyTally()));
}

// 05d row 10: restart (a wake) drops the yawn hold.
static void test_yawn_row10_restart_drops_the_hold() {
  Rig r;
  makeBored(r);
  r.run(E::kContagionMs - kTick, drowsyTally());
  r.engine.restart();
  TEST_ASSERT_EQUAL(Mood::Curious, r.engine.mood());
  r.run(2 * kTick, drowsyTally());
  TEST_ASSERT_EQUAL(Mood::Curious, r.engine.mood());
}

// 05d row 11: the yawn wins over a G2 candidate, does not start the refractory, and the G2
// pull out of Tired waits a fresh T_contagion of calm.
static void test_yawn_row11_yawn_first_then_g2_waits() {
  Rig r;
  makeBored(r);
  MoodTally t = tallyOf(Mood::Happy);
  t.drowsy = 1;
  TEST_ASSERT_EQUAL(MoodEffect::Yawn, r.run(E::kContagionMs + 2 * kTick, t));
  TEST_ASSERT_EQUAL(Mood::Tired, r.engine.mood());
  r.run(E::kContagionMs - 2 * kTick, t);
  TEST_ASSERT_EQUAL(Mood::Tired, r.engine.mood());
  r.run(4 * kTick, t);
  TEST_ASSERT_EQUAL(Mood::Happy, r.engine.mood());  // no refractory was started by the yawn
}

// 05d row 12 (worked example): three idle cubes, the first Tired one sends the others to Tired.
static void test_yawn_row12_the_first_yawn_spreads() {
  Rig a, b, c;
  makeBored(a);
  makeBored(b);
  makeBored(c);
  a.run(E::kTiredMs, MoodTally{});  // A reaches Tired on its own after T_tired in Bored
  TEST_ASSERT_EQUAL(Mood::Tired, a.engine.mood());
  MoodTally seen;
  seen.drowsy = (a.engine.mood() == Mood::Tired) ? 1 : 0;
  b.run(E::kContagionMs + 2 * kTick, seen);
  TEST_ASSERT_EQUAL(Mood::Tired, b.engine.mood());
  seen.drowsy = (b.engine.mood() == Mood::Tired) ? 1 : 0;  // C hears only B
  c.run(E::kContagionMs + 2 * kTick, seen);
  TEST_ASSERT_EQUAL(Mood::Tired, c.engine.mood());
}

// The yawn hold survives the 32-bit wrap.
static void test_yawn_survives_the_clock_wrap() {
  Rig r(0xFFFFFFFFu - E::kBoredMs - 2000);
  makeBored(r);
  r.run(E::kContagionMs + 2 * kTick, drowsyTally());
  TEST_ASSERT_EQUAL(Mood::Tired, r.engine.mood());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_alone_nothing_changes);
  RUN_TEST(test_plurality_equal_to_own_mood_changes_nothing);
  RUN_TEST(test_candidate_held_for_t_contagion_pulls);
  RUN_TEST(test_pull_into_annoyed_grumbles);
  RUN_TEST(test_pulled_mood_lasts_its_natural_duration);
  RUN_TEST(test_changing_candidate_restarts_the_hold);
  RUN_TEST(test_losing_the_candidate_drops_the_hold);
  RUN_TEST(test_pull_restarts_the_idle_timer);
  RUN_TEST(test_owner_attention_postpones_the_pull);
  RUN_TEST(test_touch_down_postpones_the_pull);
  RUN_TEST(test_refractory_blocks_a_second_pull);
  RUN_TEST(test_ties_follow_the_priority);
  RUN_TEST(test_tired_never_votes);
  RUN_TEST(test_restart_drops_the_hold);
  RUN_TEST(test_restart_keeps_the_refractory);
  RUN_TEST(test_two_cubes_converge_without_ping_pong);
  RUN_TEST(test_contagion_survives_the_clock_wrap);
  RUN_TEST(test_yawn_row1_no_drowsy_neighbour_keeps_bored);
  RUN_TEST(test_yawn_row2_only_bored_catches);
  RUN_TEST(test_yawn_rows3_4_held_drowsy_neighbour_makes_bored_tired);
  RUN_TEST(test_yawn_row5_losing_the_drowsy_neighbour_drops_the_hold);
  RUN_TEST(test_yawn_row6_owner_attention_wins);
  RUN_TEST(test_yawn_row6_touch_down_postpones);
  RUN_TEST(test_yawn_row6b_touch_down_mid_hold_waits_fresh_calm);
  RUN_TEST(test_yawn_row9_tired_gets_no_extra_yawn);
  RUN_TEST(test_yawn_row10_restart_drops_the_hold);
  RUN_TEST(test_yawn_row11_yawn_first_then_g2_waits);
  RUN_TEST(test_yawn_row12_the_first_yawn_spreads);
  RUN_TEST(test_yawn_survives_the_clock_wrap);
  return UNITY_END();
}
