// Mood engine: one test per row of the 05a transition table, plus the "unlisted stimuli leave
// the mood unchanged" rule and the clock wrap.
#include <unity.h>

#include "FakeClock.h"
#include "core/MoodEngine.h"

using cubbies::Mood;
using cubbies::MoodEffect;
using cubbies::MoodEngine;
using cubbies::Stimulus;
using cubbies::test::FakeClock;

namespace {
constexpr uint32_t kTick = 40;

struct Rig {
  explicit Rig(uint32_t startMs = 1000) : clock(startMs), engine(clock) {}
  FakeClock clock;
  MoodEngine engine;

  // Ticks for `ms`; returns how many times `effect` was emitted.
  int run(uint32_t ms, MoodEffect effect = MoodEffect::None) {
    int count = 0;
    for (uint32_t t = 0; t < ms; t += kTick) {
      clock.advance(kTick);
      if (engine.tick() == effect && effect != MoodEffect::None) ++count;
    }
    return count;
  }
  void toBored() { run(MoodEngine::kBoredMs + kTick); }
  void toTired() { toBored(); run(MoodEngine::kTiredMs + kTick); }
  void toHappy() { engine.stimulus(Stimulus::Tap); }
  void toAnnoyed() { engine.stimulus(Stimulus::Loud); }
};
}  // namespace

void setUp() {}
void tearDown() {}

// — -> Curious: entered at boot and at every restart (wake).
static void test_starts_and_restarts_in_curious() {
  Rig r;
  TEST_ASSERT_EQUAL(Mood::Curious, r.engine.mood());
  r.toAnnoyed();
  r.engine.restart();
  TEST_ASSERT_EQUAL(Mood::Curious, r.engine.mood());
  r.run(MoodEngine::kBoredMs - kTick);  // timers restart from now too
  TEST_ASSERT_EQUAL(Mood::Curious, r.engine.mood());
}

// Curious -> Happy: tap or double tap.
static void test_tap_and_double_tap_make_curious_happy() {
  Rig a;
  a.engine.stimulus(Stimulus::Tap);
  TEST_ASSERT_EQUAL(Mood::Happy, a.engine.mood());
  Rig b;
  b.engine.stimulus(Stimulus::DoubleTap);
  TEST_ASSERT_EQUAL(Mood::Happy, b.engine.mood());
}

// Curious -> Bored: idle ≥ T_bored, not before.
static void test_curious_gets_bored_after_t_bored_idle() {
  Rig r;
  r.run(MoodEngine::kBoredMs - kTick);
  TEST_ASSERT_EQUAL(Mood::Curious, r.engine.mood());
  r.run(2 * kTick);
  TEST_ASSERT_EQUAL(Mood::Bored, r.engine.mood());
}

// Any stimulus restarts the idle timer, NewcomerSeen included.
static void test_any_stimulus_restarts_the_idle_timer() {
  Rig r;
  r.run(MoodEngine::kBoredMs - 1000);
  r.engine.stimulus(Stimulus::NewcomerSeen);
  r.run(MoodEngine::kBoredMs - 1000);
  TEST_ASSERT_EQUAL(Mood::Curious, r.engine.mood());
}

// Curious, Happy -> Annoyed: loud, with the grumpy beep; a voice or a clap never annoys.
static void test_loud_annoys_curious_and_happy_with_a_grumpy_beep() {
  Rig a;
  TEST_ASSERT_EQUAL(MoodEffect::Grumpy, a.engine.stimulus(Stimulus::Loud));
  TEST_ASSERT_EQUAL(Mood::Annoyed, a.engine.mood());
  Rig b;
  b.toHappy();
  TEST_ASSERT_EQUAL(MoodEffect::Grumpy, b.engine.stimulus(Stimulus::Loud));
  TEST_ASSERT_EQUAL(Mood::Annoyed, b.engine.mood());
}
static void test_a_call_never_annoys() {
  const Stimulus calls[] = {Stimulus::Voice, Stimulus::Clap};
  for (Stimulus s : calls) {
    Rig r;
    TEST_ASSERT_EQUAL(MoodEffect::None, r.engine.stimulus(s));
    TEST_ASSERT_EQUAL(Mood::Curious, r.engine.mood());
    r.toHappy();
    r.engine.stimulus(s);
    TEST_ASSERT_EQUAL(Mood::Happy, r.engine.mood());
  }
}

// Happy -> Happy: a long stroke keeps the cube happy and restarts the calm timer.
static void test_stroke_keeps_happy_and_restarts_the_settle_timer() {
  Rig r;
  r.toHappy();
  r.run(MoodEngine::kSettleMs - 1000);
  r.engine.stimulus(Stimulus::Stroke);
  TEST_ASSERT_EQUAL(Mood::Happy, r.engine.mood());
  r.run(MoodEngine::kSettleMs - 1000);
  TEST_ASSERT_EQUAL(Mood::Happy, r.engine.mood());
}

// Happy -> Curious: calm ≥ T_settle; NewcomerSeen is not a disturbance of the calm.
static void test_happy_settles_to_curious_after_t_settle_calm() {
  Rig r;
  r.toHappy();
  r.run(MoodEngine::kSettleMs - 1000);
  r.engine.stimulus(Stimulus::NewcomerSeen);
  r.run(1000 + kTick);
  TEST_ASSERT_EQUAL(Mood::Curious, r.engine.mood());
}

