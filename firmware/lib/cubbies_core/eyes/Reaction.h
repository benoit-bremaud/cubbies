// Transient reactions drawn as an overlay on the mood frame (05a: "reactions are not states").
// Timed reactions end on their own (reactionDurationMs); TouchDown and Purr are held while the
// pad is pressed and end with stopReaction(). Sources: 05a transition table, 05b Waking,
// 02-sequence-presence (greeting), social-group/05e (jealousy), use cases UC3 / UC4 / UC7.
#pragma once

#include <cstdint>

namespace cubbies {

enum class Reaction : uint8_t {
  TouchDown,  // pressed edge: eyes widen one notch within the next tick (REQ-NF-01 latency);
              // held until release, so a tap never flickers before its classified reaction
  Surprise,   // tap (05a: Curious → Happy "surprise beep", UC3)
  Joy,        // double tap: happy arcs bouncing twice (05a, UC3)
  Purr,       // long stroke: soft closed arcs "^ ^", held until stopReaction() (05a Happy / Annoyed rows, UC3)
  LookUp,     // voice or clap: eyes up and slightly wider (05a "any" row, UC4)
  Yawn,       // slow close then wide open (05a Bored → Tired "first yawn", Tired every T_yawn, 05b Asleep → Waking)
  Wake,       // progressive open and a stretch (05b Waking from Dozing)
  Greet,      // NewcomerSeen: glance one side then the other (05a, 02-presence, UC7)
  Sulk,       // jealousy: annoyed lids, a glance away and down (social-group/05e)
  // G6a jealousy repertoire (social-group/05e), T_react each. Step 1, silent look offsets:
  SideGlance, // both eyes slide to the side of the last glance and hold, top lids a little low
  Stare,      // held at a side look offset, blink suspended, then three fast blinks
  TurnAway,   // largest look offset, opposite the side of the last glance
  // Step 2:
  Pout,       // lower lids raised, eyes narrowed, inner corners a little low
  PuppyEyes,  // eyes enlarged with a highlight, lids tilted down at the outer corners
  MeToo,      // eyes hop up and down
  FakeSleep,  // eyes shut, one eye half-opens and peeks sideways, shuts again
};

}  // namespace cubbies
