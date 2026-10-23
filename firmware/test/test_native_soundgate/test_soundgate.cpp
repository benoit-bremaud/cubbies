// Sound gate: one test per row of the 05e transition table, plus the rank order, the volume
// range and the clock wrap.
#include <unity.h>

#include "FakeClock.h"
#include "RecordingAudio.h"
#include "core/SoundGate.h"

using cubbies::kMaxVolume;
using cubbies::SleepState;
using cubbies::SoundGate;
using cubbies::SoundId;
using cubbies::test::FakeClock;
using cubbies::test::RecordingAudio;

namespace {
constexpr uint32_t kTick = 40;

struct Rig {
  explicit Rig(uint32_t startMs = 1000) : clock(startMs), gate(clock, audio) {}
  FakeClock clock;
  RecordingAudio audio;
  SoundGate gate;

  void run(uint32_t ms) {
    for (uint32_t t = 0; t < ms; t += kTick) {
      clock.advance(kTick);
      gate.tick();
    }
  }
};

const SoundId kAll[] = {SoundId::Surprise, SoundId::Joy,    SoundId::Purr,      SoundId::Hello,
                        SoundId::Yawn,     SoundId::Grumpy, SoundId::WakeMelody,
                        SoundId::RitualChime, SoundId::RitualTick, SoundId::Adoption,
                        SoundId::ChoirLow, SoundId::ChoirMid, SoundId::ChoirHigh};
}  // namespace

void setUp() {}
void tearDown() {}

// — -> Idle: nothing plays at boot.
static void test_boots_idle() {
  Rig r;
  TEST_ASSERT_FALSE(r.gate.playing());
  TEST_ASSERT_EQUAL(0, r.audio.plays);
}

// Idle -> Playing: play(id) while Awake or Waking forwards the notes at kVolume.
static void test_plays_while_awake_or_waking_at_the_constant_volume() {
  const SleepState states[] = {SleepState::Awake, SleepState::Waking};
  for (SleepState s : states) {
    Rig r;
    r.gate.setSleepState(s);
    TEST_ASSERT_TRUE(r.gate.play(SoundId::WakeMelody));
    TEST_ASSERT_TRUE(r.gate.playing());
    TEST_ASSERT_EQUAL(1, r.audio.plays);
    TEST_ASSERT_NOT_NULL(r.audio.lastNotes);
    TEST_ASSERT_GREATER_THAN(0, r.audio.lastCount);
    TEST_ASSERT_EQUAL(SoundGate::kVolume, r.audio.lastLevel);
  }
}

// Idle -> Idle: play(id) while Dozing or Asleep is rejected; nothing reaches the buzzer.
static void test_rejects_every_sound_while_dozing_or_asleep() {
  const SleepState states[] = {SleepState::Dozing, SleepState::Asleep};
  for (SleepState s : states) {
    Rig r;
    r.gate.setSleepState(s);
    for (SoundId id : kAll) TEST_ASSERT_FALSE(r.gate.play(id));
    TEST_ASSERT_FALSE(r.gate.playing());
    TEST_ASSERT_EQUAL(0, r.audio.plays);
  }
}

// Playing -> Playing: a strictly higher rank replaces the sound in progress.
static void test_higher_rank_replaces_the_sound_in_progress() {
  Rig r;
  r.gate.play(SoundId::Surprise);
  const cubbies::Note* surprise = r.audio.lastNotes;
  r.run(kTick);
  TEST_ASSERT_TRUE(r.gate.play(SoundId::Grumpy));
  TEST_ASSERT_EQUAL(2, r.audio.plays);
  TEST_ASSERT_TRUE(r.audio.lastNotes != surprise);
  // The replacing sound's own duration now decides the end, counted from the replacement.
  r.run(SoundGate::durationMs(SoundId::Grumpy) - kTick);
  TEST_ASSERT_TRUE(r.gate.playing());
  r.run(kTick);
  TEST_ASSERT_FALSE(r.gate.playing());
  r.gate.play(SoundId::Grumpy);
  TEST_ASSERT_TRUE(r.gate.play(SoundId::WakeMelody));
  TEST_ASSERT_EQUAL(4, r.audio.plays);
}

