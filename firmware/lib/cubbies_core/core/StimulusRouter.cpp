#include "core/StimulusRouter.h"

namespace cubbies {

namespace {

Route withReaction(Route r, Reaction x) {
  r.react = true;
  r.reaction = x;
  return r;
}

Route withSound(Route r, SoundId id) {
  r.sound = true;
  r.soundId = id;
  return r;
}

Route toMood(Stimulus s, bool feedsSleep) {
  Route r;
  r.toMood = true;
  r.moodStimulus = s;
  r.toSleep = feedsSleep;
  return r;
}

Route sleepOnly() {
  Route r;
  r.toSleep = true;
  return r;
}

}  // namespace

Route route(Gesture g, SleepState sleep) {
  if (g == Gesture::None) return {};
  if (sleep != SleepState::Awake) return sleepOnly();
  switch (g) {
    case Gesture::TouchDown:
      return withReaction(sleepOnly(), Reaction::TouchDown);
    case Gesture::Tap:
      return withSound(withReaction(toMood(Stimulus::Tap, true), Reaction::Surprise), SoundId::Surprise);
    case Gesture::DoubleTap:
      return withSound(withReaction(toMood(Stimulus::DoubleTap, true), Reaction::Joy), SoundId::Joy);
    case Gesture::Stroke:
      return withSound(withReaction(toMood(Stimulus::Stroke, true), Reaction::Purr), SoundId::Purr);
    case Gesture::Released: {
      Route r = toMood(Stimulus::Released, true);
      r.stopReaction = true;
      return r;
    }
    case Gesture::None:
      break;
  }
  return {};
}

Route route(SoundClass s, SleepState sleep) {
  if (s == SoundClass::None) return {};
  if (sleep != SleepState::Awake) return sleepOnly();
  switch (s) {
    case SoundClass::Voice:
      return withReaction(toMood(Stimulus::Voice, true), Reaction::LookUp);
    case SoundClass::Clap:
      return withReaction(toMood(Stimulus::Clap, true), Reaction::LookUp);
    case SoundClass::Loud:
      return toMood(Stimulus::Loud, true);
    case SoundClass::None:
      break;
  }
  return {};
}

Route routeNewcomer(SleepState sleep) {
  if (sleep != SleepState::Awake) return {};
  return withSound(withReaction(toMood(Stimulus::NewcomerSeen, false), Reaction::Greet), SoundId::Hello);
}

Route routeChain(SleepState sleep) {
  if (sleep != SleepState::Awake) return {};
  return withSound(withReaction({}, Reaction::Joy), SoundId::Joy);
}

Route effects(MoodEffect e) {
  switch (e) {
    case MoodEffect::Yawn:
      return withSound(withReaction({}, Reaction::Yawn), SoundId::Yawn);
    case MoodEffect::Grumpy:
      return withSound({}, SoundId::Grumpy);
    case MoodEffect::None:
      break;
  }
  return {};
}

Route effects(SleepEffect e) {
  Route r;
  switch (e) {
    case SleepEffect::FellAsleep:
      r.lids = Lids::Close;
      return r;
    case SleepEffect::WokeFromDoze:
      r.lids = Lids::Open;
      return withReaction(r, Reaction::Wake);
    case SleepEffect::WokeFromSleep:
      r.lids = Lids::Open;
      return withSound(withReaction(r, Reaction::Yawn), SoundId::WakeMelody);
    case SleepEffect::None:
    case SleepEffect::Dozed:
    case SleepEffect::Awoke:
      break;
  }
  return r;
}

}  // namespace cubbies
