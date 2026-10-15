// Test double for IMic: levels the test sets by hand.
#pragma once

#include <cstdint>

#include "ports/IMic.h"

namespace cubbies::test {

class ScriptedMic final : public IMic {
 public:
  MicLevels read() override { return levels_; }
  void set(uint16_t rms, uint16_t peak) { levels_ = {rms, peak}; }

 private:
  MicLevels levels_{0, 0};
};

}  // namespace cubbies::test
