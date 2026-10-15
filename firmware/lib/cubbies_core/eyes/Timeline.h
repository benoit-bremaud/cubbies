// The one "start time + duration + phase" lifecycle shared by blinks, lid moves, reactions and
// identity animations. Wrap-safe on the uint32 uptime (ages only, never absolute compares).
// Contract: start() with durationMs == 0 leaves the timeline inactive (held reactions use their
// own flag); phase() runs 0..254 while active and is 255 when inactive, so "phase == 255" also
// means done; elapsed() keeps growing past the duration until stop(). Owners call stop() once
// they have seen the end, so a finished timeline can never look active again 49.7 days later.
#pragma once

#include <cstdint>

#include "ports/IClock.h"

namespace cubbies {

class Timeline {
 public:
  void start(uint32_t nowMs, uint32_t durationMs) {
    startMs_ = nowMs;
    durationMs_ = durationMs;
    running_ = durationMs > 0;
  }
  void stop() { running_ = false; }
  bool active(uint32_t nowMs) const { return running_ && ageMs(nowMs, startMs_) < durationMs_; }
  uint32_t elapsed(uint32_t nowMs) const { return running_ ? ageMs(nowMs, startMs_) : 0; }
  uint8_t phase(uint32_t nowMs) const {
    if (!active(nowMs)) return 255;
    return static_cast<uint8_t>((static_cast<uint64_t>(ageMs(nowMs, startMs_)) * 255) / durationMs_);
  }

 private:
  uint32_t startMs_ = 0;
  uint32_t durationMs_ = 0;
  bool running_ = false;
};

}  // namespace cubbies
