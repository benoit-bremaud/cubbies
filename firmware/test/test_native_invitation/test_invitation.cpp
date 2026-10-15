// Invitation window: one test per row of the identity/05a transition table, plus the wrap.
#include <unity.h>

#include "FakeClock.h"
#include "ScriptedTouch.h"
#include "core/InvitationWindow.h"

using cubbies::GestureClassifier;
using cubbies::IdentityFrame;
using cubbies::IdentityOutput;
using cubbies::InvitationWindow;
using cubbies::test::FakeClock;
using cubbies::test::ScriptedTouch;

namespace {
constexpr uint32_t kTick = 40;

struct Rig {
  explicit Rig(uint32_t startMs = 1000) : clock(startMs), gestures(clock, touch), window(clock, gestures) {
    window.start();
  }
  FakeClock clock;
  ScriptedTouch touch;
  GestureClassifier gestures;
  InvitationWindow window;
  IdentityOutput last;

  IdentityOutput step() {
    clock.advance(kTick);
    last = window.tick();
    return last;
  }
  // Ticks for `ms` with the pad at `level`; returns the last output that was not Keep.
  IdentityOutput run(bool level, uint32_t ms) {
    touch.set(level);
    IdentityOutput seen;
    for (uint32_t t = 0; t < ms; t += kTick) {
      const IdentityOutput o = step();
      if (o.frame != IdentityFrame::Keep) seen = o;
    }
    return seen;
  }
  void settle() { run(false, InvitationWindow::kSettleMs); }
};
}  // namespace

void setUp() {}
void tearDown() {}

// — -> Settling, Settling tick: nothing is shown and a touch is never read.
static void test_settling_shows_nothing_and_ignores_touch() {
  Rig r;
  const IdentityOutput o = r.run(true, InvitationWindow::kSettleMs - kTick);
  TEST_ASSERT_EQUAL(IdentityFrame::Keep, o.frame);
  TEST_ASSERT_FALSE(r.window.entered());
  TEST_ASSERT_FALSE(r.window.declined());
}

// Settling -> Inviting, pad released: empty gauge.
static void test_settle_end_shows_an_empty_invitation() {
  Rig r;
  const IdentityOutput o = r.run(false, InvitationWindow::kSettleMs);
  TEST_ASSERT_EQUAL(IdentityFrame::Invitation, o.frame);
  TEST_ASSERT_EQUAL_UINT8(0, o.progress);
  TEST_ASSERT_FALSE(o.hasSound);
}

// Settling -> Inviting, pad already pressed: consumed, a long hold does not enter.
static void test_press_down_at_settle_end_is_consumed() {
  Rig r;
  r.run(true, InvitationWindow::kSettleMs);
  TEST_ASSERT_EQUAL(IdentityFrame::Invitation, r.last.frame);
  r.run(true, InvitationWindow::kSecretMs + 200);
  TEST_ASSERT_FALSE(r.window.entered());
  TEST_ASSERT_EQUAL_UINT32(0, r.gestures.holdMs());
}

// Inviting -> Holding: a press fills the gauge.
static void test_press_starts_the_gauge() {
  Rig r;
  r.settle();
  r.touch.set(true);
  r.step();  // pressed edge
  const IdentityOutput o = r.step();
  TEST_ASSERT_EQUAL(IdentityFrame::Invitation, o.frame);
  TEST_ASSERT_EQUAL_UINT8(kTick * 255u / InvitationWindow::kSecretMs, o.progress);
}

// Holding -> Holding: progress = hold × 255 / T_secret, rising.
static void test_gauge_follows_the_hold() {
  Rig r;
  r.settle();
  const IdentityOutput o = r.run(true, 1000 + kTick);  // edge tick + 1000 ms of hold
  TEST_ASSERT_EQUAL(IdentityFrame::Invitation, o.frame);
  TEST_ASSERT_EQUAL_UINT8(1000u * 255u / InvitationWindow::kSecretMs, o.progress);
}

// Holding -> Inviting: released inside T_invite empties the gauge.
static void test_release_inside_window_empties_the_gauge() {
  Rig r;
  r.settle();
  r.run(true, 600);
  r.touch.set(false);
  const IdentityOutput o = r.step();
  TEST_ASSERT_EQUAL(IdentityFrame::Invitation, o.frame);
  TEST_ASSERT_EQUAL_UINT8(0, o.progress);
  TEST_ASSERT_FALSE(r.window.declined());
}

// Holding -> Declined: released after T_invite clears the frame.
static void test_release_after_window_declines() {
  Rig r;
  r.settle();
  r.run(false, InvitationWindow::kInviteMs - 1000);
  r.run(true, 1500);  // window elapses mid-hold, hold not yet T_secret
  TEST_ASSERT_FALSE(r.window.declined());
  r.touch.set(false);
  TEST_ASSERT_EQUAL(IdentityFrame::Clear, r.step().frame);
  TEST_ASSERT_TRUE(r.window.declined());
}

