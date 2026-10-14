// Eyes renderer: timing rules of FS1 / REQ-F-01 / REQ-NF-08 and golden images of every mood.
// Golden images are characterisation tests: regenerate with CUBBIES_UPDATE_GOLDEN=1 and review
// the PNGs in the pull request before committing them.
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <unity.h>

#include <cstdio>
#include <string>

#include "FakeClock.h"
#include "Png.h"
#include "RecordingDisplay.h"
#include "core/Mood.h"
#include "core/Prng.h"
#include "eyes/EyesRenderer.h"

using namespace cubbies;
using cubbies::test::FakeClock;
using cubbies::test::RecordingDisplay;

namespace {
constexpr uint32_t kTickMs = 40;
const std::string kGoldenDir = "test/golden/";

struct Rig {
  FakeClock clock;
  Prng prng{1234, 1};
  RecordingDisplay display;
  EyesRenderer eyes{clock, prng, display};
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

void checkGolden(const char* name, const Framebuffer& fb) {
  const std::string path = kGoldenDir + name + ".png";
  if (cubbies::test::updateGoldenRequested()) {
    TEST_ASSERT_TRUE_MESSAGE(cubbies::test::writePng(path, fb), path.c_str());
    return;
  }
  const long diff = cubbies::test::comparePng(path, fb);
  char msg[160];
  std::snprintf(msg, sizeof msg, "%s: %ld pixels differ (run with CUBBIES_UPDATE_GOLDEN=1 to regenerate)", name, diff);
  TEST_ASSERT_EQUAL_INT_MESSAGE(0, diff, msg);
}
}  // namespace

void setUp() {}
void tearDown() {}

static void test_first_tick_presents_a_frame_with_two_eyes_in_the_identity_colour() {
  Rig r;
  r.eyes.setColour(rgb565(0, 200, 255));
  r.eyes.setMood(Mood::Curious);
  r.eyes.tick();
  TEST_ASSERT_EQUAL_UINT32(1, r.display.presentCount);
  TEST_ASSERT_GREATER_THAN_INT(400, litPixels(r.display.last));
  // left and right halves both lit: two eyes
  int left = 0, right = 0;
  for (int y = 0; y < Framebuffer::kHeight; ++y)
    for (int x = 0; x < Framebuffer::kWidth; ++x)
      if (r.display.last.at(x, y).value != 0) (x < 48 ? left : right)++;
  TEST_ASSERT_GREATER_THAN_INT(150, left);
  TEST_ASSERT_GREATER_THAN_INT(150, right);
  // the only non-black colour is the identity colour
  for (int y = 0; y < Framebuffer::kHeight; ++y)
    for (int x = 0; x < Framebuffer::kWidth; ++x) {
      const uint16_t v = r.display.last.at(x, y).value;
      TEST_ASSERT_TRUE(v == 0 || v == rgb565(0, 200, 255).value);
    }
}

static void test_blinks_happen_every_2_to_6_seconds_while_awake() {
  Rig r;
  r.eyes.setColour(rgb565(255, 255, 255));
  r.eyes.setMood(Mood::Curious);
  r.eyes.tick();
  const int openLit = litPixels(r.display.last);
  uint32_t lastBlinkStart = r.clock.nowMs();
  bool inBlink = false;
  int blinks = 0;
  for (uint32_t t = 0; t < 120000; t += kTickMs) {
    r.clock.advance(kTickMs);
    r.eyes.tick();
    const bool closedNow = litPixels(r.display.last) < openLit / 3;
    if (closedNow && !inBlink) {
      const uint32_t gap = r.clock.nowMs() - lastBlinkStart;
      if (blinks > 0) {
        TEST_ASSERT_GREATER_OR_EQUAL_UINT32(2000, gap);
        TEST_ASSERT_LESS_OR_EQUAL_UINT32(6000, gap);
      }
      lastBlinkStart = r.clock.nowMs();
      ++blinks;
    }
    inBlink = closedNow;
  }
  TEST_ASSERT_GREATER_OR_EQUAL_INT(20, blinks);
  TEST_ASSERT_LESS_OR_EQUAL_INT(60, blinks);
}

static void test_a_blink_lasts_between_80_and_240_ms() {
  Rig r;
  r.eyes.setColour(rgb565(255, 255, 255));
  r.eyes.setMood(Mood::Curious);
  r.eyes.tick();
  const int openLit = litPixels(r.display.last);
  r.eyes.blinkNow();
  uint32_t closedMs = 0;
  for (int i = 0; i < 20; ++i) {
    r.clock.advance(kTickMs);
    r.eyes.tick();
    if (litPixels(r.display.last) < openLit / 3) closedMs += kTickMs;
  }
  TEST_ASSERT_GREATER_OR_EQUAL_UINT32(80, closedMs);
  TEST_ASSERT_LESS_OR_EQUAL_UINT32(240, closedMs);
}

static void test_frame_never_stays_identical_for_more_than_10_seconds() {
  Rig r;
  r.eyes.setColour(rgb565(255, 255, 255));
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

static void test_closed_eyes_show_almost_nothing_and_open_eyes_restore() {
  Rig r;
  r.eyes.setColour(rgb565(255, 255, 255));
  r.eyes.setMood(Mood::Curious);
  r.eyes.tick();
  const int open = litPixels(r.display.last);
  r.eyes.closeEyes();
  r.run(400);
  TEST_ASSERT_LESS_THAN_INT(open / 4, litPixels(r.display.last));
  r.eyes.openEyes();
  r.run(400);
  TEST_ASSERT_GREATER_THAN_INT(open * 3 / 4, litPixels(r.display.last));
}

static void test_brightness_is_forwarded_to_the_display() {
  Rig r;
  r.eyes.setBrightness(Brightness::Low);
  TEST_ASSERT_EQUAL(Brightness::Low, r.display.brightness);
  r.eyes.setBrightness(Brightness::Minimum);
  TEST_ASSERT_EQUAL(Brightness::Minimum, r.display.brightness);
}

static void test_drift_moves_the_frame_by_one_pixel_within_each_minute() {
  Rig r;
  r.eyes.setColour(rgb565(255, 255, 255));
  r.eyes.setMood(Mood::Bored);
  r.eyes.setIdleGlances(false);  // isolate the drift from the random glances
  r.eyes.tick();
  const Framebuffer first = r.display.last;
  bool moved = false;
  for (uint32_t t = 0; t < 61000 && !moved; t += kTickMs) {
    r.clock.advance(kTickMs);
    r.eyes.tick();
    moved = !framesEqual(first, r.display.last) && litPixels(r.display.last) == litPixels(first);
  }
  TEST_ASSERT_TRUE_MESSAGE(moved, "no one-pixel drift within 61 s");
}

static void test_golden_images_of_every_mood_and_of_closed_eyes() {
  const struct { Mood mood; const char* name; } cases[] = {
      {Mood::Curious, "curious"}, {Mood::Happy, "happy"}, {Mood::Bored, "bored"},
      {Mood::Tired, "tired"},     {Mood::Annoyed, "annoyed"},
  };
  for (const auto& c : cases) {
    Rig r;
    r.eyes.setColour(rgb565(0, 200, 255));
    r.eyes.setIdleGlances(false);
    r.eyes.setMood(c.mood);
    r.eyes.tick();
    checkGolden(c.name, r.display.last);
  }
  Rig r;
  r.eyes.setColour(rgb565(0, 200, 255));
  r.eyes.setIdleGlances(false);
  r.eyes.setMood(Mood::Tired);
  r.eyes.closeEyes();
  r.run(400);
  checkGolden("closed", r.display.last);
}

static void test_golden_images_of_a_blink_sequence() {
  Rig r;
  r.eyes.setColour(rgb565(0, 200, 255));
  r.eyes.setIdleGlances(false);
  r.eyes.setMood(Mood::Curious);
  r.eyes.tick();
  r.eyes.blinkNow();
  r.clock.advance(kTickMs);
  r.eyes.tick();
  checkGolden("blink-1", r.display.last);
  r.clock.advance(kTickMs);
  r.eyes.tick();
  checkGolden("blink-2", r.display.last);
  r.clock.advance(kTickMs);
  r.eyes.tick();
  checkGolden("blink-3", r.display.last);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_first_tick_presents_a_frame_with_two_eyes_in_the_identity_colour);
  RUN_TEST(test_blinks_happen_every_2_to_6_seconds_while_awake);
  RUN_TEST(test_a_blink_lasts_between_80_and_240_ms);
  RUN_TEST(test_frame_never_stays_identical_for_more_than_10_seconds);
  RUN_TEST(test_closed_eyes_show_almost_nothing_and_open_eyes_restore);
  RUN_TEST(test_brightness_is_forwarded_to_the_display);
  RUN_TEST(test_drift_moves_the_frame_by_one_pixel_within_each_minute);
  RUN_TEST(test_golden_images_of_every_mood_and_of_closed_eyes);
  RUN_TEST(test_golden_images_of_a_blink_sequence);
  return UNITY_END();
}
