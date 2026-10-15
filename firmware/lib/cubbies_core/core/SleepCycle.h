// Sleep cycle (05b): Awake, Dozing, Asleep and Waking, orthogonal to the mood. Reads the mood
// ("is Tired") and restarts the Mood engine at every wake; the caller ticks the Mood engine only
// while Awake (paused otherwise). Returns the side effect of each transition and exposes the
// brightness level and, in Waking, where the wake came from (for the presence codec).
#pragma once

#include <cstdint>

#include "core/GestureClassifier.h"
#include "core/MoodEngine.h"
#include "core/SoundClassifier.h"
#include "ports/IClock.h"
#include "ports/IDisplay.h"

namespace cubbies {

enum class SleepState : uint8_t { Awake, Dozing, Asleep, Waking };

// One per transition of 05b: dozed off (low brightness), fell asleep (eyes closed, minimum),
// woke from a doze (short animation, no melody), woke from sleep (yawn and wake-up melody),
// awake again (full brightness, mood restarted in Curious).
enum class SleepEffect : uint8_t { None, Dozed, FellAsleep, WokeFromDoze, WokeFromSleep, Awoke };

class SleepCycle {
 public:
  SleepCycle(IClock& clock, MoodEngine& mood);

  SleepState state() const { return state_; }
  SleepState wakeOrigin() const { return origin_; }  // Dozing or Asleep; meaningful in Waking
  Brightness brightness() const;

  SleepEffect gesture(Gesture g);    // a classified gesture; any contact wakes (TouchDown)
  SleepEffect sound(SoundClass s);   // a classified sound
  SleepEffect tick();                // timers; call once per tick

  // Durations (05b, ADR-0013 starting values; Waking timers owner decisions 2026-10-15).
  static constexpr uint32_t kDozeMs = 120000;
  static constexpr uint32_t kFallAsleepMs = 1200000;
  // Waking lasts as long as its wake animation, never under 1 s (owner decision, PR #12 review):
  // Wake (800 ms) from Dozing, Yawn (1200 ms) from Asleep. Pinned against the renderer in the
  // router tests.
  static constexpr uint32_t kWakingFromDozeMs = 1000;
  static constexpr uint32_t kWakingFromSleepMs = 1200;

 private:
  SleepEffect enter(SleepState s);

  IClock& clock_;
  MoodEngine& mood_;
  SleepState state_ = SleepState::Awake;
  SleepState origin_ = SleepState::Dozing;
  uint32_t enteredMs_ = 0;   // when the current state started
  uint32_t awakeRefMs_ = 0;  // start of the current Tired spell, or last stimulus while Awake
  bool wasTired_ = false;
};

}  // namespace cubbies
