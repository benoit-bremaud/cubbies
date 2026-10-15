// Prng (PCG32): determinism by seed, independent streams, bounded draws (ADR-0013 D1).
#include <unity.h>

#include "core/Prng.h"

using cubbies::Prng;

void setUp() {}
void tearDown() {}

static void test_same_seed_gives_same_sequence() {
  Prng a(42, 1);
  Prng b(42, 1);
  for (int i = 0; i < 1000; ++i) TEST_ASSERT_EQUAL_UINT32(a.next(), b.next());
}

// Expected number of equal outputs between two independent 32-bit streams is ~0.
static void test_different_seed_gives_different_sequence() {
  Prng a(42, 1);
  Prng b(43, 1);
  int same = 0;
  for (int i = 0; i < 100; ++i) same += (a.next() == b.next()) ? 1 : 0;
  TEST_ASSERT_LESS_THAN_INT(5, same);
}

static void test_different_stream_gives_different_sequence() {
  Prng a(42, 1);
  Prng b(42, 2);
  int same = 0;
  for (int i = 0; i < 100; ++i) same += (a.next() == b.next()) ? 1 : 0;
  TEST_ASSERT_LESS_THAN_INT(5, same);
}

static void test_uniform_stays_below_bound_and_covers_range() {
  Prng p(7, 1);
  bool seen[10] = {false};
  for (int i = 0; i < 10000; ++i) {
    const uint32_t v = p.uniform(10);
    TEST_ASSERT_LESS_THAN_UINT32(10, v);
    seen[v] = true;
  }
  for (bool s : seen) TEST_ASSERT_TRUE(s);
}

static void test_uniform_of_one_is_zero_and_of_zero_is_zero() {
  Prng p(7, 1);
  TEST_ASSERT_EQUAL_UINT32(0, p.uniform(1));
  TEST_ASSERT_EQUAL_UINT32(0, p.uniform(0));
}

static void test_uniform_with_a_large_bound_exercises_the_rejection_loop() {
  Prng p(7, 1);
  for (int i = 0; i < 1000; ++i) TEST_ASSERT_LESS_THAN_UINT32(0x80000001u, p.uniform(0x80000001u));
}

static void test_known_pcg32_reference_value() {
  // pcg32 reference: seed 42, seq 54 → first output 0xa15c02b7 (pcg-random.org sample).
  Prng p(42, 54);
  TEST_ASSERT_EQUAL_HEX32(0xa15c02b7, p.next());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_same_seed_gives_same_sequence);
  RUN_TEST(test_different_seed_gives_different_sequence);
  RUN_TEST(test_different_stream_gives_different_sequence);
  RUN_TEST(test_uniform_stays_below_bound_and_covers_range);
  RUN_TEST(test_uniform_of_one_is_zero_and_of_zero_is_zero);
  RUN_TEST(test_uniform_with_a_large_bound_exercises_the_rejection_loop);
  RUN_TEST(test_known_pcg32_reference_value);
  return UNITY_END();
}