// Playing -> Playing: an equal or lower rank is dropped (no queue).
static void test_equal_or_lower_rank_is_dropped() {
  Rig r;
  r.gate.play(SoundId::Yawn);
  TEST_ASSERT_FALSE(r.gate.play(SoundId::Yawn));
  TEST_ASSERT_FALSE(r.gate.play(SoundId::Surprise));
  TEST_ASSERT_EQUAL(1, r.audio.plays);
  r.run(SoundGate::durationMs(SoundId::Yawn) + kTick);  // dropped means not queued either
  TEST_ASSERT_EQUAL(1, r.audio.plays);
}

// Playing -> Idle: the sound ends once its note durations have elapsed; then anything plays.
static void test_sound_ends_after_its_duration() {
  Rig r;
  r.gate.play(SoundId::Grumpy);
  r.run(SoundGate::durationMs(SoundId::Grumpy) - kTick);
  TEST_ASSERT_TRUE(r.gate.playing());
  r.run(kTick);  // exactly at the duration
  TEST_ASSERT_FALSE(r.gate.playing());
  TEST_ASSERT_TRUE(r.gate.play(SoundId::Surprise));
}

// Playing -> Idle: dozing off or falling asleep stops the sound in progress.
static void test_entering_doze_or_sleep_stops_the_sound() {
  const SleepState states[] = {SleepState::Dozing, SleepState::Asleep};
  for (SleepState s : states) {
    Rig r;
    r.gate.play(SoundId::Yawn);
    r.gate.setSleepState(s);
    TEST_ASSERT_FALSE(r.gate.playing());
    TEST_ASSERT_EQUAL(1, r.audio.stops);
  }
}

// Ranks: wake-up melody > grumpy > yawn > surprise, joy, purr, hello, ritual sounds; every sound has notes.
static void test_rank_order_and_durations() {
  TEST_ASSERT_EQUAL(0, SoundGate::rank(SoundId::Surprise));
  TEST_ASSERT_EQUAL(0, SoundGate::rank(SoundId::Joy));
  TEST_ASSERT_EQUAL(0, SoundGate::rank(SoundId::Purr));
  TEST_ASSERT_EQUAL(0, SoundGate::rank(SoundId::Hello));
  TEST_ASSERT_EQUAL(1, SoundGate::rank(SoundId::Yawn));
  TEST_ASSERT_EQUAL(2, SoundGate::rank(SoundId::Grumpy));
  TEST_ASSERT_EQUAL(3, SoundGate::rank(SoundId::WakeMelody));
  TEST_ASSERT_EQUAL(0, SoundGate::rank(SoundId::RitualChime));  // identity/05b
  TEST_ASSERT_EQUAL(0, SoundGate::rank(SoundId::RitualTick));
  TEST_ASSERT_EQUAL(0, SoundGate::rank(SoundId::Adoption));
  TEST_ASSERT_EQUAL(2, SoundGate::rank(SoundId::ChoirLow));  // social-group/05c row 12
  TEST_ASSERT_EQUAL(2, SoundGate::rank(SoundId::ChoirMid));
  TEST_ASSERT_EQUAL(2, SoundGate::rank(SoundId::ChoirHigh));
  for (SoundId id : kAll) TEST_ASSERT_GREATER_THAN(0, SoundGate::durationMs(id));
}

// The V1 constant is a valid level of the four-step scale, below the maximum.
static void test_volume_is_a_level_below_max() {
  TEST_ASSERT_TRUE(SoundGate::kVolume < kMaxVolume);
}

// The duration check is correct across the 49.7-day wrap of the clock.
static void test_duration_survives_the_clock_wrap() {
  Rig r(0xFFFFFFFFu - 20);
  r.gate.play(SoundId::WakeMelody);
  r.run(SoundGate::durationMs(SoundId::WakeMelody) - kTick);
  TEST_ASSERT_TRUE(r.gate.playing());
  r.run(kTick);
  TEST_ASSERT_FALSE(r.gate.playing());
}

