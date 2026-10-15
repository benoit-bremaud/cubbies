// Framebuffer 96×64 RGB565: primitives and clipping (no out-of-bounds writes).
#include <unity.h>

#include <climits>

#include "eyes/Framebuffer.h"

using cubbies::Framebuffer;
using cubbies::Rgb565;
using cubbies::rgb565;

void setUp() {}
void tearDown() {}

static int countColour(const Framebuffer& fb, Rgb565 c) {
  int n = 0;
  for (int y = 0; y < Framebuffer::kHeight; ++y)
    for (int x = 0; x < Framebuffer::kWidth; ++x)
      if (fb.at(x, y).value == c.value) ++n;
  return n;
}

static void test_dimensions_and_clear() {
  Framebuffer fb;
  fb.clear(rgb565(255, 0, 0));
  TEST_ASSERT_EQUAL_INT(96 * 64, countColour(fb, rgb565(255, 0, 0)));
}

static void test_rgb565_packing() {
  TEST_ASSERT_EQUAL_HEX16(0xF800, rgb565(255, 0, 0).value);
  TEST_ASSERT_EQUAL_HEX16(0x07E0, rgb565(0, 255, 0).value);
  TEST_ASSERT_EQUAL_HEX16(0x001F, rgb565(0, 0, 255).value);
  TEST_ASSERT_EQUAL_HEX16(0x0000, rgb565(0, 0, 0).value);
}

static void test_fill_rect_sets_exactly_its_pixels() {
  Framebuffer fb;
  fb.clear(rgb565(0, 0, 0));
  fb.fillRect(10, 20, 5, 4, rgb565(0, 255, 0));
  TEST_ASSERT_EQUAL_INT(20, countColour(fb, rgb565(0, 255, 0)));
  TEST_ASSERT_EQUAL_HEX16(0x07E0, fb.at(10, 20).value);
  TEST_ASSERT_EQUAL_HEX16(0x07E0, fb.at(14, 23).value);
  TEST_ASSERT_EQUAL_HEX16(0x0000, fb.at(15, 23).value);
  TEST_ASSERT_EQUAL_HEX16(0x0000, fb.at(14, 24).value);
}

static void test_fill_rect_clips_at_every_edge() {
  Framebuffer fb;
  fb.clear(rgb565(0, 0, 0));
  fb.fillRect(-5, -5, 10, 10, rgb565(0, 0, 255));      // top-left corner
  fb.fillRect(90, 60, 20, 20, rgb565(0, 0, 255));      // bottom-right corner
  fb.fillRect(200, 200, 10, 10, rgb565(0, 0, 255));    // fully outside
  fb.fillRect(0, 0, 0, 0, rgb565(0, 0, 255));          // empty
  TEST_ASSERT_EQUAL_INT(25 + 24, countColour(fb, rgb565(0, 0, 255)));
}

static void test_extreme_coordinates_leave_the_frame_untouched_and_terminate() {
  Framebuffer fb;
  fb.clear(rgb565(0, 0, 0));
  fb.fillRect(INT_MAX, INT_MAX, INT_MAX, INT_MAX, rgb565(8, 4, 8));
  fb.fillRect(INT_MIN, 0, 10, 10, rgb565(8, 4, 8));
  fb.fillRect(-10, -10, INT_MAX, INT_MAX, rgb565(255, 255, 255));  // covers the whole screen
  TEST_ASSERT_EQUAL_INT(96 * 64, countColour(fb, rgb565(255, 255, 255)));
  fb.clear(rgb565(0, 0, 0));
  fb.fillRoundRect(INT_MAX - 5, 0, 10, 10, 3, rgb565(8, 4, 8));
  fb.fillRoundRect(-100, -100, INT_MAX, INT_MAX, 50, rgb565(255, 255, 255));  // corners off screen
  TEST_ASSERT_EQUAL_INT(96 * 64, countColour(fb, rgb565(255, 255, 255)));
  fb.clear(rgb565(0, 0, 0));
  fb.fillCircle(INT_MIN, INT_MIN, INT_MAX, rgb565(8, 4, 8));
  fb.fillCircle(48, 32, INT_MAX, rgb565(255, 255, 255));
  TEST_ASSERT_EQUAL_INT(96 * 64, countColour(fb, rgb565(255, 255, 255)));
  fb.clear(rgb565(0, 0, 0));
  fb.fillCircle(-1000, 32, 5, rgb565(8, 4, 8));
  TEST_ASSERT_EQUAL_INT(0, countColour(fb, rgb565(8, 4, 8)));
}

