// Stimulus router: the routing rule of 03 (Awake vs not Awake), the generic reactions of S3 and
// 05a ("any" rows, unlisted stimuli still react), and the effect mappings of 05a and 05b.
#include <unity.h>

#include <initializer_list>

#include "core/StimulusRouter.h"
#include "eyes/EyesRenderer.h"

using cubbies::effects;
using cubbies::Gesture;
using cubbies::Lids;
using cubbies::MoodEffect;
using cubbies::Reaction;
using cubbies::route;
using cubbies::Route;
using cubbies::routeChain;
using cubbies::routeNewcomer;
using cubbies::SleepEffect;
using cubbies::SleepState;
using cubbies::SoundClass;
using cubbies::SoundId;
using cubbies::Stimulus;

namespace {
const SleepState kNotAwake[] = {SleepState::Dozing, SleepState::Asleep, SleepState::Waking};

void assertMood(const Route& r, Stimulus s) {
  TEST_ASSERT_TRUE(r.toMood);
  TEST_ASSERT_EQUAL(s, r.moodStimulus);
}
void assertReaction(const Route& r, Reaction x) {
  TEST_ASSERT_TRUE(r.react);
  TEST_ASSERT_EQUAL(x, r.reaction);
}
void assertSound(const Route& r, SoundId id) {
  TEST_ASSERT_TRUE(r.sound);
  TEST_ASSERT_EQUAL(id, r.soundId);
}
void assertSilentAndStill(const Route& r) {
  TEST_ASSERT_FALSE(r.react);
  TEST_ASSERT_FALSE(r.sound);
}
}  // namespace

void setUp() {}
void tearDown() {}

// Awake, touch-down: eyes widen (held), feeds the idle timer, not a mood stimulus (05a "any").
static void test_awake_touch_down_widens_eyes_only() {
  const Route r = route(Gesture::TouchDown, SleepState::Awake);
  TEST_ASSERT_FALSE(r.toMood);
  TEST_ASSERT_TRUE(r.toSleep);
  assertReaction(r, Reaction::TouchDown);
  TEST_ASSERT_FALSE(r.sound);
}

// Awake, tap / double tap / stroke: mood stimulus, idle timer, surprise / joy / purr (S3).
static void test_awake_gestures_reach_the_mood_with_their_reaction() {
  Route r = route(Gesture::Tap, SleepState::Awake);
  assertMood(r, Stimulus::Tap);
  TEST_ASSERT_TRUE(r.toSleep);
  assertReaction(r, Reaction::Surprise);
  assertSound(r, SoundId::Surprise);

  r = route(Gesture::DoubleTap, SleepState::Awake);
  assertMood(r, Stimulus::DoubleTap);
  assertReaction(r, Reaction::Joy);
  assertSound(r, SoundId::Joy);

  r = route(Gesture::Stroke, SleepState::Awake);
  assertMood(r, Stimulus::Stroke);
  assertReaction(r, Reaction::Purr);
  assertSound(r, SoundId::Purr);
}

// Awake, released: a mood stimulus (owner decision, medium press) that ends a held reaction.
static void test_awake_released_reaches_the_mood_and_ends_the_held_reaction() {
  const Route r = route(Gesture::Released, SleepState::Awake);
  assertMood(r, Stimulus::Released);
  TEST_ASSERT_TRUE(r.toSleep);
  TEST_ASSERT_TRUE(r.stopReaction);
  assertSilentAndStill(r);
}

// Awake, voice / clap: a call (UC4), eyes look up, no sound; loud: mood only (grumpy is 05a's).
static void test_awake_sounds_reach_the_mood() {
  Route r = route(SoundClass::Voice, SleepState::Awake);
  assertMood(r, Stimulus::Voice);
  TEST_ASSERT_TRUE(r.toSleep);
  assertReaction(r, Reaction::LookUp);
  TEST_ASSERT_FALSE(r.sound);

  r = route(SoundClass::Clap, SleepState::Awake);
  assertMood(r, Stimulus::Clap);
  assertReaction(r, Reaction::LookUp);

  r = route(SoundClass::Loud, SleepState::Awake);
  assertMood(r, Stimulus::Loud);
  TEST_ASSERT_TRUE(r.toSleep);
  assertSilentAndStill(r);
}

// Awake, NewcomerSeen: mood stimulus, greet + hello beep (UC7); does not feed the idle timer.
static void test_awake_newcomer_greets() {
  const Route r = routeNewcomer(SleepState::Awake);
  assertMood(r, Stimulus::NewcomerSeen);
  TEST_ASSERT_FALSE(r.toSleep);
  assertReaction(r, Reaction::Greet);
  assertSound(r, SoundId::Hello);
}

// Chain reaction (social-group/05b rows 5, 11): Joy eyes and sound while Awake, mood and idle
// timer untouched; nothing otherwise.
static void test_chain_reaction_routes() {
  const Route r = routeChain(SleepState::Awake);
  TEST_ASSERT_FALSE(r.toMood);
  TEST_ASSERT_FALSE(r.toSleep);
  assertReaction(r, Reaction::Joy);
  assertSound(r, SoundId::Joy);
  for (SleepState st : {SleepState::Dozing, SleepState::Asleep, SleepState::Waking}) {
    const Route q = routeChain(st);
    TEST_ASSERT_FALSE(q.react);
    TEST_ASSERT_FALSE(q.sound);
  }
}

