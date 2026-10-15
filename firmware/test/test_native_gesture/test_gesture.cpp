// Gesture classifier: one test per row of the 05c transition table, plus the wrap.
#include <unity.h>

#include "FakeClock.h"
#include "ScriptedTouch.h"
#include "core/GestureClassifier.h"

using cubbies::Gesture;
using cubbies::GestureClassifier;
using cubbies::test::FakeClock;
using cubbies::test::ScriptedTouch;

namespace {
constexpr uint32_t kTick = 40;

struct Rig {
  explicit Rig(uint32_t startMs = 1000) : clock(startMs), gestures(clock, touch) {}
  FakeClock clock;
  ScriptedTouch touch;
  GestureClassifier gestures;

  Gesture step() {  // one tick
    clock.advance(kTick);
    return gestures.tick();
  }
  // Ticks for `ms` with the pad at `level`; returns the only non-None gesture seen, or None.
  // Fails if more than one gesture shows up.
  Gesture hold(bool level, uint32_t ms) {
    touch.set(level);
    Gesture seen = Gesture::None;
    for (uint32_t t = 0; t < ms; t += kTick) {
      const Gesture g = step();
      if (g == Gesture::None) continue;
      TEST_ASSERT_EQUAL_MESSAGE(Gesture::None, seen, "more than one gesture");
      seen = g;
    }
    return seen;
  }
  void idle() { TEST_ASSERT_EQUAL(Gesture::None, hold(false, 2000)); }
};
}  // namespace

void setUp() {}
void tearDown() {}

// Idle -> Pressed: TouchDown on the tick that sees the pressed edge (REQ-NF-01).
static void test_pressed_edge_emits_touchdown_on_the_same_tick() {
  Rig r;
  r.touch.set(true);
  TEST_ASSERT_EQUAL(Gesture::TouchDown, r.step());
  TEST_ASSERT_EQUAL(Gesture::None, r.step());  // once per press
}

// Pressed -> WaitSecond -> Idle: a short press is a Tap once T_doubletap elapsed, no Released.
static void test_short_press_is_a_tap_after_the_double_tap_window() {
  Rig r;
  TEST_ASSERT_EQUAL(Gesture::TouchDown, r.hold(true, 120));
  r.touch.set(false);
  TEST_ASSERT_EQUAL(Gesture::None, r.step());  // release tick: touch-down stays held
  const uint32_t releasedAt = r.clock.nowMs();
  Gesture g = Gesture::None;
  while (g == Gesture::None) g = r.step();
  TEST_ASSERT_EQUAL(Gesture::Tap, g);
  // First tick at or past the window, not earlier.
  const uint32_t age = r.clock.nowMs() - releasedAt;
  TEST_ASSERT_TRUE(age >= GestureClassifier::kDoubleTapMs && age < GestureClassifier::kDoubleTapMs + kTick);
  r.idle();
}

// Pressed -> Idle: released after T_tapMax (but before T_stroke) only ends the touch-down.
static void test_medium_press_emits_released_and_no_tap() {
  Rig r;
  TEST_ASSERT_EQUAL(Gesture::TouchDown, r.hold(true, 800));
  r.touch.set(false);
  TEST_ASSERT_EQUAL(Gesture::Released, r.step());  // on the release tick
  r.idle();                                        // and never a Tap afterwards
}

// Pressed -> Stroking: Stroke fires while the finger is still down, at T_stroke.
static void test_long_contact_emits_stroke_at_t_stroke_while_pressed() {
  Rig r;
  r.touch.set(true);
  TEST_ASSERT_EQUAL(Gesture::TouchDown, r.step());
  const uint32_t pressedAt = r.clock.nowMs();
  Gesture g = Gesture::None;
  while (g == Gesture::None && r.clock.nowMs() - pressedAt < 3000) g = r.step();
  TEST_ASSERT_EQUAL(Gesture::Stroke, g);
  const uint32_t age = r.clock.nowMs() - pressedAt;
  TEST_ASSERT_TRUE(age >= GestureClassifier::kStrokeMs && age < GestureClassifier::kStrokeMs + kTick);
}

// Stroking -> Idle: a single Released, however long the contact.
static void test_stroke_ends_with_one_released_however_long() {
  Rig r;
  r.touch.set(true);
  TEST_ASSERT_EQUAL(Gesture::TouchDown, r.step());
  TEST_ASSERT_EQUAL(Gesture::Stroke, r.hold(true, 10000));  // one Stroke over 10 s
  TEST_ASSERT_EQUAL(Gesture::Released, r.hold(false, 1000));
  r.idle();
}

// WaitSecond -> Pressed2 -> Idle: second short press emits TouchDown, then DoubleTap at release.
static void test_two_short_presses_are_a_double_tap_without_waiting() {
  Rig r;
  r.hold(true, 120);
  TEST_ASSERT_EQUAL(Gesture::None, r.hold(false, 120));
  r.touch.set(true);
  TEST_ASSERT_EQUAL(Gesture::TouchDown, r.step());
  r.hold(true, 80);
  r.touch.set(false);
  TEST_ASSERT_EQUAL(Gesture::DoubleTap, r.step());
  r.idle();
}

// Pressed2 -> Idle: a long second press is no double tap, only Released.
static void test_short_then_medium_press_is_no_double_tap() {
  Rig r;
  r.hold(true, 120);
  r.hold(false, 120);
  TEST_ASSERT_EQUAL(Gesture::TouchDown, r.hold(true, 800));
  TEST_ASSERT_EQUAL(Gesture::Released, r.hold(false, 1000));
  r.idle();
}