// Review findings (Codex on PR #9): oversized shapes keep their exact geometry on screen.
static void test_oversized_round_rect_keeps_its_visible_corners() {
  Framebuffer fb;
  fb.clear(rgb565(0, 0, 0));
  fb.fillRoundRect(0, 0, 193, 20, 10, rgb565(255, 255, 255));
  TEST_ASSERT_EQUAL_HEX16(0x0000, fb.at(0, 0).value);     // corner pixel outside the arc
  TEST_ASSERT_EQUAL_HEX16(0xFFFF, fb.at(10, 10).value);
  TEST_ASSERT_EQUAL_HEX16(0xFFFF, fb.at(0, 10).value);    // mid-left edge filled
  TEST_ASSERT_EQUAL_HEX16(0xFFFF, fb.at(95, 10).value);   // interior reaches the right edge
}

static void test_large_circle_with_offscreen_centre_keeps_its_radius() {
  Framebuffer fb;
  fb.clear(rgb565(0, 0, 0));
  fb.fillCircle(200, 32, 200, rgb565(255, 255, 255));
  TEST_ASSERT_EQUAL_HEX16(0xFFFF, fb.at(1, 32).value);    // 199^2 < 200^2
  TEST_ASSERT_EQUAL_HEX16(0x0000, fb.at(0, 32).value);    // 200^2 is not < 200^2
  fb.clear(rgb565(0, 0, 0));
  fb.fillCircle(INT_MAX, 32, INT_MAX, rgb565(255, 255, 255));  // cx + r would overflow an int
  TEST_ASSERT_EQUAL_HEX16(0xFFFF, fb.at(1, 32).value);
  TEST_ASSERT_EQUAL_HEX16(0x0000, fb.at(0, 32).value);
}

static void test_at_outside_returns_black_and_does_not_crash() {
  Framebuffer fb;
  fb.clear(rgb565(255, 255, 255));
  TEST_ASSERT_EQUAL_HEX16(0x0000, fb.at(-1, 0).value);
  TEST_ASSERT_EQUAL_HEX16(0x0000, fb.at(96, 0).value);
  TEST_ASSERT_EQUAL_HEX16(0x0000, fb.at(0, 64).value);
}

static void test_round_rect_leaves_corners_unfilled() {
  Framebuffer fb;
  fb.clear(rgb565(0, 0, 0));
  fb.fillRoundRect(10, 10, 20, 20, 6, rgb565(255, 255, 255));
  TEST_ASSERT_EQUAL_HEX16(0x0000, fb.at(10, 10).value);  // corner pixel is outside the radius
  TEST_ASSERT_EQUAL_HEX16(0xFFFF, fb.at(20, 20).value);  // centre is filled
  TEST_ASSERT_EQUAL_HEX16(0xFFFF, fb.at(10, 20).value);  // mid-left edge is filled
  // 20×20 minus the 3 pixels per corner outside the radius-6 arc (deterministic rendering).
  TEST_ASSERT_EQUAL_INT(20 * 20 - 4 * 3, countColour(fb, rgb565(255, 255, 255)));
}

static void test_data_is_row_major_rgb565() {
  Framebuffer fb;
  fb.clear(rgb565(0, 0, 0));
  fb.fillRect(1, 0, 1, 1, rgb565(255, 0, 0));
  TEST_ASSERT_EQUAL_HEX16(0xF800, fb.data()[1]);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_dimensions_and_clear);
  RUN_TEST(test_rgb565_packing);
  RUN_TEST(test_fill_rect_sets_exactly_its_pixels);
  RUN_TEST(test_fill_rect_clips_at_every_edge);
  RUN_TEST(test_extreme_coordinates_leave_the_frame_untouched_and_terminate);
  RUN_TEST(test_oversized_round_rect_keeps_its_visible_corners);
  RUN_TEST(test_large_circle_with_offscreen_centre_keeps_its_radius);
  RUN_TEST(test_at_outside_returns_black_and_does_not_crash);
  RUN_TEST(test_round_rect_leaves_corners_unfilled);
  RUN_TEST(test_data_is_row_major_rgb565);
  return UNITY_END();
}