// Not Awake: gestures and sounds go to the Sleep cycle only, without reaction or sound.
static void test_not_awake_gestures_and_sounds_go_to_sleep_only() {
  const Gesture gestures[] = {Gesture::TouchDown, Gesture::Tap, Gesture::DoubleTap,
                              Gesture::Stroke, Gesture::Released};
  const SoundClass sounds[] = {SoundClass::Voice, SoundClass::Clap, SoundClass::Loud};
  for (SleepState st : kNotAwake) {
    for (Gesture g : gestures) {
      const Route r = route(g, st);
      TEST_ASSERT_FALSE(r.toMood);
      TEST_ASSERT_TRUE(r.toSleep);
      assertSilentAndStill(r);
    }
    for (SoundClass s : sounds) {
      const Route r = route(s, st);
      TEST_ASSERT_FALSE(r.toMood);
      TEST_ASSERT_TRUE(r.toSleep);
      assertSilentAndStill(r);
    }
  }
}

// Not Awake: NewcomerSeen is dropped (05b: a sleeping cube stays silent when a colleague arrives).
static void test_not_awake_newcomer_is_dropped() {
  for (SleepState st : kNotAwake) {
    const Route r = routeNewcomer(st);
    TEST_ASSERT_FALSE(r.toMood);
    TEST_ASSERT_FALSE(r.toSleep);
    assertSilentAndStill(r);
  }
}

// None in, nothing out, whatever the state.
static void test_none_routes_nowhere() {
  const Route g = route(Gesture::None, SleepState::Awake);
  const Route s = route(SoundClass::None, SleepState::Dozing);
  TEST_ASSERT_FALSE(g.toMood || g.toSleep || g.react || g.sound);
  TEST_ASSERT_FALSE(s.toMood || s.toSleep || s.react || s.sound);
}

// Mood effects (05a): yawn = yawn animation + sound; grumpy = beep (the lids come from the mood).
static void test_mood_effects() {
  Route r = effects(MoodEffect::Yawn);
  assertReaction(r, Reaction::Yawn);
  assertSound(r, SoundId::Yawn);
  r = effects(MoodEffect::Grumpy);
  TEST_ASSERT_FALSE(r.react);
  assertSound(r, SoundId::Grumpy);
  r = effects(MoodEffect::None);
  assertSilentAndStill(r);
}

// Sleep effects (05b table): close the eyes asleep, open them on waking, the wake-up melody only
// from Asleep.
static void test_sleep_effects() {
  Route r = effects(SleepEffect::Dozed);
  assertSilentAndStill(r);
  TEST_ASSERT_EQUAL(Lids::Keep, r.lids);

  r = effects(SleepEffect::FellAsleep);
  assertSilentAndStill(r);
  TEST_ASSERT_EQUAL(Lids::Close, r.lids);

  r = effects(SleepEffect::WokeFromDoze);
  TEST_ASSERT_EQUAL(Lids::Open, r.lids);
  assertReaction(r, Reaction::Wake);
  TEST_ASSERT_FALSE(r.sound);

  r = effects(SleepEffect::WokeFromSleep);
  TEST_ASSERT_EQUAL(Lids::Open, r.lids);
  assertReaction(r, Reaction::Yawn);
  assertSound(r, SoundId::WakeMelody);

  r = effects(SleepEffect::Awoke);
  assertSilentAndStill(r);
  TEST_ASSERT_EQUAL(Lids::Keep, r.lids);
}

// Waking covers its whole wake animation, so the reaction is never cut by an Awake stimulus.
static void test_waking_lasts_as_long_as_its_wake_animation() {
  using cubbies::EyesRenderer;
  using cubbies::SleepCycle;
  const Route doze = effects(SleepEffect::WokeFromDoze);
  const Route sleep = effects(SleepEffect::WokeFromSleep);
  TEST_ASSERT_TRUE(EyesRenderer::reactionDurationMs(doze.reaction) <= SleepCycle::kWakingFromDozeMs);
  TEST_ASSERT_TRUE(EyesRenderer::reactionDurationMs(sleep.reaction) <= SleepCycle::kWakingFromSleepMs);
  TEST_ASSERT_TRUE(SleepCycle::kWakingFromDozeMs >= 1000 && SleepCycle::kWakingFromSleepMs >= 1000);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_awake_touch_down_widens_eyes_only);
  RUN_TEST(test_awake_gestures_reach_the_mood_with_their_reaction);
  RUN_TEST(test_awake_released_reaches_the_mood_and_ends_the_held_reaction);
  RUN_TEST(test_awake_sounds_reach_the_mood);
  RUN_TEST(test_awake_newcomer_greets);
  RUN_TEST(test_chain_reaction_routes);
  RUN_TEST(test_not_awake_gestures_and_sounds_go_to_sleep_only);
  RUN_TEST(test_not_awake_newcomer_is_dropped);
  RUN_TEST(test_none_routes_nowhere);
  RUN_TEST(test_mood_effects);
  RUN_TEST(test_sleep_effects);
  RUN_TEST(test_waking_lasts_as_long_as_its_wake_animation);
  return UNITY_END();
}