// Pressed2 -> Stroking: a tap then a long contact is a Stroke; the first tap is absorbed.
static void test_tap_then_long_contact_is_a_stroke() {
  Rig r;
  r.hold(true, 120);
  r.hold(false, 120);
  r.touch.set(true);
  TEST_ASSERT_EQUAL(Gesture::TouchDown, r.step());
  TEST_ASSERT_EQUAL(Gesture::Stroke, r.hold(true, 2000));
  TEST_ASSERT_EQUAL(Gesture::Released, r.hold(false, 1000));
}

// A third quick tap after a double tap starts a new gesture.
static void test_third_tap_starts_a_new_gesture() {
  Rig r;
  r.hold(true, 80);
  r.hold(false, 80);
  r.hold(true, 80);
  TEST_ASSERT_EQUAL(Gesture::DoubleTap, r.hold(false, 80));
  r.touch.set(true);
  TEST_ASSERT_EQUAL(Gesture::TouchDown, r.step());
  r.hold(true, 80);
  TEST_ASSERT_EQUAL(Gesture::Tap, r.hold(false, 1000));
}

// Durations are ages on the uint32 clock: a tap across the wrap is still a tap.
static void test_tap_and_stroke_survive_the_clock_wrap() {
  Rig r(0xFFFFFFFFu - 100);
  TEST_ASSERT_EQUAL(Gesture::TouchDown, r.hold(true, 120));
  TEST_ASSERT_EQUAL(Gesture::Tap, r.hold(false, 1000));
  Rig s(0xFFFFFFFFu - 500);
  TEST_ASSERT_EQUAL(Gesture::TouchDown, s.hold(true, kTick));
  TEST_ASSERT_EQUAL(Gesture::Stroke, s.hold(true, 2000));
}

// holdMs(): age of the current press, 0 when released (identity/05a reads it).
static void test_hold_ms_reports_the_age_of_the_current_press() {
  Rig r;
  TEST_ASSERT_EQUAL_UINT32(0, r.gestures.holdMs());
  r.hold(true, 1000);
  TEST_ASSERT_EQUAL_UINT32(1000 - kTick, r.gestures.holdMs());
  r.hold(true, 1000);  // past T_stroke, still counting
  TEST_ASSERT_EQUAL_UINT32(2000 - kTick, r.gestures.holdMs());
  r.hold(false, kTick);
  TEST_ASSERT_EQUAL_UINT32(0, r.gestures.holdMs());
}

// any -> Consumed: consumeCurrentPress(); holdMs() reads 0, nothing is emitted while held.
static void test_consumed_press_emits_nothing_while_held() {
  Rig r;
  r.hold(true, 200);
  r.gestures.consumeCurrentPress();
  TEST_ASSERT_EQUAL_UINT32(0, r.gestures.holdMs());
  TEST_ASSERT_EQUAL(Gesture::None, r.hold(true, 3000));  // no Stroke
}

// Consumed -> Idle: the release emits nothing, and the next press is a fresh gesture.
static void test_consumed_press_release_is_silent_then_idle() {
  Rig r;
  r.touch.set(true);
  for (uint32_t t = 0; t < 2000; t += kTick) r.step();  // TouchDown then Stroke
  r.gestures.consumeCurrentPress();
  TEST_ASSERT_EQUAL(Gesture::None, r.hold(false, 1000));  // no Released, no Tap
  r.touch.set(true);
  TEST_ASSERT_EQUAL(Gesture::TouchDown, r.step());
}

// Pressed2 -> Consumed: the second press of a double tap can be consumed too.
static void test_second_press_can_be_consumed() {
  Rig r;
  r.hold(true, 80);
  r.hold(false, 80);
  r.touch.set(true);
  TEST_ASSERT_EQUAL(Gesture::TouchDown, r.step());  // Pressed2
  r.gestures.consumeCurrentPress();
  TEST_ASSERT_EQUAL(Gesture::None, r.hold(true, 2000));
  TEST_ASSERT_EQUAL(Gesture::None, r.hold(false, 1000));  // no DoubleTap, no Released
}

// consumeCurrentPress() with the pad released: the pending Tap is dropped, Idle at once.
static void test_consume_while_released_drops_the_pending_tap() {
  Rig r;
  r.hold(true, 80);
  r.touch.set(false);
  r.step();  // WaitSecond, Tap pending
  r.gestures.consumeCurrentPress();
  TEST_ASSERT_EQUAL(Gesture::None, r.hold(false, 1000));
  r.touch.set(true);
  TEST_ASSERT_EQUAL(Gesture::TouchDown, r.step());  // the very next press is fresh
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_pressed_edge_emits_touchdown_on_the_same_tick);
  RUN_TEST(test_short_press_is_a_tap_after_the_double_tap_window);
  RUN_TEST(test_medium_press_emits_released_and_no_tap);
  RUN_TEST(test_long_contact_emits_stroke_at_t_stroke_while_pressed);
  RUN_TEST(test_stroke_ends_with_one_released_however_long);
  RUN_TEST(test_two_short_presses_are_a_double_tap_without_waiting);
  RUN_TEST(test_short_then_medium_press_is_no_double_tap);
  RUN_TEST(test_tap_then_long_contact_is_a_stroke);
  RUN_TEST(test_third_tap_starts_a_new_gesture);
  RUN_TEST(test_tap_and_stroke_survive_the_clock_wrap);
  RUN_TEST(test_hold_ms_reports_the_age_of_the_current_press);
  RUN_TEST(test_consumed_press_emits_nothing_while_held);
  RUN_TEST(test_consumed_press_release_is_silent_then_idle);
  RUN_TEST(test_second_press_can_be_consumed);
  RUN_TEST(test_consume_while_released_drops_the_pending_tap);
  return UNITY_END();
}
