#include "core/PresenceCodec.h"

#include "core/Identity.h"

namespace cubbies {

uint8_t moodByte(SleepState sleep, SleepState wakeOrigin, Mood mood) {
  switch (sleep) {
    case SleepState::Awake:
      return static_cast<uint8_t>(mood);
    case SleepState::Dozing:
      return static_cast<uint8_t>(Mood::Tired);
    case SleepState::Waking:
      return wakeOrigin == SleepState::Asleep ? kMoodAsleep : static_cast<uint8_t>(Mood::Tired);
    case SleepState::Asleep:
      break;
  }
  return kMoodAsleep;
}

void encodePresence(const Presence& p, uint8_t out[kFrameSize]) {
  out[0] = kMagic0;
  out[1] = kMagic1;
  out[2] = kProtocolVersion;
  out[3] = kTypePresence;
  out[4] = p.paletteIndex;
  out[5] = p.mood;
  out[6] = p.adopted ? kFlagAdopted : 0;
  out[7] = 0;
}

DecodeResult decodePresence(const uint8_t* bytes, size_t len, Presence& out) {
  if (bytes == nullptr || len != kFrameSize) return DecodeResult::Len;
  if (bytes[0] != kMagic0 || bytes[1] != kMagic1) return DecodeResult::Magic;
  if (bytes[2] != kProtocolVersion) return DecodeResult::Version;
  if (bytes[3] != kTypePresence) return DecodeResult::Type;
  if (bytes[4] >= kPaletteSize || bytes[5] > kMoodAsleep || (bytes[6] & ~kFlagAdopted) != 0 ||
      bytes[7] != 0)
    return DecodeResult::Range;
  out = Presence{bytes[4], bytes[5], (bytes[6] & kFlagAdopted) != 0};
  return DecodeResult::Ok;
}

void encodeChain(const Chain& c, uint8_t out[kFrameSize]) {
  out[0] = kMagic0;
  out[1] = kMagic1;
  out[2] = kProtocolVersion;
  out[3] = kTypeChain;
  out[4] = c.paletteIndex;
  out[5] = c.eventId;
  out[6] = c.hop;
  out[7] = 0;
}

DecodeResult decodeChain(const uint8_t* bytes, size_t len, Chain& out) {
  if (bytes == nullptr || len != kFrameSize) return DecodeResult::Len;
  if (bytes[0] != kMagic0 || bytes[1] != kMagic1) return DecodeResult::Magic;
  if (bytes[2] != kProtocolVersion) return DecodeResult::Version;
  if (bytes[3] != kTypeChain) return DecodeResult::Type;
  if (bytes[4] >= kPaletteSize || bytes[6] > kMaxHop || bytes[7] != 0) return DecodeResult::Range;
  out = Chain{bytes[4], bytes[5], bytes[6]};
  return DecodeResult::Ok;
}

void encodeChoir(const ChoirStart&, uint8_t out[kFrameSize]) {
  for (size_t i = 0; i < kFrameSize; ++i) out[i] = 0;
}

DecodeResult decodeChoir(const uint8_t*, size_t, ChoirStart&) { return DecodeResult::Len; }

}  // namespace cubbies
