// Port: monotonic uptime in milliseconds (never a time of day, C10).
// Contract: unsigned 32-bit counter that wraps every 49.7 days; consumers compare ages only
// through (uint32_t)(now - then). Real adapter: MillisClock; test double: FakeClock.
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

}  // namespace cubbies
