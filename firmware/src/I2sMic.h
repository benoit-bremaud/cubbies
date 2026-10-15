// IMic adapter: INMP441 on I2S standard mode, 16 kHz, 32-bit slots, mono left
// (electronics-spec). read() drains every sample since the previous call and returns only
// rms and peak. Privacy boundary (C7, REQ-NF-07): the sample buffer never leaves this
// object and nothing here logs a sample.
#pragma once

#include <ESP_I2S.h>

#include "ports/IMic.h"

namespace cubbies {

class I2sMic final : public IMic {
 public:
  I2sMic(int8_t bclk, int8_t ws, int8_t sd) : bclk_(bclk), ws_(ws), sd_(sd) {}
  bool begin();
  MicLevels read() override;

 private:
  static constexpr size_t kChunkSamples = 128;

  int8_t bclk_, ws_, sd_;
  I2SClass i2s_;
  int32_t chunk_[kChunkSamples];
  int32_t dc_ = 0;  // running DC offset of the 16-bit samples, Q8 fixed point
};

}  // namespace cubbies
