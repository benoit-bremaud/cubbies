// Sleep cycle: one test per row of the 05b transition table, plus the Awake idle-timer rule of
// 03 (gestures and sounds feed the sleep cycle while Awake), the stimuli Waking swallows and the
// clock wrap. The Mood engine is the real one (domain, not mocked).
#include <unity.h>

#include "FakeClock.h"
#include "core/SleepCycle.h"

using cubbies::Brightness;
using cubbies::Gesture;
using cubbies::Mood;
using cubbies::MoodEngine;
using cubbies::SleepCycle;
using cubbies::SleepEffect;
using cubbies::SleepState;
using cubbies::SoundClass;
using cubbies::test::FakeClock;

namespace {
constexpr uint32_t kTick = 40;

struct Rig {
  explicit Rig(uint32_t startMs = 1000) : clock(startMs), mood(clock), sleep(clock, mood) {}
  FakeClock clock;
  MoodEngine mood;
  SleepCycle sleep;

  // Ticks for `ms` like main.cpp (mood only while Awake); returns how many times `effect` fired.
  int run(uint32_t ms, SleepEffect effect = SleepEffect::None) {
    int count = 0;
    for (uint32_t t = 0; t < ms; t += kTick) {
      clock.advance(kTick);
      if (sleep.state() == SleepState::Awake) mood.tick();
      if (sleep.tick() == effect && effect != SleepEffect::None) ++count;
    }
    return count;
  }
  // Tick until the condition holds, so each helper stops on the tick of the transition.
  template <typename Done>
  void runUntil(Done done) {
    while (!done()) run(kTick);
  }
  void toTired() {
    runUntil([this] { return mood.mood() == Mood::Tired; });
    sleep.tick();  // the sleep cycle sees the Tired spell start on this same tick
  }
  void toDozing() {
    toTired();
    runUntil([this] { return sleep.state() == SleepState::Dozing; });
  }
  void toAsleep() {
    toDozing();
    runUntil([this] { return sleep.state() == SleepState::Asleep; });
  }
};
}  // namespace

void setUp() {}
void tearDown() {}

// [*] -> Awake: boot, full brightness.
static void test_boots_awake_at_full_brightness() {
  Rig r;
  TEST_ASSERT_EQUAL(SleepState::Awake, r.sleep.state());
  TEST_ASSERT_EQUAL(Brightness::Full, r.sleep.brightness());
}

// Awake -> Dozing: mood Tired for ≥ T_doze, low brightness; not before.
static void test_dozes_after_tired_for_t_doze() {
  Rig r;
  r.toTired();
  TEST_ASSERT_EQUAL(Mood::Tired, r.mood.mood());
  TEST_ASSERT_EQUAL(0, r.run(SleepCycle::kDozeMs - 2 * kTick, SleepEffect::Dozed));
  TEST_ASSERT_EQUAL(SleepState::Awake, r.sleep.state());
  TEST_ASSERT_EQUAL(1, r.run(3 * kTick, SleepEffect::Dozed));
  TEST_ASSERT_EQUAL(SleepState::Dozing, r.sleep.state());
  TEST_ASSERT_EQUAL(Brightness::Low, r.sleep.brightness());
}

// Awake, not Tired: never dozes, however long it stays idle in another mood.
static void test_does_not_doze_unless_tired() {
  Rig r;
  r.run(MoodEngine::kBoredMs + SleepCycle::kDozeMs);  // Bored for T_doze
  TEST_ASSERT_EQUAL(Mood::Bored, r.mood.mood());
  TEST_ASSERT_EQUAL(SleepState::Awake, r.sleep.state());
}

// Awake (03): a gesture or a sound restarts T_doze, even one the mood ignores (medium press).
static void test_stimulus_while_awake_restarts_t_doze() {
  Rig r;
  r.toTired();
  r.run(SleepCycle::kDozeMs - 1000);
  TEST_ASSERT_EQUAL(SleepEffect::None, r.sleep.gesture(Gesture::TouchDown));
  r.run(SleepCycle::kDozeMs - 1000);
  TEST_ASSERT_EQUAL(SleepState::Awake, r.sleep.state());
  r.sleep.sound(SoundClass::Voice);  // the mood is untouched here: only the router feeds it
  r.run(SleepCycle::kDozeMs - 1000);
  TEST_ASSERT_EQUAL(SleepState::Awake, r.sleep.state());
  r.run(2000);
  TEST_ASSERT_EQUAL(SleepState::Dozing, r.sleep.state());
}

// Dozing -> Waking: touch (any contact), clap, voice or loud; no melody, origin Dozing.
static void test_dozing_wakes_on_touch_clap_voice_loud() {
  {
    Rig r;
    r.toDozing();
    TEST_ASSERT_EQUAL(SleepEffect::WokeFromDoze, r.sleep.gesture(Gesture::TouchDown));
    TEST_ASSERT_EQUAL(SleepState::Waking, r.sleep.state());
    TEST_ASSERT_EQUAL(SleepState::Dozing, r.sleep.wakeOrigin());
  }
  const SoundClass sounds[] = {SoundClass::Clap, SoundClass::Voice, SoundClass::Loud};
  for (SoundClass s : sounds) {
    Rig r;
    r.toDozing();
    TEST_ASSERT_EQUAL(SleepEffect::WokeFromDoze, r.sleep.sound(s));
    TEST_ASSERT_EQUAL(SleepState::Waking, r.sleep.state());
    TEST_ASSERT_EQUAL(SleepState::Dozing, r.sleep.wakeOrigin());
  }
}

