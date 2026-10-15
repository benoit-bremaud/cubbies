// Port: monotonic uptime in milliseconds (never a time of day, C10).
// Contract: unsigned 32-bit counter that wraps every 49.7 days; consumers compare ages only
// through ageMs() / isDue(), never with `now > then + T`. Real adapter: MillisClock (F5);
// test double: FakeClock.
#pragma once

#include <cstdint>

namespace cubbies {

class IClock {
 public:
  virtual ~IClock() = default;
  virtual uint32_t nowMs() const = 0;
};

// Age of `then` seen from `now`, correct across the wrap.
constexpr uint32_t ageMs(uint32_t now, uint32_t then) { return now - then; }

// True once `deadline` has been reached, correct across the wrap for deadlines less than
// 24.8 days away (half-range rule: a "negative" age wraps above 2^31).
constexpr bool isDue(uint32_t now, uint32_t deadline) { return ageMs(now, deadline) < 0x80000000u; }

}  // namespace cubbies
