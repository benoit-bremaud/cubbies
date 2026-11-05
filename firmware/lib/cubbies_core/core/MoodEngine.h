// Mood engine (05a): the five moods and what moves them. Fed with classified stimuli by the
// Stimulus router, ticked on the injected clock, and pulled by the neighbours' tally
// (social-group/05a). Returns only the side effects it owns (the
// yawn of Tired, the grumpy beep on entering Annoyed, the jealous Sulk of social-group/05e); the generic reaction to each stimulus
// (S3: surprise on a tap, purr on a stroke, …) is the router's. Paused by the Sleep cycle, which
// calls restart() at every wake.
#pragma once

#include <cstdint>

#include "core/Mood.h"
#include "ports/IClock.h"

namespace cubbies {

// Released ends a medium press or a stroke: a gesture like the others (owner decision
// 2026-10-15, so a medium press wakes a Bored or Tired mood).
enum class Stimulus : uint8_t { Tap, DoubleTap, Stroke, Released, Voice, Clap, Loud, NewcomerSeen };
enum class MoodEffect : uint8_t { None, Yawn, Grumpy, Sulk };

class MoodEngine {
 public:
  explicit MoodEngine(IClock& clock);

  Mood mood() const { return mood_; }
  void restart();                        // Curious, all timers from now (boot and every wake)
  MoodEffect stimulus(Stimulus s);       // a classified stimulus, at most one per call
  MoodEffect tick();                     // timers; call once per tick
  // An owner touch began: breaks the calm at once, so contagion cannot pull on the tick of the
  // touch before its gesture is classified (social-group/05a row 6). The mood is unchanged.
  void touchDown() { lastLoudMs_ = clock_.nowMs(); }
  // Contagion (social-group/05a): call once per Awake tick, before tick(), with the neighbours'
  // tally. Enters the pulled mood once its candidate held T_contagion in calm, out of refractory,
  // and returns that mood's entry effect (Grumpy for Annoyed). The first pull is never
  // refractory; restart() drops the hold but keeps the refractory.
  // Collective sleep (social-group/05d) is evaluated first: a Bored cube whose drowsy neighbours
  // held T_contagion in calm enters Tired and returns its Yawn, without starting the refractory.
  MoodEffect contagion(const MoodTally& tally);
  // Jealousy (social-group/05e): call when the Social protocol heard a PET this Awake tick.
  // Sulk when calm ≥ kJealousCalmMs, out of the kJealousMs cooldown and within the budget; the
  // mood, its timers, the calm and the G2 refractory are unchanged. restart() keeps the limits.
  MoodEffect jealousy();

  // Durations (05a starting values, tuning, no ADR).
  static constexpr uint32_t kSettleMs = 10000;
  static constexpr uint32_t kCalmMs = 20000;
  static constexpr uint32_t kBoredMs = 180000;
  static constexpr uint32_t kTiredMs = 300000;
  static constexpr uint32_t kYawnMs = 45000;
  // social-group/05a: just above one presence period and below kSettleMs, so a petted cube
  // spreads its joy; the refractory kills two-cube ping-pong and caps forged-frame nuisance.
  static constexpr uint32_t kContagionMs = 6000;
  static constexpr uint32_t kRefractoryMs = 60000;
  // social-group/05e: a cube touched in the last T_contagion is in use; the cooldown and the
  // budget cap forged PET nuisance (ADR-0012).
  static constexpr uint32_t kJealousCalmMs = kContagionMs;
  static constexpr uint32_t kJealousMs = 60000;
  static constexpr uint32_t kJealousWindowMs = 600000;
  static constexpr uint8_t kJealousBudget = 4;

 private:
  MoodEffect enter(Mood m);
  bool catchesYawn(const MoodTally& tally, uint32_t now);

  IClock& clock_;
  Mood mood_ = Mood::Curious;
  uint32_t enteredMs_ = 0;    // when the current mood started
  uint32_t lastAnyMs_ = 0;    // last stimulus of any kind (idle timer)
  uint32_t lastLoudMs_ = 0;   // last gesture, touch-down, sound, pull or caught yawn (calm timer); NewcomerSeen does not count
  uint32_t lastYawnMs_ = 0;
  bool holding_ = false;       // a contagion candidate is being held
  Mood candidate_ = Mood::Curious;
  uint32_t holdSinceMs_ = 0;
  bool yawnHolding_ = false;   // a drowsy neighbour is being held (05d)
  uint32_t yawnSinceMs_ = 0;
  bool pulled_ = false;        // at least one pull happened (refractory applies)
  uint32_t lastPullMs_ = 0;
  uint32_t sulks_[kJealousBudget] = {};  // ring of the last sulk times (05e rows 3, 4)
  uint8_t sulkCount_ = 0;                // sulks recorded so far, saturates at kJealousBudget
  uint8_t sulkHead_ = 0;                 // next ring slot, also the oldest once full
};

}  // namespace cubbies
