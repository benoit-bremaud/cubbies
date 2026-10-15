// Test double for IClock: a clock the test advances by hand (FIRST: repeatable).
#pragma once

#include <cstdint>

#include "ports/IClock.h"

namespace cubbies::test {

class FakeClock final : public IClock {
 public:
  explicit FakeClock(uint32_t startMs = 0) : now_(startMs) {}
  uint32_t nowMs() const override { return now_; }
  void advance(uint32_t ms) { now_ += ms; }  // wraps like the real counter
  void set(uint32_t ms) { now_ = ms; }

 private:
  uint32_t now_;
};

}  // namespace cubbies::test
