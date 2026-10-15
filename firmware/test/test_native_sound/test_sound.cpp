// Sound classifier: one test per row of the 05d transition table, plus baseline behaviour.
#include <unity.h>

#include <vector>

#include "FakeClock.h"
#include "ScriptedMic.h"
#include "core/SoundClassifier.h"

using cubbies::SoundClass;
using cubbies::SoundClassifier;
using cubbies::test::FakeClock;
using cubbies::test::ScriptedMic;

namespace {
constexpr uint32_t kTick = 40;
constexpr uint16_t kRoom = 200;  // office background rms, above kMinLevel

struct Rig {
  explicit Rig(uint16_t background = kRoom) : sound(clock, mic) { play(background, background, 2000); }
  FakeClock clock{1000};
  ScriptedMic mic;
  SoundClassifier sound;

  // Ticks for `ms` at these levels; returns every class emitted, in order.
  std::vector<SoundClass> play(uint16_t rms, uint16_t peak, uint32_t ms) {
    mic.set(rms, peak);
    std::vector<SoundClass> out;
    for (uint32_t t = 0; t < ms; t += kTick) {
      clock.advance(kTick);
      const SoundClass c = sound.tick();
      if (c != SoundClass::None) out.push_back(c);
    }
    return out;
  }
  std::vector<SoundClass> quiet(uint32_t ms = 1000) { return play(kRoom, kRoom, ms); }
};

using Seq = std::vector<SoundClass>;
void assertSeq(const Seq& expected, const Seq& actual) {
  TEST_ASSERT_EQUAL_UINT32(expected.size(), actual.size());
  for (size_t i = 0; i < expected.size(); ++i) TEST_ASSERT_EQUAL(expected[i], actual[i]);
}
}  // namespace

void setUp() {}
void tearDown() {}

// Quiet -> Quiet: steady background emits nothing.
static void test_steady_background_emits_nothing() {
  Rig r;
  assertSeq({}, r.play(kRoom, kRoom, 30000));
}

// Quiet -> Burst -> Quiet: a peak shorter than T_clap is a Clap, once the burst ended.
static void test_short_peak_burst_is_a_clap() {
  Rig r;
  assertSeq({}, r.play(kRoom, kRoom * SoundClassifier::kClapRatio, 2 * kTick));  // 80 ms
  assertSeq({SoundClass::Clap}, r.quiet());
}

// Burst -> Sound: a burst of T_clap or more is no clap.
static void test_long_peak_burst_is_no_clap() {
  Rig r;
  assertSeq({}, r.play(kRoom, kRoom * SoundClassifier::kClapRatio, 4 * kTick));  // 160 ms
  assertSeq({}, r.quiet());
}

// Sound -> Voice: rms ≥ R_voice·ref for T_voice, emitted once per sound.
static void test_sustained_voice_emits_voice_once() {
  Rig r;
  assertSeq({SoundClass::Voice}, r.play(kRoom * 3, kRoom * 3, 2000));
  assertSeq({}, r.quiet());
}

// Sound -> Quiet before T_voice: a short sound is nothing.
static void test_short_sound_is_no_voice() {
  Rig r;
  assertSeq({}, r.play(kRoom * 3, kRoom * 3, 120));
  assertSeq({}, r.quiet());
}

// Sound -> Loud: rms ≥ R_loud·ref for T_loud; a loud sound is also a voice (Voice then Loud).
static void test_sustained_loud_emits_voice_then_loud() {
  Rig r;
  assertSeq({SoundClass::Voice, SoundClass::Loud}, r.play(kRoom * 5, kRoom * 5, 2000));
}

// Sound -> Quiet: the next sound emits again.
static void test_each_new_sound_emits_again() {
  Rig r;
  assertSeq({SoundClass::Voice}, r.play(kRoom * 3, kRoom * 3, 400));
  r.quiet(400);
  assertSeq({SoundClass::Voice}, r.play(kRoom * 3, kRoom * 3, 400));
}

// The baseline follows a slowly louder office: after 60 s at 3× the old room, 3× is background.
static void test_baseline_learns_a_louder_room_in_quiet_steps() {
  Rig r;
  // Ramp up slowly so no step is ever a sound (each step < R_voice × current baseline).
  for (uint16_t level = kRoom; level <= kRoom * 3; level += kRoom / 4) r.play(level, level, 15000);
  assertSeq({}, r.play(kRoom * 3, kRoom * 3, 5000));
}

// The baseline is frozen during a sound: a minute-long conversation does not raise the threshold.
static void test_baseline_is_frozen_while_a_sound_lasts() {
  Rig r;
  assertSeq({SoundClass::Voice}, r.play(kRoom * 3, kRoom * 3, 60000));
  r.quiet(400);
  assertSeq({SoundClass::Voice}, r.play(kRoom * 3, kRoom * 3, 400));
}

// Floor: in a near-silent room the reference is kMinLevel, so a breath is nothing.
static void test_floor_ignores_a_breath_in_a_silent_room() {
  Rig r(5);
  const uint16_t breath = SoundClassifier::kMinLevel;  // well above 2× the background of 5
  assertSeq({}, r.play(breath, breath, 1000));
  const uint16_t voice = SoundClassifier::kMinLevel * 3;
  assertSeq({SoundClass::Voice}, r.play(voice, voice, 400));
}

