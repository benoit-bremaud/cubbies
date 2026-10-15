// Stimulus router (03-component): pure functions that say where each classified input goes and
// which generic reaction and sound it produces (S3, 05a reaction rows), given the Sleep cycle
// state. While Awake, gestures, sounds and NewcomerSeen go to the Mood engine, and gestures and
// sounds also feed the Sleep cycle's idle timer. While not Awake, gestures and sounds go to the
// Sleep cycle only and NewcomerSeen is dropped. Two more mappings turn the effects returned by
// the Mood engine and the Sleep cycle into eyes and sound commands; main.cpp applies them.
#pragma once

#include <cstdint>

#include "core/GestureClassifier.h"
#include "core/MoodEngine.h"
#include "core/SleepCycle.h"
#include "core/SoundClassifier.h"
#include "core/SoundGate.h"
#include "eyes/Reaction.h"

namespace cubbies {

enum class Lids : uint8_t { Keep, Close, Open };

// What to do with one input or effect. A flag off means "nothing on that output".
struct Route {
  bool toMood = false;
  Stimulus moodStimulus = Stimulus::Tap;
  bool toSleep = false;
  bool react = false;
  Reaction reaction = Reaction::TouchDown;
  bool stopReaction = false;  // ends a held reaction (TouchDown, Purr)
  bool sound = false;
  SoundId soundId = SoundId::Surprise;
  Lids lids = Lids::Keep;
};

Route route(Gesture g, SleepState sleep);
Route route(SoundClass s, SleepState sleep);
Route routeNewcomer(SleepState sleep);

Route effects(MoodEffect e);   // yawn, grumpy beep (05a)
Route effects(SleepEffect e);  // lids, wake animation, wake-up melody (05b); brightness is read
                               // from SleepCycle::brightness()

}  // namespace cubbies