// social-group/05c: the three voices share one length and map from the rank, voice = rank mod 3.
static void test_choir_voices_share_one_length() {
  TEST_ASSERT_EQUAL_UINT32(SoundGate::durationMs(SoundId::ChoirMid), SoundGate::durationMs(SoundId::ChoirLow));
  TEST_ASSERT_EQUAL_UINT32(SoundGate::durationMs(SoundId::ChoirMid), SoundGate::durationMs(SoundId::ChoirHigh));
  TEST_ASSERT_EQUAL(SoundId::ChoirLow, cubbies::choirVoice(0));
  TEST_ASSERT_EQUAL(SoundId::ChoirMid, cubbies::choirVoice(1));
  TEST_ASSERT_EQUAL(SoundId::ChoirHigh, cubbies::choirVoice(2));
}

// 05c: play(id, rest) sends one leading rest then the notes, at kVolume, and stays playing for
// rest + duration.
static void test_lead_rest_precedes_the_notes() {
  Rig r;
  TEST_ASSERT_TRUE(r.gate.play(SoundId::ChoirMid, 463));
  TEST_ASSERT_EQUAL(1, r.audio.plays);
  TEST_ASSERT_EQUAL(SoundGate::kVolume, r.audio.lastLevel);
  TEST_ASSERT_EQUAL_UINT16(0, r.audio.lastNotes[0].hz);
  TEST_ASSERT_EQUAL_UINT16(463, r.audio.lastNotes[0].ms);
  TEST_ASSERT_EQUAL_UINT8(8, r.audio.lastCount);  // the rest + 7 notes
  TEST_ASSERT_EQUAL_UINT16(659, r.audio.lastNotes[1].hz);
  r.run(463 + SoundGate::durationMs(SoundId::ChoirMid) - kTick);
  TEST_ASSERT_TRUE(r.gate.playing());
  r.run(kTick);
  TEST_ASSERT_FALSE(r.gate.playing());
}

// 05c: a zero rest plays the plain note table; the rest is capped at kMaxRestMs.
static void test_zero_rest_is_plain_and_rest_is_capped() {
  Rig r;
  TEST_ASSERT_TRUE(r.gate.play(SoundId::ChoirLow, 0));
  TEST_ASSERT_EQUAL_UINT8(4, r.audio.lastCount);
  TEST_ASSERT_EQUAL_UINT16(262, r.audio.lastNotes[0].hz);
  Rig big;
  TEST_ASSERT_TRUE(big.gate.play(SoundId::ChoirLow, 5000));
  TEST_ASSERT_EQUAL_UINT16(SoundGate::kMaxRestMs, big.audio.lastNotes[0].ms);
}

// 05c rows 11, 12: a reaction during the rest is dropped; sleep stops the voice; never silent-gate
// bypass while dozing.
static void test_choir_voice_is_protected_and_stops_on_sleep() {
  Rig r;
  TEST_ASSERT_TRUE(r.gate.play(SoundId::ChoirHigh, 500));
  TEST_ASSERT_FALSE(r.gate.play(SoundId::Joy));
  TEST_ASSERT_FALSE(r.gate.play(SoundId::Grumpy));
  r.gate.setSleepState(SleepState::Dozing);
  TEST_ASSERT_EQUAL(1, r.audio.stops);
  TEST_ASSERT_FALSE(r.gate.play(SoundId::ChoirHigh, 500));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_boots_idle);
  RUN_TEST(test_plays_while_awake_or_waking_at_the_constant_volume);
  RUN_TEST(test_rejects_every_sound_while_dozing_or_asleep);
  RUN_TEST(test_higher_rank_replaces_the_sound_in_progress);
  RUN_TEST(test_equal_or_lower_rank_is_dropped);
  RUN_TEST(test_sound_ends_after_its_duration);
  RUN_TEST(test_entering_doze_or_sleep_stops_the_sound);
  RUN_TEST(test_rank_order_and_durations);
  RUN_TEST(test_choir_voices_share_one_length);
  RUN_TEST(test_lead_rest_precedes_the_notes);
  RUN_TEST(test_zero_rest_is_plain_and_rest_is_capped);
  RUN_TEST(test_choir_voice_is_protected_and_stops_on_sleep);
  RUN_TEST(test_volume_is_a_level_below_max);
  RUN_TEST(test_duration_survives_the_clock_wrap);
  return UNITY_END();
}