// Dozing -> Asleep: no wake-qualifying stimulus for ≥ T_fall_asleep; minimum brightness.
static void test_falls_asleep_after_t_fall_asleep() {
  Rig r;
  r.toDozing();
  TEST_ASSERT_EQUAL(SleepEffect::None, r.sleep.sound(SoundClass::None));  // below threshold
  TEST_ASSERT_EQUAL(0, r.run(SleepCycle::kFallAsleepMs - 2 * kTick, SleepEffect::FellAsleep));
  TEST_ASSERT_EQUAL(SleepState::Dozing, r.sleep.state());
  TEST_ASSERT_EQUAL(1, r.run(3 * kTick, SleepEffect::FellAsleep));
  TEST_ASSERT_EQUAL(SleepState::Asleep, r.sleep.state());
  TEST_ASSERT_EQUAL(Brightness::Minimum, r.sleep.brightness());
}

// Asleep -> Waking: touch or loud (deep-wake threshold), with the melody; origin Asleep.
static void test_asleep_wakes_on_touch_or_loud_with_melody() {
  {
    Rig r;
    r.toAsleep();
    TEST_ASSERT_EQUAL(SleepEffect::WokeFromSleep, r.sleep.gesture(Gesture::TouchDown));
    TEST_ASSERT_EQUAL(SleepState::Waking, r.sleep.state());
    TEST_ASSERT_EQUAL(SleepState::Asleep, r.sleep.wakeOrigin());
  }
  Rig r;
  r.toAsleep();
  TEST_ASSERT_EQUAL(SleepEffect::WokeFromSleep, r.sleep.sound(SoundClass::Loud));
  TEST_ASSERT_EQUAL(SleepState::Asleep, r.sleep.wakeOrigin());
}

// Asleep: a voice or a clap (office background) does not wake the cube (S7).
static void test_asleep_ignores_voice_and_clap() {
  Rig r;
  r.toAsleep();
  TEST_ASSERT_EQUAL(SleepEffect::None, r.sleep.sound(SoundClass::Voice));
  TEST_ASSERT_EQUAL(SleepEffect::None, r.sleep.sound(SoundClass::Clap));
  TEST_ASSERT_EQUAL(SleepState::Asleep, r.sleep.state());
}

// Waking -> Awake: after the wake animation (kWakingMs); full brightness, mood Curious.
static void test_waking_returns_awake_and_restarts_mood() {
  Rig r;
  r.toDozing();
  r.sleep.gesture(Gesture::TouchDown);
  TEST_ASSERT_EQUAL(0, r.run(SleepCycle::kWakingMs - 2 * kTick, SleepEffect::Awoke));
  TEST_ASSERT_EQUAL(SleepState::Waking, r.sleep.state());
  TEST_ASSERT_EQUAL(1, r.run(3 * kTick, SleepEffect::Awoke));
  TEST_ASSERT_EQUAL(SleepState::Awake, r.sleep.state());
  TEST_ASSERT_EQUAL(Brightness::Full, r.sleep.brightness());
  TEST_ASSERT_EQUAL(Mood::Curious, r.mood.mood());
}

// Waking swallows stimuli: a second tap or a loud noise does not restart the wake or the melody.
static void test_waking_ignores_stimuli() {
  Rig r;
  r.toAsleep();
  r.sleep.gesture(Gesture::TouchDown);
  TEST_ASSERT_EQUAL(SleepEffect::None, r.sleep.gesture(Gesture::Tap));
  TEST_ASSERT_EQUAL(SleepEffect::None, r.sleep.gesture(Gesture::TouchDown));
  TEST_ASSERT_EQUAL(SleepEffect::None, r.sleep.sound(SoundClass::Loud));
  TEST_ASSERT_EQUAL(SleepState::Waking, r.sleep.state());
  TEST_ASSERT_EQUAL(SleepState::Asleep, r.sleep.wakeOrigin());
}

// Timers are correct across the 49.7-day wrap of the clock.
static void test_timers_survive_the_clock_wrap() {
  Rig r(0xFFFFFFFFu - MoodEngine::kBoredMs);
  r.toDozing();
  TEST_ASSERT_EQUAL(SleepState::Dozing, r.sleep.state());
  r.run(SleepCycle::kFallAsleepMs + kTick);
  TEST_ASSERT_EQUAL(SleepState::Asleep, r.sleep.state());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_boots_awake_at_full_brightness);
  RUN_TEST(test_dozes_after_tired_for_t_doze);
  RUN_TEST(test_does_not_doze_unless_tired);
  RUN_TEST(test_stimulus_while_awake_restarts_t_doze);
  RUN_TEST(test_dozing_wakes_on_touch_clap_voice_loud);
  RUN_TEST(test_falls_asleep_after_t_fall_asleep);
  RUN_TEST(test_asleep_wakes_on_touch_or_loud_with_melody);
  RUN_TEST(test_asleep_ignores_voice_and_clap);
  RUN_TEST(test_waking_returns_awake_and_restarts_mood);
  RUN_TEST(test_waking_ignores_stimuli);
  RUN_TEST(test_timers_survive_the_clock_wrap);
  return UNITY_END();
}
