// IClock adapter: Arduino millis(), an unsigned 32-bit counter that wraps like the port says.
#pragma once

#include <Arduino.h>

#include "ports/IClock.h"

namespace cubbies {

class MillisClock final : public IClock {
 public:
  uint32_t nowMs() const override { return millis(); }
};

}  // namespace cubbies
