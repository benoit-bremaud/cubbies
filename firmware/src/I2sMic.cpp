#include "I2sMic.h"

#include <cmath>
#include <cstdlib>

namespace cubbies {

namespace {

constexpr uint32_t kSampleRateHz = 16000;

uint16_t clamp16(uint32_t v) { return v > 0xFFFFu ? 0xFFFFu : static_cast<uint16_t>(v); }

}  // namespace

bool I2sMic::begin() {
  i2s_.setPins(bclk_, ws_, -1, sd_);
  if (!i2s_.begin(I2S_MODE_STD, kSampleRateHz, I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_MONO,
                  I2S_STD_SLOT_LEFT)) {
    return false;
  }
  ready_ = true;
  return true;
}

MicLevels I2sMic::read() {
  if (!ready_) return {0, 0};
  uint64_t sumSq = 0;
  uint32_t n = 0;
  uint32_t peak = 0;
  for (;;) {
    // Straight to the IDF channel: I2SClass::readBytes loops until the buffer is full and
    // discards a partial read on timeout. Timeout 0 returns whatever the DMA holds.
    size_t got = 0;
    i2s_channel_read(i2s_.rxChan(), chunk_, sizeof(chunk_), &got, 0);
    const size_t samples = got / sizeof(int32_t);
    for (size_t i = 0; i < samples; ++i) {
      // INMP441: 24-bit data left-justified in the 32-bit slot; keep the top 16 bits.
      const int32_t s = chunk_[i] >> 16;
      dc_ += (s * 256 - dc_) >> 10;  // ≈ 0.6 s time constant at 16 kHz
      const uint32_t a = static_cast<uint32_t>(std::abs(s - (dc_ >> 8)));
      sumSq += static_cast<uint64_t>(a) * a;
      if (a > peak) peak = a;
    }
    n += samples;
    if (samples < kChunkSamples) break;
  }
  if (n == 0) return {0, 0};
  // Signed 16-bit magnitudes reach 32767; doubled to fill the port's unsigned full scale.
  const auto rms = static_cast<uint32_t>(std::sqrt(static_cast<double>(sumSq) / n));
  return {clamp16(rms * 2u), clamp16(peak * 2u)};
}

}  // namespace cubbies
