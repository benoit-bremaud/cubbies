// Eyes renderer: the rules of FS1 / REQ-F-01 / REQ-NF-08 (one test per rule, ADR-0014) and
// golden images of every mood (characterisation tests). Regenerate goldens with
// CUBBIES_UPDATE_GOLDEN=1 from firmware/, review the PNG sheet in the pull request, then commit.
#include <unity.h>

#include <cstdio>
#include <string>

#include "FakeClock.h"
#include "Golden.h"
#include "RecordingDisplay.h"
#include "core/Mood.h"
#include "core/Prng.h"
#include "eyes/EyesRenderer.h"

using namespace cubbies;
using cubbies::test::FakeClock;
using cubbies::test::RecordingDisplay;

namespace {
constexpr uint32_t kTickMs = EyesRenderer::kTickMs;
const std::string kGoldenDir = "test/golden/";
const Rgb565 kCyan = rgb565(0, 200, 255);
// A frame is "closed" when fewer than 1/10 of the open pixels are lit: the half-open blink
// phase keeps about 1/2, the closed line about 1/30.
constexpr int kClosedDivisor = 10;

struct Rig {
  explicit Rig(uint64_t seed = 1234, uint32_t startMs = 0) : clock(startMs), prng(seed, 1), eyes(clock, prng, display) {}
  FakeClock clock;
  Prng prng;
  RecordingDisplay display;
  EyesRenderer eyes;
  void run(uint32_t ms) {
    for (uint32_t t = 0; t < ms; t += kTickMs) {
      clock.advance(kTickMs);
      eyes.tick();
    }
  }
};

int litPixels(const Framebuffer& fb) {
  int n = 0;
  for (int y = 0; y < Framebuffer::kHeight; ++y)
    for (int x = 0; x < Framebuffer::kWidth; ++x)
      if (fb.at(x, y).value != 0) ++n;
  return n;
}

bool framesEqual(const Framebuffer& a, const Framebuffer& b) {
  for (size_t i = 0; i < Framebuffer::kPixels; ++i)
    if (a.data()[i] != b.data()[i]) return false;
  return true;
}

// True when b is a shifted by (dx, dy) pixels (pixels shifted out are ignored).
bool shiftedBy(const Framebuffer& a, const Framebuffer& b, int dx, int dy) {
  for (int y = 0; y < Framebuffer::kHeight; ++y)
    for (int x = 0; x < Framebuffer::kWidth; ++x)
      if (x + dx >= 0 && x + dx < Framebuffer::kWidth && y + dy >= 0 && y + dy < Framebuffer::kHeight &&
          b.at(x + dx, y + dy).value != a.at(x, y).value)
        return false;
  return true;
}

void checkGolden(const char* name, const Framebuffer& fb) {
  const std::string raw = kGoldenDir + name + ".rgb565";
  const std::string png = kGoldenDir + name + ".png";
  if (cubbies::test::updateGoldenRequested()) {
    TEST_ASSERT_TRUE_MESSAGE(cubbies::test::writeRaw(raw, fb), raw.c_str());
    TEST_ASSERT_TRUE_MESSAGE(cubbies::test::writePng(png.c_str(), fb), png.c_str());
    TEST_MESSAGE("golden regenerated, not compared: review the PNG sheet in the PR");
    return;
  }
  const long diff = cubbies::test::compareRaw(raw, fb);
  if (diff < 0) TEST_FAIL_MESSAGE(("golden missing or wrong size, run from firmware/: " + raw).c_str());
  if (diff > 0) cubbies::test::writePng((kGoldenDir + name + ".actual.png").c_str(), fb);
  char msg[200];
  std::snprintf(msg, sizeof msg, "%s: %ld pixels differ, see %s.actual.png; regenerate only if the change is intended",
                name, diff, name);
  TEST_ASSERT_EQUAL_INT_MESSAGE(0, diff, msg);
}

Rig goldenRig(Mood mood) {
  Rig r;
  r.eyes.setColour(kCyan);
  r.eyes.setIdleGlances(false);
  r.eyes.setMood(mood);
  return r;
}
}  // namespace

void setUp() {}
void tearDown() {}

