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

namespace {

void writeHeader(uint8_t type, uint8_t out[kFrameSize]) {
  out[0] = kMagic0;
  out[1] = kMagic1;
  out[2] = kProtocolVersion;
  out[3] = type;
}

// Shared allowlist of bytes 0–3 (06); Ok means the payload bytes 4–7 may be checked.
DecodeResult checkHeader(const uint8_t* bytes, size_t len, uint8_t type) {
  if (bytes == nullptr || len != kFrameSize) return DecodeResult::Len;
  if (bytes[0] != kMagic0 || bytes[1] != kMagic1) return DecodeResult::Magic;
  if (bytes[2] != kProtocolVersion) return DecodeResult::Version;
  return bytes[3] == type ? DecodeResult::Ok : DecodeResult::Type;
}

}  // namespace

void encodePresence(const Presence& p, uint8_t out[kFrameSize]) {
  writeHeader(kTypePresence, out);
  out[4] = p.paletteIndex;
  out[5] = p.mood;
  out[6] = p.adopted ? kFlagAdopted : 0;
  out[7] = 0;
}

DecodeResult decodePresence(const uint8_t* bytes, size_t len, Presence& out) {
  if (const DecodeResult h = checkHeader(bytes, len, kTypePresence); h != DecodeResult::Ok) return h;
  if (bytes[4] >= kPaletteSize || bytes[5] > kMoodAsleep || (bytes[6] & ~kFlagAdopted) != 0 ||
      bytes[7] != 0)
    return DecodeResult::Range;
  out = Presence{bytes[4], bytes[5], (bytes[6] & kFlagAdopted) != 0};
  return DecodeResult::Ok;
}

void encodeChain(const Chain& c, uint8_t out[kFrameSize]) {
  writeHeader(kTypeChain, out);
  out[4] = c.paletteIndex;
  out[5] = c.eventId;
  out[6] = c.hop;
  out[7] = 0;
}

DecodeResult decodeChain(const uint8_t* bytes, size_t len, Chain& out) {
  if (const DecodeResult h = checkHeader(bytes, len, kTypeChain); h != DecodeResult::Ok) return h;
  if (bytes[4] >= kPaletteSize || bytes[6] > kMaxHop || bytes[7] != 0) return DecodeResult::Range;
  out = Chain{bytes[4], bytes[5], bytes[6]};
  return DecodeResult::Ok;
}

void encodeChoir(const ChoirStart& c, uint8_t out[kFrameSize]) {
  writeHeader(kTypeChoir, out);
  out[4] = c.paletteIndex;
  out[5] = c.melody;
  out[6] = c.lead;
  out[7] = 0;
}

DecodeResult decodeChoir(const uint8_t* bytes, size_t len, ChoirStart& out) {
  if (const DecodeResult h = checkHeader(bytes, len, kTypeChoir); h != DecodeResult::Ok) return h;
  if (bytes[4] >= kPaletteSize || bytes[5] != kChoirMelody || bytes[6] == 0 || bytes[6] > kMaxChoirLead ||
      bytes[7] != 0)
    return DecodeResult::Range;
  out = ChoirStart{bytes[4], bytes[5], bytes[6]};
  return DecodeResult::Ok;
}

}  // namespace cubbies
