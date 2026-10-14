// PCG32 pseudo-random generator, lib-owned (ADR-0013 D1): one instance per consumer, seeded
// by main.cpp from esp_random() after the Wi-Fi driver is up, or by a fixed seed in tests.
// Never used for secrets. Algorithm: PCG-XSH-RR 64/32, Melissa O'Neill, pcg-random.org
// (minimal C implementation, Apache-2.0).
#pragma once

#include <cstdint>

namespace cubbies {

class Prng {
 public:
  Prng(uint64_t seed, uint64_t stream);
  uint32_t next();
  // Uniform draw in [0, n). Returns 0 for n <= 1. Unbiased (rejection sampling).
  uint32_t uniform(uint32_t n);

 private:
  uint64_t state_;
  uint64_t inc_;
};

}  // namespace cubbies
