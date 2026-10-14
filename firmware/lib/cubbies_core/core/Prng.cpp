#include "core/Prng.h"

namespace cubbies {

namespace {
constexpr uint64_t kMultiplier = 6364136223846793005ULL;
}

Prng::Prng(uint64_t seed, uint64_t stream) : state_(0), inc_((stream << 1u) | 1u) {
  next();
  state_ += seed;
  next();
}

uint32_t Prng::next() {
  const uint64_t old = state_;
  state_ = old * kMultiplier + inc_;
  const uint32_t xorshifted = static_cast<uint32_t>(((old >> 18u) ^ old) >> 27u);
  const uint32_t rot = static_cast<uint32_t>(old >> 59u);
  return (xorshifted >> rot) | (xorshifted << ((-rot) & 31u));
}

uint32_t Prng::uniform(uint32_t n) {
  if (n <= 1) return 0;
  const uint32_t threshold = (0u - n) % n;  // rejection bound for an unbiased result
  for (;;) {
    const uint32_t r = next();
    if (r >= threshold) return r % n;
  }
}

}  // namespace cubbies
