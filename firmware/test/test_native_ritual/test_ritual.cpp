// Adoption ritual: one test per row of the identity/05b transition table, plus the wrap.
#include <unity.h>

#include "FakeClock.h"
#include "FakeStorage.h"
#include "core/AdoptionRitual.h"

using cubbies::AdoptionRitual;
using cubbies::Gesture;
using cubbies::Identity;
using cubbies::IdentityFrame;
using cubbies::IdentityOutput;
using cubbies::kColourKey;
using cubbies::paletteColour;
using cubbies::SoundId;
using cubbies::test::FakeClock;
using cubbies::test::FakeStorage;

namespace {
constexpr uint32_t kTick = 40;
constexpr Identity kDefault{{0x24, 0x0A, 0xC4, 0x12, 0x34, 0x56}, 6, false};  // placeholder MAC

struct Rig {
  explicit Rig(uint32_t startMs = 1000) : clock(startMs), ritual(clock, storage) {}
  FakeClock clock;
  FakeStorage storage;
  AdoptionRitual ritual;

  IdentityOutput step(Gesture g = Gesture::None) {
    clock.advance(kTick);
    return ritual.tick(g);
  }
  void wait(uint32_t ms) {
    for (uint32_t t = 0; t < ms; t += kTick) step();
  }
};

void assertRitualFrame(const IdentityOutput& o, uint8_t index, SoundId sound) {
  TEST_ASSERT_EQUAL(IdentityFrame::Ritual, o.frame);
  TEST_ASSERT_EQUAL_UINT16(paletteColour(index).value, o.colour.value);
  TEST_ASSERT_TRUE(o.hasSound);
  TEST_ASSERT_EQUAL(sound, o.sound);
}
}  // namespace

void setUp() {}
void tearDown() {}

// — -> Cycling: the current colour is the first candidate, with the chime.
static void test_start_shows_the_current_colour_with_the_chime() {
  Rig r;
  assertRitualFrame(r.ritual.start(kDefault), 6, SoundId::RitualChime);
  TEST_ASSERT_FALSE(r.ritual.isDone());
}

// Cycling -> Cycling on Tap: next colour, wrapping at 8.
static void test_tap_moves_forward_and_wraps() {
  Rig r;
  r.ritual.start(kDefault);
  assertRitualFrame(r.step(Gesture::Tap), 7, SoundId::RitualTick);
  assertRitualFrame(r.step(Gesture::Tap), 0, SoundId::RitualTick);
}

// Cycling -> Cycling on DoubleTap: forward by two (two quick taps both count).
static void test_double_tap_moves_forward_by_two() {
  Rig r;
  r.ritual.start(kDefault);
  assertRitualFrame(r.step(Gesture::DoubleTap), 0, SoundId::RitualTick);
}

// Cycling -> Cycling on TouchDown or Released: nothing shown, but the timeout restarts.
static void test_touchdown_and_release_only_restart_the_timeout() {
  Rig r;
  r.ritual.start(kDefault);
  r.wait(AdoptionRitual::kTimeoutMs - 1000);
  TEST_ASSERT_EQUAL(IdentityFrame::Keep, r.step(Gesture::TouchDown).frame);
  r.wait(AdoptionRitual::kTimeoutMs - 1000);
  TEST_ASSERT_EQUAL(IdentityFrame::Keep, r.step(Gesture::Released).frame);
  r.wait(AdoptionRitual::kTimeoutMs - 1000);
  TEST_ASSERT_FALSE(r.ritual.isDone());
}

