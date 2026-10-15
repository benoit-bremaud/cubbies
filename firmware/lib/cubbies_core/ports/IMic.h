// Port: the microphone as two levels per tick, never samples (03-component, 05d). Both cover
// every sample since the previous call, so a clap between two ticks still shows in the peak.
// Unsigned 16-bit, full scale of the adapter. Real adapter: I2sMic (F5); test double: ScriptedMic.
#pragma once

#include <cstdint>

namespace cubbies {

struct MicLevels {
  uint16_t rms;
  uint16_t peak;
};

class IMic {
 public:
  virtual ~IMic() = default;
  virtual MicLevels read() = 0;
};

}  // namespace cubbies