// REQ-F-01: two eyes, in the identity colour only.
static void test_first_tick_presents_two_eyes_in_the_identity_colour_only() {
  Rig r;
  r.eyes.setColour(kCyan);
  r.eyes.setIdleGlances(false);
  r.eyes.setMood(Mood::Curious);
  r.eyes.tick();
  TEST_ASSERT_EQUAL_UINT32(1, r.display.presentCount);
  int left = 0, right = 0;
  for (int y = 0; y < Framebuffer::kHeight; ++y)
    for (int x = 0; x < Framebuffer::kWidth; ++x) {
      const uint16_t v = r.display.last.at(x, y).value;
      TEST_ASSERT_TRUE(v == 0 || v == kCyan.value);
      if (v != 0) (x < Framebuffer::kWidth / 2 ? left : right)++;
    }
  // Each eye is a rounded kEyeW × kEyeH rectangle: at least 80 % of that area is lit.
  const int minPerEye = EyesRenderer::kEyeW * EyesRenderer::kEyeH * 8 / 10;
  TEST_ASSERT_GREATER_THAN_INT(minPerEye, left);
  TEST_ASSERT_GREATER_THAN_INT(minPerEye, right);
}

// FS1: eyes stay open 2–6 s between two blinks (measured from the reopening), for any seed.
static void checkBlinkGaps(uint64_t seed) {
  Rig r(seed);
  r.eyes.setColour(kCyan);
  r.eyes.setMood(Mood::Curious);
  r.eyes.tick();
  const int openLit = litPixels(r.display.last);
  uint32_t lastReopen = r.clock.nowMs();
  bool inBlink = false;
  int blinks = 0;
  for (uint32_t t = 0; t < 120000; t += kTickMs) {
    r.clock.advance(kTickMs);
    r.eyes.tick();
    const bool closedNow = litPixels(r.display.last) < openLit / kClosedDivisor;
    if (!closedNow && inBlink) lastReopen = r.clock.nowMs();
    if (closedNow && !inBlink) {
      const uint32_t gap = r.clock.nowMs() - lastReopen;
      // Measured between the first non-closed frame and the next closed frame: the scheduled
      // 2000..6000 ms plus up to three ticks (the trailing and leading half-open frames count as
      // open, and the blink starts on the first tick at or after its deadline).
      TEST_ASSERT_GREATER_OR_EQUAL_UINT32(EyesRenderer::kBlinkMinMs, gap);
      TEST_ASSERT_LESS_OR_EQUAL_UINT32(EyesRenderer::kBlinkMaxMs + 3 * kTickMs, gap);
      ++blinks;
    }
    inBlink = closedNow;
  }
  TEST_ASSERT_GREATER_OR_EQUAL_INT(18, blinks);
  TEST_ASSERT_LESS_OR_EQUAL_INT(60, blinks);
}
static void test_blinks_keep_eyes_open_2_to_6_seconds_for_any_seed() {
  for (uint64_t seed = 1; seed <= 32; ++seed) checkBlinkGaps(seed);
}

// FS1: a blink is short (closed for about two ticks) and the eyes reopen fully.
static void test_a_blink_closes_for_about_80_ms_then_reopens_fully() {
  Rig r;
  r.eyes.setColour(kCyan);
  r.eyes.setIdleGlances(false);
  r.eyes.setMood(Mood::Curious);
  r.eyes.tick();
  const int openLit = litPixels(r.display.last);
  r.eyes.blinkNow();
  uint32_t closedMs = 0;
  for (int i = 0; i < 8; ++i) {
    r.clock.advance(kTickMs);
    r.eyes.tick();
    if (litPixels(r.display.last) < openLit / kClosedDivisor) closedMs += kTickMs;
  }
  TEST_ASSERT_EQUAL_UINT32(EyesRenderer::kBlinkClosedMs, closedMs);
  TEST_ASSERT_EQUAL_INT(openLit, litPixels(r.display.last));
}