// Sound -> Loud needs rms ≥ R_loud·ref for the whole T_loud, not just one loud tick late in a
// voice, nor a loudness that started less than T_loud ago (review F2).
static void test_loud_must_be_sustained_for_t_loud() {
  Rig a;
  assertSeq({SoundClass::Voice}, a.play(kRoom * 5 / 2, kRoom * 5 / 2, 480));
  assertSeq({}, a.play(kRoom * 4, kRoom * 4, kTick));            // one loud tick at 520 ms
  assertSeq({}, a.play(kRoom * 5 / 2, kRoom * 5 / 2, 1000));
  Rig b;
  assertSeq({SoundClass::Voice}, b.play(kRoom * 5 / 2, kRoom * 5 / 2, 400));
  assertSeq({}, b.play(kRoom * 5, kRoom * 5, SoundClassifier::kLoudMs - kTick));
  assertSeq({SoundClass::Loud}, b.play(kRoom * 5, kRoom * 5, 2 * kTick));
}

// The loud run starts on the very tick that enters Sound: a loud sound whose first sample is
// loud emits Loud exactly T_loud later (PR #12 review).
static void test_loud_counts_from_the_first_loud_tick() {
  Rig r;
  // 14 ticks: the first enters Sound, the 14th is 520 ms later (≥ T_loud, the 13th is 480 ms).
  assertSeq({SoundClass::Voice, SoundClass::Loud}, r.play(kRoom * 5, kRoom * 5, 14 * kTick));
  Rig early;  // one tick short: not yet
  assertSeq({SoundClass::Voice}, early.play(kRoom * 5, kRoom * 5, 13 * kTick));
}

// Burst -> Sound: a long loud bang counts its loud run from the burst start, like the sound.
static void test_long_loud_bang_counts_loud_from_the_burst_start() {
  const uint16_t bang = kRoom * SoundClassifier::kClapRatio;
  Rig r;
  assertSeq({SoundClass::Voice, SoundClass::Loud}, r.play(kRoom * 5, bang, 14 * kTick));
  Rig early;
  assertSeq({SoundClass::Voice}, early.play(kRoom * 5, bang, 13 * kTick));
}

// Burst -> Quiet: a clap ends its loud run; a loud tail after it is a new sound (05d).
static void test_clap_does_not_leak_its_loud_run_into_the_next_sound() {
  Rig r;
  assertSeq({}, r.play(kRoom * 5, kRoom * SoundClassifier::kClapRatio, 2 * kTick));
  assertSeq({SoundClass::Clap, SoundClass::Voice}, r.play(kRoom * 5, kRoom * 5, 12 * kTick));
}

// Loud held for less than T_loud is only a voice.
static void test_short_loud_is_only_a_voice() {
  Rig r;
  assertSeq({SoundClass::Voice}, r.play(kRoom * 5, kRoom * 5, SoundClassifier::kLoudMs - kTick));
  assertSeq({}, r.quiet());
}

// Thresholds are inclusive: exactly R_voice·ref is a voice, one below is nothing; same for loud.
static void test_ratio_thresholds_are_inclusive() {
  const uint16_t voice = kRoom * SoundClassifier::kVoiceRatio;
  const uint16_t loud = kRoom * SoundClassifier::kLoudRatio;
  Rig a;
  assertSeq({SoundClass::Voice}, a.play(voice, voice, 1000));
  Rig b;
  assertSeq({}, b.play(voice - 1, voice - 1, 1000));
  Rig c;
  assertSeq({SoundClass::Voice, SoundClass::Loud}, c.play(loud, loud, 1000));
  Rig d;
  assertSeq({SoundClass::Voice}, d.play(loud - 1, loud - 1, 1000));
}

// Burst boundary: a peak lasting three ticks (120 ms ≥ T_clap) is no clap, two ticks is.
static void test_clap_boundary_is_t_clap() {
  Rig r;
  assertSeq({}, r.play(kRoom, kRoom * SoundClassifier::kClapRatio, 3 * kTick));
  assertSeq({}, r.quiet());
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_loud_must_be_sustained_for_t_loud);
  RUN_TEST(test_loud_counts_from_the_first_loud_tick);
  RUN_TEST(test_long_loud_bang_counts_loud_from_the_burst_start);
  RUN_TEST(test_clap_does_not_leak_its_loud_run_into_the_next_sound);
  RUN_TEST(test_short_loud_is_only_a_voice);
  RUN_TEST(test_ratio_thresholds_are_inclusive);
  RUN_TEST(test_clap_boundary_is_t_clap);
  RUN_TEST(test_steady_background_emits_nothing);
  RUN_TEST(test_short_peak_burst_is_a_clap);
  RUN_TEST(test_long_peak_burst_is_no_clap);
  RUN_TEST(test_sustained_voice_emits_voice_once);
  RUN_TEST(test_short_sound_is_no_voice);
  RUN_TEST(test_sustained_loud_emits_voice_then_loud);
  RUN_TEST(test_each_new_sound_emits_again);
  RUN_TEST(test_baseline_learns_a_louder_room_in_quiet_steps);
  RUN_TEST(test_baseline_is_frozen_while_a_sound_lasts);
  RUN_TEST(test_floor_ignores_a_breath_in_a_silent_room);
  return UNITY_END();
}
