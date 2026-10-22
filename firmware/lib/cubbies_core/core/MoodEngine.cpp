#include "core/MoodEngine.h"

namespace cubbies {

MoodEngine::MoodEngine(IClock& clock) : clock_(clock) { restart(); }

void MoodEngine::restart() {
  const uint32_t now = clock_.nowMs();
  mood_ = Mood::Curious;
  enteredMs_ = lastAnyMs_ = lastLoudMs_ = lastYawnMs_ = now;
  holding_ = false;  // a wake starts with no candidate; the refractory period is kept
}

MoodEffect MoodEngine::enter(Mood m) {
  mood_ = m;
  enteredMs_ = clock_.nowMs();
  if (m == Mood::Tired) {
    lastYawnMs_ = enteredMs_;
    return MoodEffect::Yawn;  // first yawn
  }
  return m == Mood::Annoyed ? MoodEffect::Grumpy : MoodEffect::None;
}

MoodEffect MoodEngine::stimulus(Stimulus s) {
  const uint32_t now = clock_.nowMs();
  lastAnyMs_ = now;
  if (s != Stimulus::NewcomerSeen) lastLoudMs_ = now;  // gestures and sounds break the calm

  switch (mood_) {
    case Mood::Curious:
      if (s == Stimulus::Tap || s == Stimulus::DoubleTap) return enter(Mood::Happy);
      if (s == Stimulus::Loud) return enter(Mood::Annoyed);
      return MoodEffect::None;
    case Mood::Happy:
      if (s == Stimulus::Loud) return enter(Mood::Annoyed);
      return MoodEffect::None;
    case Mood::Bored:
    case Mood::Tired:
      return enter(Mood::Curious);
    case Mood::Annoyed:
      if (s == Stimulus::Stroke) return enter(Mood::Happy);
      return MoodEffect::None;
  }
  return MoodEffect::None;
}

MoodEffect MoodEngine::contagion(const MoodTally& tally) {
  const uint32_t now = clock_.nowMs();
  Mood target = mood_;
  if (!pulledMood(tally, target) || target == mood_) {
    holding_ = false;
    return MoodEffect::None;
  }
  if (!holding_ || target != candidate_) {
    holding_ = true;
    candidate_ = target;
    holdSinceMs_ = now;
    return MoodEffect::None;
  }
  if (ageMs(now, holdSinceMs_) < kContagionMs || ageMs(now, lastLoudMs_) < kContagionMs) return MoodEffect::None;
  if (pulled_ && ageMs(now, lastPullMs_) < kRefractoryMs) return MoodEffect::None;
  holding_ = false;
  pulled_ = true;
  lastPullMs_ = lastAnyMs_ = lastLoudMs_ = now;  // the pulled mood lasts its natural duration
  return enter(target);
}

MoodEffect MoodEngine::tick() {
  const uint32_t now = clock_.nowMs();
  switch (mood_) {
    case Mood::Curious:
      if (ageMs(now, lastAnyMs_) >= kBoredMs) return enter(Mood::Bored);
      break;
    case Mood::Happy:
      if (ageMs(now, lastLoudMs_) >= kSettleMs) return enter(Mood::Curious);
      break;
    case Mood::Bored:
      if (ageMs(now, enteredMs_) >= kTiredMs) return enter(Mood::Tired);
      break;
    case Mood::Tired:
      if (ageMs(now, lastYawnMs_) >= kYawnMs) {
        lastYawnMs_ = now;
        return MoodEffect::Yawn;
      }
      break;
    case Mood::Annoyed:
      if (ageMs(now, lastLoudMs_) >= kCalmMs) return enter(Mood::Curious);
      break;
  }
  return MoodEffect::None;
}

}  // namespace cubbies