// FS1: never a static frame for more than 10 s while awake, even with idle glances disabled
// (the drift alone must carry the rule).
static void test_frame_never_stays_identical_for_more_than_10_seconds() {
  for (bool glances : {true, false}) {
    Rig r;
    r.eyes.setColour(kCyan);
    r.eyes.setIdleGlances(glances);
    r.eyes.setMood(Mood::Bored);  // the calmest mood
    r.eyes.tick();
    Framebuffer previous = r.display.last;
    uint32_t staticMs = 0;
    for (uint32_t t = 0; t < 180000; t += kTickMs) {
      r.clock.advance(kTickMs);
      r.eyes.tick();
      if (framesEqual(previous, r.display.last)) {
        staticMs += kTickMs;
        TEST_ASSERT_LESS_THAN_UINT32(10000, staticMs);
      } else {
        staticMs = 0;
        previous = r.display.last;
      }
    }
  }
}

// Sleep: closed eyes show only the thin lines, no blink while closed, reopening restores them.
static void test_closed_eyes_never_blink_and_open_eyes_restore() {
  Rig r;
  r.eyes.setColour(kCyan);
  r.eyes.setIdleGlances(false);
  r.eyes.setMood(Mood::Curious);
  r.eyes.tick();
  const int open = litPixels(r.display.last);
  r.eyes.closeEyes();
  r.run(EyesRenderer::kLidMoveMs + kTickMs);
  const int closed = litPixels(r.display.last);
  TEST_ASSERT_LESS_THAN_INT(open / kClosedDivisor, closed);
  for (uint32_t t = 0; t < 30000; t += kTickMs) {  // 30 s closed: the lit count never rises
    r.clock.advance(kTickMs);
    r.eyes.tick();
    TEST_ASSERT_LESS_OR_EQUAL_INT(closed, litPixels(r.display.last));
  }
  r.eyes.openEyes();
  r.run(EyesRenderer::kLidMoveMs + kTickMs);
  TEST_ASSERT_EQUAL_INT(open, litPixels(r.display.last));
}

// Waking never starts with a blink on top of the lid opening: the sleep-time deadline is stale.
static void test_no_blink_in_the_first_two_seconds_after_waking() {
  Rig r;
  r.eyes.setColour(kCyan);
  r.eyes.setIdleGlances(false);
  r.eyes.setMood(Mood::Curious);
  r.eyes.tick();
  const int open = litPixels(r.display.last);
  r.eyes.closeEyes();
  r.run(60000);
  r.eyes.openEyes();
  r.run(EyesRenderer::kLidMoveMs + kTickMs);
  for (uint32_t t = 0; t < EyesRenderer::kBlinkMinMs - EyesRenderer::kLidMoveMs - kTickMs; t += kTickMs) {
    r.clock.advance(kTickMs);
    r.eyes.tick();
    TEST_ASSERT_EQUAL_INT(open, litPixels(r.display.last));
  }
}

// A rig whose clock does not start at 0 renders the same first frame (no zero sentinel).
static void test_first_frame_does_not_depend_on_the_clock_start() {
  Rig a(1234, 0);
  Rig b(1234, 100);
  for (Rig* r : {&a, &b}) {
    r->eyes.setColour(kCyan);
    r->eyes.setIdleGlances(false);
    r->eyes.setMood(Mood::Curious);
    r->eyes.tick();
  }
  TEST_ASSERT_TRUE(framesEqual(a.display.last, b.display.last));
}

static void test_brightness_is_forwarded_to_the_display() {
  Rig r;
  r.eyes.setBrightness(Brightness::Low);
  TEST_ASSERT_EQUAL(Brightness::Low, r.display.brightness);
  r.eyes.setBrightness(Brightness::Minimum);
  TEST_ASSERT_EQUAL(Brightness::Minimum, r.display.brightness);
}

static void test_setLook_clamps_to_max_look() {
  Rig a, b;
  for (Rig* r : {&a, &b}) {
    r->eyes.setColour(kCyan);
    r->eyes.setIdleGlances(false);
  }
  a.eyes.setLook(100, -100);
  b.eyes.setLook(EyesRenderer::kMaxLook, -EyesRenderer::kMaxLook);
  a.eyes.tick();
  b.eyes.tick();
  TEST_ASSERT_TRUE(framesEqual(a.display.last, b.display.last));
}