// Cycling -> Done on Stroke, write ok: one uint8 written, adopted, adoption frame and melody.
static void test_stroke_confirms_and_writes_once() {
  Rig r;
  r.ritual.start(kDefault);
  r.step(Gesture::Tap);
  const IdentityOutput o = r.step(Gesture::Stroke);
  TEST_ASSERT_EQUAL(IdentityFrame::Adoption, o.frame);
  TEST_ASSERT_EQUAL_UINT16(paletteColour(7).value, o.colour.value);
  TEST_ASSERT_TRUE(o.hasSound);
  TEST_ASSERT_EQUAL(SoundId::Adoption, o.sound);
  TEST_ASSERT_TRUE(r.ritual.isDone());
  TEST_ASSERT_EQUAL(1, r.storage.writes);
  TEST_ASSERT_EQUAL_UINT8(7, r.storage.values[kColourKey]);
  TEST_ASSERT_EQUAL_UINT8(7, r.ritual.identity().paletteIndex);
  TEST_ASSERT_TRUE(r.ritual.identity().adopted);
}

// Cycling -> Done on Stroke, write failed: fail closed, identity unchanged, error frame.
static void test_failed_write_keeps_the_identity_and_shows_error() {
  Rig r;
  r.storage.failWrites = true;
  r.ritual.start(kDefault);
  r.step(Gesture::Tap);
  const IdentityOutput o = r.step(Gesture::Stroke);
  TEST_ASSERT_EQUAL(IdentityFrame::Error, o.frame);
  TEST_ASSERT_FALSE(o.hasSound);
  TEST_ASSERT_TRUE(r.ritual.isDone());
  TEST_ASSERT_EQUAL_UINT8(6, r.ritual.identity().paletteIndex);
  TEST_ASSERT_FALSE(r.ritual.identity().adopted);
}

// Cycling -> Done on timeout: identity unchanged, nothing written, frame cleared.
static void test_timeout_abandons_without_writing() {
  Rig r;
  r.ritual.start(kDefault);
  r.wait(30000);
  r.step(Gesture::Tap);  // restarts the timeout
  r.wait(AdoptionRitual::kTimeoutMs - kTick);
  TEST_ASSERT_FALSE(r.ritual.isDone());
  const IdentityOutput last = r.step();  // T_ritual_timeout after the tap
  TEST_ASSERT_TRUE(r.ritual.isDone());
  TEST_ASSERT_EQUAL(IdentityFrame::Clear, last.frame);
  TEST_ASSERT_EQUAL(0, r.storage.writes);
  TEST_ASSERT_EQUAL_UINT8(6, r.ritual.identity().paletteIndex);
  TEST_ASSERT_FALSE(r.ritual.identity().adopted);
}

// Done -> Done: later ticks do nothing, a later stroke writes nothing.
static void test_done_ignores_further_gestures() {
  Rig r;
  r.ritual.start(kDefault);
  r.step(Gesture::Stroke);
  TEST_ASSERT_EQUAL(IdentityFrame::Keep, r.step(Gesture::Tap).frame);
  TEST_ASSERT_EQUAL(IdentityFrame::Keep, r.step(Gesture::Stroke).frame);
  TEST_ASSERT_EQUAL(1, r.storage.writes);
}

// The timeout is an age: correct across the uint32 wrap.
static void test_timeout_survives_the_clock_wrap() {
  Rig r(0xFFFFFFFFu - 1000);
  r.ritual.start(kDefault);
  r.wait(AdoptionRitual::kTimeoutMs - 1000);
  TEST_ASSERT_FALSE(r.ritual.isDone());
  r.wait(1000 + kTick);
  TEST_ASSERT_TRUE(r.ritual.isDone());
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_start_shows_the_current_colour_with_the_chime);
  RUN_TEST(test_tap_moves_forward_and_wraps);
  RUN_TEST(test_double_tap_moves_forward_by_two);
  RUN_TEST(test_touchdown_and_release_only_restart_the_timeout);
  RUN_TEST(test_stroke_confirms_and_writes_once);
  RUN_TEST(test_failed_write_keeps_the_identity_and_shows_error);
  RUN_TEST(test_timeout_abandons_without_writing);
  RUN_TEST(test_done_ignores_further_gestures);
  RUN_TEST(test_timeout_survives_the_clock_wrap);
  return UNITY_END();
}
