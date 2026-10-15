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

int main(int, char**) {
  UNITY_BEGIN();
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
