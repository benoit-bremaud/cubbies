// Social protocol (social-presence/02, 05a): drains at most 4 frames per tick from IRadio, keeps
// the 16-slot neighbour table (RAM only), greets each newcomer once per arrival under a cooldown
// and a sliding budget, and broadcasts PRESENCE every 5 s ± 1 s (Prng jitter). Returns the
// greeting as a SocialOutput that main.cpp hands to routeNewcomer(). Drop counters by reason are
// exposed through stats() for debug output.
#pragma once

#include <cstdint>

#include "core/ChainReaction.h"
#include "core/Identity.h"
#include "core/Mood.h"
#include "core/Prng.h"
#include "core/SleepCycle.h"
#include "ports/IClock.h"
#include "ports/IRadio.h"

namespace cubbies {

// The cube's own state, read by main.cpp from the Identity, Sleep cycle and Mood engine.
struct SelfState {
  uint8_t paletteIndex;
  bool adopted;
  SleepState sleep;
  SleepState wakeOrigin;
  Mood mood;
};

struct SocialOutput {
  bool newcomer = false;
  uint8_t paletteIndex = 0;
  bool chain = false;  // a chain event fired here and the cube is Awake: react (social-group/05b)
};

// Drop counters keyed by reason, never by MAC (06 privacy note).
struct SocialStats {
  uint32_t dropLen = 0;
  uint32_t dropMagic = 0;
  uint32_t dropVersion = 0;
  uint32_t dropType = 0;
  uint32_t dropRange = 0;
  uint32_t dropFull = 0;
};

enum class EntryState : uint8_t { Free, Waiting, Greeted, Absent };

struct Neighbour {
  EntryState state = EntryState::Free;
  Mac mac{};
  uint8_t paletteIndex = 0;
  uint8_t mood = 0;
  uint32_t lastSeen = 0;
  bool greeted = false;  // kept while Absent (05a)
};

class SocialProtocol {
 public:
  // `self` is the cube's own MAC, used to rank it in the chain wave (social-group/05b).
  SocialProtocol(IClock& clock, IRadio& radio, Prng& prng, const Mac& self);

  SocialOutput tick(const SelfState& self);
  // Owner double tap (05b rows 1–3): broadcasts a new CHAIN event when a neighbour is present
  // and the trigger cooldown is over; returns whether a frame was sent.
  bool startChain(uint8_t paletteIndex);

  const SocialStats& stats() const { return stats_; }
  static constexpr uint8_t kCapacity = 16;
  MoodTally moodTally() const;  // present, awake, non-Tired neighbours (social-group/05a)
  const Neighbour& neighbour(uint8_t slot) const { return table_[slot]; }  // slot < kCapacity

  // Tuning values (02, 05a).
  static constexpr uint32_t kPresenceMs = 5000;
  static constexpr uint32_t kJitterMs = 1000;
  static constexpr uint32_t kTtlMs = 15000;
  static constexpr uint32_t kGreetCooldownMs = 10000;
  static constexpr uint32_t kRegreetMs = 600000;
  static constexpr uint32_t kBudgetWindowMs = 600000;
  static constexpr uint8_t kBudget = 6;
  static constexpr uint8_t kMaxFramesPerTick = 4;

 private:
  void receive(const RxFrame& f);
  void count(DecodeResult r);
  uint8_t presentCount() const;
  uint8_t rank() const;
  Neighbour* findSlot(const Mac& mac, uint32_t now);
  void expire(uint32_t now);
  SocialOutput greet(uint32_t now);
  void broadcastIfDue(uint32_t now, const SelfState& self);

  IClock& clock_;
  IRadio& radio_;
  Prng& prng_;
  Mac self_;
  ChainReaction chain_;
  Neighbour table_[kCapacity];
  SocialStats stats_;
  uint32_t greetings_[kBudget] = {};  // ring of the last greeting times
  uint8_t greetCount_ = 0;            // greetings recorded so far, saturates at kBudget
  uint8_t greetHead_ = 0;             // next ring slot, also the oldest once full
  uint32_t nextBroadcast_ = 0;
  bool started_ = false;
};

}  // namespace cubbies
