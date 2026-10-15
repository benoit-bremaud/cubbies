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
                        SoundId::Yawn,     SoundId::Grumpy, SoundId::WakeMelody};
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

// Ranks: wake-up melody > grumpy > yawn > surprise, joy, purr, hello; every sound has notes.
static void test_rank_order_and_durations() {
  TEST_ASSERT_EQUAL(0, SoundGate::rank(SoundId::Surprise));
  TEST_ASSERT_EQUAL(0, SoundGate::rank(SoundId::Joy));
  TEST_ASSERT_EQUAL(0, SoundGate::rank(SoundId::Purr));
  TEST_ASSERT_EQUAL(0, SoundGate::rank(SoundId::Hello));
  TEST_ASSERT_EQUAL(1, SoundGate::rank(SoundId::Yawn));
  TEST_ASSERT_EQUAL(2, SoundGate::rank(SoundId::Grumpy));
  TEST_ASSERT_EQUAL(3, SoundGate::rank(SoundId::WakeMelody));
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
  RUN_TEST(test_volume_is_a_level_below_max);
  RUN_TEST(test_duration_survives_the_clock_wrap);
  return UNITY_END();
}