// Holding -> Entered: hold reaches T_secret, even past T_invite; the press is consumed.
static void test_hold_reaching_secret_enters_even_past_the_window() {
  Rig r;
  r.settle();
  r.run(false, InvitationWindow::kInviteMs - 1000);
  r.run(true, InvitationWindow::kSecretMs);  // edge tick + T_secret - tick of hold
  TEST_ASSERT_FALSE(r.window.entered());
  TEST_ASSERT_EQUAL(IdentityFrame::Keep, r.step().frame);  // hold == T_secret
  TEST_ASSERT_TRUE(r.window.entered());
  TEST_ASSERT_EQUAL_UINT32(0, r.gestures.holdMs());  // consumed
}

// Inviting -> Declined: T_invite elapsed without a press.
static void test_window_elapsed_declines() {
  Rig r;
  r.settle();
  r.run(false, InvitationWindow::kInviteMs - kTick);
  TEST_ASSERT_FALSE(r.window.declined());
  const IdentityOutput o = r.step();
  TEST_ASSERT_EQUAL(IdentityFrame::Clear, o.frame);
  TEST_ASSERT_TRUE(r.window.declined());
  TEST_ASSERT_FALSE(r.window.entered());
}

// Inviting -> Holding: a press whose edge is the last tick inside T_invite still counts.
static void test_press_on_the_last_tick_of_the_window_counts() {
  Rig r;
  r.settle();
  r.run(false, InvitationWindow::kInviteMs - 2 * kTick);  // next tick is the last inside
  r.touch.set(true);
  r.step();  // pressed edge, holdMs() == 0
  const IdentityOutput o = r.step();  // past T_invite, held 40 ms
  TEST_ASSERT_EQUAL(IdentityFrame::Invitation, o.frame);
  TEST_ASSERT_FALSE(r.window.declined());
}

// Inviting -> Holding with a hold already past T_secret (late tick): gauge clamped, then enters.
static void test_late_tick_clamps_the_gauge() {
  Rig r;
  r.settle();
  r.touch.set(true);
  r.step();  // pressed edge
  r.clock.advance(InvitationWindow::kSecretMs + 500);
  const IdentityOutput o = r.window.tick();
  TEST_ASSERT_EQUAL(IdentityFrame::Invitation, o.frame);
  TEST_ASSERT_EQUAL_UINT8(255, o.progress);
  r.step();
  TEST_ASSERT_TRUE(r.window.entered());
}

// After the window, the classifier is handed back clean: no gesture left over (PR #20 review).
static void assertNoLeftoverGesture(Rig& r) {
  r.touch.set(false);
  for (uint32_t t = 0; t < 600; t += kTick) {
    r.clock.advance(kTick);
    TEST_ASSERT_EQUAL(cubbies::Gesture::None, r.gestures.tick());
  }
}

// Inviting -> Declined: a tap released just before the expiry does not leak a Tap.
static void test_pending_tap_is_dropped_when_the_window_closes() {
  Rig r;
  r.settle();
  r.run(false, InvitationWindow::kInviteMs - 200);
  r.run(true, kTick);   // short press
  r.run(false, 200);    // release; the window expires while the classifier waits
  TEST_ASSERT_TRUE(r.window.declined());
  assertNoLeftoverGesture(r);
}

// Holding -> Declined: a short press released after T_invite does not leak a Tap.
static void test_short_press_released_after_the_window_leaks_nothing() {
  Rig r;
  r.settle();
  r.run(false, InvitationWindow::kInviteMs - 200);
  r.run(true, 280);
  r.touch.set(false);
  r.step();
  TEST_ASSERT_TRUE(r.window.declined());
  assertNoLeftoverGesture(r);
}

// Settling -> Inviting: a press whose edge is the settle-end tick is consumed too.
static void test_press_on_the_settle_end_tick_is_consumed() {
  Rig r;
  r.run(false, InvitationWindow::kSettleMs - kTick);
  r.touch.set(true);
  TEST_ASSERT_EQUAL(IdentityFrame::Invitation, r.step().frame);  // settle end + pressed edge
  r.run(true, InvitationWindow::kSecretMs + 200);
  TEST_ASSERT_FALSE(r.window.entered());
}

// Ages survive the uint32 clock wrap.
static void test_window_survives_the_clock_wrap() {
  Rig r(0xFFFFFFFFu - 1500);
  r.settle();
  r.run(true, InvitationWindow::kSecretMs + kTick);
  TEST_ASSERT_TRUE(r.window.entered());
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_settling_shows_nothing_and_ignores_touch);
  RUN_TEST(test_settle_end_shows_an_empty_invitation);
  RUN_TEST(test_press_down_at_settle_end_is_consumed);
  RUN_TEST(test_press_starts_the_gauge);
  RUN_TEST(test_gauge_follows_the_hold);
  RUN_TEST(test_release_inside_window_empties_the_gauge);
  RUN_TEST(test_release_after_window_declines);
  RUN_TEST(test_hold_reaching_secret_enters_even_past_the_window);
  RUN_TEST(test_window_elapsed_declines);
  RUN_TEST(test_press_on_the_last_tick_of_the_window_counts);
  RUN_TEST(test_late_tick_clamps_the_gauge);
  RUN_TEST(test_pending_tap_is_dropped_when_the_window_closes);
  RUN_TEST(test_short_press_released_after_the_window_leaks_nothing);
  RUN_TEST(test_press_on_the_settle_end_tick_is_consumed);
  RUN_TEST(test_window_survives_the_clock_wrap);
  return UNITY_END();
}
