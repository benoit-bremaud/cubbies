// Chain reaction (social-group/05b): one event per (origin palette, event id) key, remembered in
// an 8-key ring for T_seen; each cube reacts once after T_step × (1 + rank) and relays once with
// hop + 1 below kMaxHop. Pure: the Social protocol feeds it decoded CHAIN frames, the time and
// the rank, and broadcasts what it returns. Times are on one IClock timeline; a frame's receive
// time may be slightly later than the tick's `now`, which isDue() and ageMs() tolerate.
#pragma once

#include <cstdint>

#include "core/PresenceCodec.h"
#include "core/Prng.h"

namespace cubbies {

// What the chain asks for on a tick: react (eyes and sound), and/or relay `frame`.
struct ChainFire {
  bool react = false;
  bool relay = false;
  Chain frame{};
};

class ChainReaction {
 public:
  // Origin side (rows 1–3): true and `out` set when a frame must be sent. The event id is drawn
  // from `prng` only then, so a refused trigger does not shift the Prng sequence.
  bool start(uint32_t now, bool awake, uint8_t paletteIndex, uint8_t presentNeighbours, Prng& prng, Chain& out);
  // Receiver side (rows 4, 8–10, 12–14); `rank` = present neighbours with a lower MAC, sender excluded.
  void hear(const Chain& c, uint32_t now, uint8_t rank);
  // Fires the pending event once its delay has elapsed (rows 5–7, 11). Any fire, with or without a
  // reaction, starts T_cooldown (row 12), so relays are rate-limited too.
  ChainFire tick(uint32_t now, bool awake);

  // Tuning values (05b).
  static constexpr uint32_t kStepMs = 300;
  static constexpr uint32_t kTriggerMs = 10000;
  static constexpr uint32_t kCooldownMs = 5000;
  static constexpr uint32_t kSeenMs = 30000;
  static constexpr uint8_t kSeenKeys = 8;

 private:
  struct Key {
    uint8_t palette;
    uint8_t id;
    uint32_t at;
  };
  bool seen(const Chain& c, uint32_t now) const;
  void remember(const Chain& c, uint32_t now);

  Key keys_[kSeenKeys] = {};
  uint8_t keyCount_ = 0;
  uint8_t keyHead_ = 0;
  bool pending_ = false;
  Chain pendingFrame_{};
  uint32_t fireAt_ = 0;
  bool triggered_ = false;
  uint32_t lastTrigger_ = 0;
  bool reacted_ = false;
  uint32_t lastReaction_ = 0;
};

}  // namespace cubbies
