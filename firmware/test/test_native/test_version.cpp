// Native smoke test: proves the host test pipeline works before any behaviour exists.
#include <unity.h>

#include <cstring>

#include "version.h"

void setUp() {}
void tearDown() {}

static void test_firmware_version_is_semver_initial_development() {
  TEST_ASSERT_EQUAL_CHAR('0', cubbies::kFirmwareVersion[0]);
  TEST_ASSERT_EQUAL_CHAR('.', cubbies::kFirmwareVersion[1]);
  TEST_ASSERT_TRUE(std::strlen(cubbies::kFirmwareVersion) >= 5);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_firmware_version_is_semver_initial_development);
  return UNITY_END();
}
