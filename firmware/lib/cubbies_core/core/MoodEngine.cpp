#include "core/MoodEngine.h"

namespace cubbies {

MoodEngine::MoodEngine(IClock& clock) : clock_(clock) { restart(); }

void MoodEngine::restart() {
  const uint32_t now = clock_.nowMs();
  mood_ = Mood::Curious;
  enteredMs_ = lastAnyMs_ = lastLoudMs_ = lastOwnerMs_ = lastYawnMs_ = now;
  holding_ = yawnHolding_ = false;  // a wake starts with no candidate; the refractory period is kept
  noticed_ = false;                 // 05e rows 5, 10: the jealousy cooldown and ring are kept
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
  if (s != Stimulus::NewcomerSeen) lastLoudMs_ = lastOwnerMs_ = now;  // gestures and sounds break the calm

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

bool MoodEngine::catchesYawn(const MoodTally& tally, uint32_t now) {
  if (mood_ != Mood::Bored || tally.drowsy == 0) {
    yawnHolding_ = false;
    return false;
  }
  if (!yawnHolding_) {
    yawnHolding_ = true;
    yawnSinceMs_ = now;
    return false;
  }
  return ageMs(now, yawnSinceMs_) >= kContagionMs && ageMs(now, lastLoudMs_) >= kContagionMs;
}

MoodEffect MoodEngine::contagion(const MoodTally& tally) {
  const uint32_t now = clock_.nowMs();
  if (catchesYawn(tally, now)) {
    yawnHolding_ = holding_ = false;
    lastLoudMs_ = now;  // a G2 pull out of Tired waits a fresh calm (05d row 11)
    return enter(Mood::Tired);
  }
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

uint8_t MoodEngine::recentStep2(uint32_t now, uint32_t windowMs) const {
  uint8_t n = 0;
  for (uint8_t i = 1; i <= step2Count_; ++i)  // the newest step2Count_ slots, behind the head
    if (ageMs(now, step2Ms_[(step2Head_ + kJealousBudget - i) % kJealousBudget]) <= windowMs) ++n;
  return n;
}

// Every recorded step 2 is out of the longest window (T_weary): forget them before they can
// alias (2^32 ms). The budget still reads the entries against kJealousWindowMs (row 16b).
void MoodEngine::forgetStaleRing(uint32_t now) {
  if (step2Count_ == 0) return;
  const uint8_t last = static_cast<uint8_t>((step2Head_ + kJealousBudget - 1) % kJealousBudget);
  if (ageMs(now, step2Ms_[last]) > kWearyMs) step2Count_ = 0;
}

bool MoodEngine::jealousyBlocked(uint32_t now) {
  forgetStaleRing(now);
  if (step2Count_ == 0) return false;
  const uint8_t last = static_cast<uint8_t>((step2Head_ + kJealousBudget - 1) % kJealousBudget);
  return ageMs(now, step2Ms_[last]) < kJealousMs;  // row 3
}

JealousContexts MoodEngine::contexts(const PetContext& pet) const {
  const uint32_t now = clock_.nowMs();
  JealousContexts c;
  c.bored = mood_ == Mood::Bored;
  c.crowd = pet.neighbours >= 2;
  c.newcomer = pet.strokedAgeMs < kFriendMs;
  c.friend_ = !c.newcomer;
  c.neglected = ageMs(now, lastOwnerMs_) >= kLonelyMs;
  // The ring is cleared past T_weary by forgetStaleRing(); an uncleared stale entry never counts.
  c.weary = recentStep2(now, kWearyMs) >= kWearyCount;
  return c;
}

JealousReaction MoodEngine::jealousy(bool padHeld, const PetContext& pet, JealousyRepertoire& repertoire) {
  const uint32_t now = clock_.nowMs();
  if (padHeld || ageMs(now, lastLoudMs_) < kJealousCalmMs) {  // rows 2, 2b
    // The owner is there: the notice is dropped; a calm broken only by a pull or a yawn keeps it.
    if (padHeld || ageMs(now, lastOwnerMs_) < kJealousCalmMs) noticed_ = false;
    return JealousReaction::None;
  }
  if (jealousyBlocked(now)) return JealousReaction::None;  // row 3: Noticed is not entered either
  if (noticed_ && noticeMs_ == now) return JealousReaction::None;  // row 6: one step per tick
  const bool stillNoticed = noticed_ && ageMs(now, noticeMs_) < kNoticeMs &&  // row 12
                            ageMs(now, lastOwnerMs_) > ageMs(now, noticeMs_);  // row 12b
  if (!stillNoticed) {  // row 1: no cooldown or budget entry
    noticed_ = true;
    noticeMs_ = now;
    return repertoire.draw(JealousStep::Notice, contexts(pet));
  }
  // Row 4: full ring, step2Head_ is the oldest of the last kJealousBudget (strict window).
  if (step2Count_ == kJealousBudget && ageMs(now, step2Ms_[step2Head_]) <= kJealousWindowMs)
    return JealousReaction::None;  // stays Noticed until T_notice elapses
  const JealousContexts c = contexts(pet);  // Weary before this step 2 is recorded
  step2Ms_[step2Head_] = now;
  step2Head_ = static_cast<uint8_t>((step2Head_ + 1) % kJealousBudget);
  if (step2Count_ < kJealousBudget) ++step2Count_;
  noticed_ = false;
  return repertoire.draw(JealousStep::Jealous, c);  // rows 1b, 7, 8: no timer, calm or refractory touched
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