// Bored -> Curious: any stimulus.
static void test_any_stimulus_wakes_bored_to_curious() {
  const Stimulus all[] = {Stimulus::Tap, Stimulus::DoubleTap, Stimulus::Stroke, Stimulus::Voice,
                          Stimulus::Clap, Stimulus::Loud, Stimulus::NewcomerSeen};
  for (Stimulus s : all) {
    Rig r;
    r.toBored();
    TEST_ASSERT_EQUAL(MoodEffect::None, r.engine.stimulus(s));
    TEST_ASSERT_EQUAL(Mood::Curious, r.engine.mood());
  }
}

// Bored -> Tired: idle ≥ T_tired in Bored, with the first yawn.
static void test_bored_gets_tired_after_t_tired_with_a_first_yawn() {
  Rig r;
  r.toBored();
  // toBored() overshoots the entry by one tick, so stop two ticks short of T_tired.
  TEST_ASSERT_EQUAL(0, r.run(MoodEngine::kTiredMs - 2 * kTick, MoodEffect::Yawn));
  TEST_ASSERT_EQUAL(Mood::Bored, r.engine.mood());
  TEST_ASSERT_EQUAL(1, r.run(2 * kTick, MoodEffect::Yawn));
  TEST_ASSERT_EQUAL(Mood::Tired, r.engine.mood());
}

// Tired -> Tired: a yawn every T_yawn.
static void test_tired_yawns_every_t_yawn() {
  Rig r;
  r.toTired();
  TEST_ASSERT_EQUAL(3, r.run(3 * MoodEngine::kYawnMs + kTick, MoodEffect::Yawn));
  TEST_ASSERT_EQUAL(Mood::Tired, r.engine.mood());
}

// Tired -> Curious: any stimulus (a cube petted or spoken to never dozes off).
static void test_any_stimulus_wakes_tired_to_curious() {
  const Stimulus all[] = {Stimulus::Tap, Stimulus::DoubleTap, Stimulus::Stroke, Stimulus::Voice,
                          Stimulus::Clap, Stimulus::Loud, Stimulus::NewcomerSeen};
  for (Stimulus s : all) {
    Rig r;
    r.toTired();
    TEST_ASSERT_EQUAL(MoodEffect::None, r.engine.stimulus(s));
    TEST_ASSERT_EQUAL(Mood::Curious, r.engine.mood());
  }
}

// Annoyed -> Curious: calm ≥ T_calm; a tap disturbs the calm and restarts the timer.
static void test_annoyed_calms_down_after_t_calm() {
  Rig r;
  r.toAnnoyed();
  r.run(MoodEngine::kCalmMs - 1000);
  r.engine.stimulus(Stimulus::Tap);
  TEST_ASSERT_EQUAL(Mood::Annoyed, r.engine.mood());  // unlisted: unchanged
  r.run(MoodEngine::kCalmMs - 1000);
  TEST_ASSERT_EQUAL(Mood::Annoyed, r.engine.mood());
  r.run(1000 + kTick);
  TEST_ASSERT_EQUAL(Mood::Curious, r.engine.mood());
}

// Annoyed -> Happy: a long stroke.
static void test_stroke_soothes_annoyed_to_happy() {
  Rig r;
  r.toAnnoyed();
  r.engine.stimulus(Stimulus::Stroke);
  TEST_ASSERT_EQUAL(Mood::Happy, r.engine.mood());
}

// Unlisted stimuli leave the mood unchanged.
static void test_unlisted_stimuli_leave_the_mood_unchanged() {
  Rig a;
  a.engine.stimulus(Stimulus::Stroke);  // stroke in Curious: a purr, no mood change
  TEST_ASSERT_EQUAL(Mood::Curious, a.engine.mood());
  Rig b;
  b.toHappy();
  b.engine.stimulus(Stimulus::Tap);
  TEST_ASSERT_EQUAL(Mood::Happy, b.engine.mood());
  Rig c;
  c.toAnnoyed();
  TEST_ASSERT_EQUAL(MoodEffect::None, c.engine.stimulus(Stimulus::Loud));  // no second grumpy beep
  TEST_ASSERT_EQUAL(Mood::Annoyed, c.engine.mood());
}

// Timers are ages on the uint32 clock.
static void test_timers_survive_the_clock_wrap() {
  Rig r(0xFFFFFFFFu - 1000);
  r.run(MoodEngine::kBoredMs - kTick);
  TEST_ASSERT_EQUAL(Mood::Curious, r.engine.mood());
  r.run(2 * kTick);
  TEST_ASSERT_EQUAL(Mood::Bored, r.engine.mood());
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_starts_and_restarts_in_curious);
  RUN_TEST(test_tap_and_double_tap_make_curious_happy);
  RUN_TEST(test_curious_gets_bored_after_t_bored_idle);
  RUN_TEST(test_any_stimulus_restarts_the_idle_timer);
  RUN_TEST(test_loud_annoys_curious_and_happy_with_a_grumpy_beep);
  RUN_TEST(test_a_call_never_annoys);
  RUN_TEST(test_stroke_keeps_happy_and_restarts_the_settle_timer);
  RUN_TEST(test_happy_settles_to_curious_after_t_settle_calm);
  RUN_TEST(test_any_stimulus_wakes_bored_to_curious);
  RUN_TEST(test_bored_gets_tired_after_t_tired_with_a_first_yawn);
  RUN_TEST(test_tired_yawns_every_t_yawn);
  RUN_TEST(test_any_stimulus_wakes_tired_to_curious);
  RUN_TEST(test_annoyed_calms_down_after_t_calm);
  RUN_TEST(test_stroke_soothes_annoyed_to_happy);
  RUN_TEST(test_unlisted_stimuli_leave_the_mood_unchanged);
  RUN_TEST(test_timers_survive_the_clock_wrap);
  return UNITY_END();
}