// REQ-NF-08: the frame drifts one pixel per minute around a square, open or closed.
static void checkDrift(bool closed) {
  Rig r;
  r.eyes.setColour(kCyan);
  r.eyes.setIdleGlances(false);
  r.eyes.setMood(Mood::Bored);
  if (closed) {
    r.eyes.closeEyes();
    r.run(EyesRenderer::kLidMoveMs + kTickMs);
  }
  r.eyes.tick();
  const Framebuffer first = r.display.last;
  const int steps[4][2] = {{1, 0}, {1, 1}, {0, 1}, {0, 0}};
  for (const auto& s : steps) {
    r.run(EyesRenderer::kDriftPeriodMs);
    TEST_ASSERT_TRUE_MESSAGE(shiftedBy(first, r.display.last, s[0], s[1]), closed ? "closed" : "open");
  }
}
static void test_drift_moves_one_pixel_per_minute_around_a_square_open_and_closed() {
  checkDrift(false);
  checkDrift(true);
}

// IClock contract: blinks keep coming across the 49.7-day wrap.
static void test_blinks_survive_the_uint32_clock_wrap() {
  Rig r(1234, 0xFFFFFFFFu - 3000);
  r.eyes.setColour(kCyan);
  r.eyes.setMood(Mood::Curious);
  r.eyes.tick();
  const int openLit = litPixels(r.display.last);
  int blinks = 0;
  bool in = false;
  for (uint32_t t = 0; t < 30000; t += kTickMs) {
    r.clock.advance(kTickMs);
    r.eyes.tick();
    const bool c = litPixels(r.display.last) < openLit / kClosedDivisor;
    if (c && !in) ++blinks;
    in = c;
  }
  TEST_ASSERT_GREATER_OR_EQUAL_INT(4, blinks);
}

// Golden images, one test per state so one broken mood does not hide the others.
static void test_golden_curious() { Rig r = goldenRig(Mood::Curious); r.eyes.tick(); checkGolden("curious", r.display.last); }
static void test_golden_happy() { Rig r = goldenRig(Mood::Happy); r.eyes.tick(); checkGolden("happy", r.display.last); }
static void test_golden_bored() { Rig r = goldenRig(Mood::Bored); r.eyes.tick(); checkGolden("bored", r.display.last); }
static void test_golden_tired() { Rig r = goldenRig(Mood::Tired); r.eyes.tick(); checkGolden("tired", r.display.last); }
static void test_golden_annoyed() { Rig r = goldenRig(Mood::Annoyed); r.eyes.tick(); checkGolden("annoyed", r.display.last); }
static void test_golden_closed() {
  Rig r = goldenRig(Mood::Tired);
  r.eyes.closeEyes();
  r.run(EyesRenderer::kLidMoveMs + kTickMs);
  checkGolden("closed", r.display.last);
}
static void test_golden_blink_sequence() {
  Rig r = goldenRig(Mood::Curious);
  r.eyes.tick();
  r.eyes.blinkNow();
  const char* names[] = {"blink-1", "blink-2", "blink-3", "blink-4"};
  for (const char* n : names) {  // 40 ms: half, 80: closed, 120: closed, 160: half
    r.clock.advance(kTickMs);
    r.eyes.tick();
    checkGolden(n, r.display.last);
  }
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_first_tick_presents_two_eyes_in_the_identity_colour_only);
  RUN_TEST(test_blinks_keep_eyes_open_2_to_6_seconds_for_any_seed);
  RUN_TEST(test_a_blink_closes_for_about_80_ms_then_reopens_fully);
  RUN_TEST(test_frame_never_stays_identical_for_more_than_10_seconds);
  RUN_TEST(test_closed_eyes_never_blink_and_open_eyes_restore);
  RUN_TEST(test_no_blink_in_the_first_two_seconds_after_waking);
  RUN_TEST(test_first_frame_does_not_depend_on_the_clock_start);
  RUN_TEST(test_brightness_is_forwarded_to_the_display);
  RUN_TEST(test_setLook_clamps_to_max_look);
  RUN_TEST(test_drift_moves_one_pixel_per_minute_around_a_square_open_and_closed);
  RUN_TEST(test_blinks_survive_the_uint32_clock_wrap);
  RUN_TEST(test_golden_curious);
  RUN_TEST(test_golden_happy);
  RUN_TEST(test_golden_bored);
  RUN_TEST(test_golden_tired);
  RUN_TEST(test_golden_annoyed);
  RUN_TEST(test_golden_closed);
  RUN_TEST(test_golden_blink_sequence);
  return UNITY_END();
}
