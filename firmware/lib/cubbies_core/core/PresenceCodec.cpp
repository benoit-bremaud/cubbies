#include "core/PresenceCodec.h"

namespace cubbies {

uint8_t moodByte(SleepState, SleepState, Mood) { return 0; }

void encodePresence(const Presence&, uint8_t out[kFrameSize]) {
  for (size_t i = 0; i < kFrameSize; ++i) out[i] = 0;
}

DecodeResult decodePresence(const uint8_t*, size_t, Presence& out) {
  out = Presence{};
  return DecodeResult::Ok;
}

}  // namespace cubbies
