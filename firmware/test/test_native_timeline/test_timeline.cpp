// Timeline: the one "start time + duration + phase" lifecycle shared by blinks, lid moves and
// reactions (F1 review: third timed animation → extract). Wrap-safe on the uint32 clock.
#include <unity.h>

#include "eyes/Timeline.h"

using cubbies::Timeline;

void setUp() {}
void tearDown() {}

static void test_inactive_until_started() {
  Timeline t;
  TEST_ASSERT_FALSE(t.active(0));
  TEST_ASSERT_FALSE(t.active(1000));
  TEST_ASSERT_EQUAL_UINT8(255, t.phase(1000));  // a finished or never-started timeline is "done"
}

static void test_phase_runs_from_0_to_255_over_the_duration_then_ends() {
  Timeline t;
  t.start(1000, 400);
  TEST_ASSERT_TRUE(t.active(1000));
  TEST_ASSERT_EQUAL_UINT8(0, t.phase(1000));
  TEST_ASSERT_EQUAL_UINT8(127, t.phase(1200));
  TEST_ASSERT_TRUE(t.active(1399));
  TEST_ASSERT_FALSE(t.active(1400));
  TEST_ASSERT_EQUAL_UINT8(255, t.phase(1400));
  TEST_ASSERT_EQUAL_UINT8(255, t.phase(9999));
}

static void test_elapsed_and_stop() {
  Timeline t;
  t.start(500, 1000);
  TEST_ASSERT_EQUAL_UINT32(250, t.elapsed(750));
  t.stop();
  TEST_ASSERT_FALSE(t.active(750));
  TEST_ASSERT_EQUAL_UINT8(255, t.phase(750));
}

static void test_survives_the_uint32_wrap() {
  Timeline t;
  t.start(0xFFFFFFFFu - 100, 400);
  TEST_ASSERT_TRUE(t.active(0xFFFFFFFFu));
  TEST_ASSERT_TRUE(t.active(200));          // 301 ms in
  TEST_ASSERT_EQUAL_UINT32(301, t.elapsed(200));
  TEST_ASSERT_FALSE(t.active(300));         // 401 ms in
}

static void test_zero_duration_is_immediately_done() {
  Timeline t;
  t.start(10, 0);
  TEST_ASSERT_FALSE(t.active(10));
  TEST_ASSERT_EQUAL_UINT8(255, t.phase(10));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_inactive_until_started);
  RUN_TEST(test_phase_runs_from_0_to_255_over_the_duration_then_ends);
  RUN_TEST(test_elapsed_and_stop);
  RUN_TEST(test_survives_the_uint32_wrap);
  RUN_TEST(test_zero_duration_is_immediately_done);
  return UNITY_END();
}
