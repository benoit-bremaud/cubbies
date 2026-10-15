// Presence frame codec (social-presence/06): the 8-byte v1 PRESENCE frame, parsed by allowlist,
// and the mood byte computed from the Sleep cycle and Mood engine states (05b table, ADR-0013).
// Pure functions over a byte array: no clock, no I/O, fuzzed natively.
#pragma once

#include <cstddef>
#include <cstdint>

#include "core/Mood.h"
#include "core/SleepCycle.h"
#include "ports/IRadio.h"

namespace cubbies {

constexpr uint8_t kMagic0 = 0x43;  // 'C'
constexpr uint8_t kMagic1 = 0x42;  // 'B'
constexpr uint8_t kProtocolVersion = 1;
constexpr uint8_t kTypePresence = 0x01;
constexpr uint8_t kMoodAsleep = 5;
constexpr uint8_t kFlagAdopted = 0x01;

struct Presence {
  uint8_t paletteIndex;
  uint8_t mood;  // 0..4 a Mood, 5 asleep
  bool adopted;
};

// Why a frame was dropped; Ok means `out` holds a valid Presence.
enum class DecodeResult : uint8_t { Ok, Len, Magic, Version, Type, Range };

// Mood byte on the air (05b): Awake → mood, Dozing → Tired, Waking → Tired from Dozing or
// asleep from Asleep, Asleep → asleep.
uint8_t moodByte(SleepState sleep, SleepState wakeOrigin, Mood mood);

void encodePresence(const Presence& p, uint8_t out[kFrameSize]);
DecodeResult decodePresence(const uint8_t* bytes, size_t len, Presence& out);

}  // namespace cubbies
