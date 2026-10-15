#include "core/SleepCycle.h"

namespace cubbies {

SleepCycle::SleepCycle(IClock& clock, MoodEngine& mood) : clock_(clock), mood_(mood) {
  enteredMs_ = awakeRefMs_ = clock_.nowMs();
}

Brightness SleepCycle::brightness() const {
  switch (state_) {
    case SleepState::Awake:
      return Brightness::Full;
    case SleepState::Dozing:
      return Brightness::Low;
    case SleepState::Asleep:
      return Brightness::Minimum;
    case SleepState::Waking:  // unchanged until Awake (05b table)
      return origin_ == SleepState::Asleep ? Brightness::Minimum : Brightness::Low;
  }
  return Brightness::Full;
}

SleepEffect SleepCycle::enter(SleepState s) {
  const uint32_t now = clock_.nowMs();
  const SleepState from = state_;
  state_ = s;
  enteredMs_ = now;
  switch (s) {
    case SleepState::Dozing:
      return SleepEffect::Dozed;
    case SleepState::Asleep:
      return SleepEffect::FellAsleep;
    case SleepState::Waking:
      origin_ = from;
      return from == SleepState::Asleep ? SleepEffect::WokeFromSleep : SleepEffect::WokeFromDoze;
    case SleepState::Awake:
      mood_.restart();
      awakeRefMs_ = now;
      wasTired_ = false;
      return SleepEffect::Awoke;
  }
  return SleepEffect::None;
}

SleepEffect SleepCycle::gesture(Gesture g) {
  if (g == Gesture::None) return SleepEffect::None;
  switch (state_) {
    case SleepState::Awake:
      awakeRefMs_ = clock_.nowMs();
      return SleepEffect::None;
    case SleepState::Dozing:
    case SleepState::Asleep:
      // Any contact starts with a TouchDown; later gestures of the same press are not a wake.
      return g == Gesture::TouchDown ? enter(SleepState::Waking) : SleepEffect::None;
    case SleepState::Waking:
      return SleepEffect::None;
  }
  return SleepEffect::None;
}

SleepEffect SleepCycle::sound(SoundClass s) {
  if (s == SoundClass::None) return SleepEffect::None;
  switch (state_) {
    case SleepState::Awake:
      awakeRefMs_ = clock_.nowMs();
      return SleepEffect::None;
    case SleepState::Dozing:  // voice, clap and loud are all above the wake threshold
      return enter(SleepState::Waking);
    case SleepState::Asleep:  // only loud reaches the deep-wake threshold
      return s == SoundClass::Loud ? enter(SleepState::Waking) : SleepEffect::None;
    case SleepState::Waking:
      return SleepEffect::None;
  }
  return SleepEffect::None;
}

SleepEffect SleepCycle::tick() {
  const uint32_t now = clock_.nowMs();
  switch (state_) {
    case SleepState::Awake: {
      const bool tired = mood_.mood() == Mood::Tired;
      if (tired && !wasTired_) awakeRefMs_ = now;  // a new Tired spell
      wasTired_ = tired;
      if (tired && ageMs(now, awakeRefMs_) >= kDozeMs) return enter(SleepState::Dozing);
      break;
    }
    case SleepState::Dozing:
      if (ageMs(now, enteredMs_) >= kFallAsleepMs) return enter(SleepState::Asleep);
      break;
    case SleepState::Asleep:
      break;
    case SleepState::Waking:
      if (ageMs(now, enteredMs_) >= kWakingMs) return enter(SleepState::Awake);
      break;
  }
  return SleepEffect::None;
}

}  // namespace cubbies
