// Transient reactions drawn as an overlay on the mood frame (05a: "reactions are not states").
// Timed reactions end on their own (reactionDurationMs); TouchDown and Purr are held while the
// pad is pressed and end with stopReaction(). Sources: 05a transition table, 05b Waking,
// 02-sequence-presence (greeting), use cases UC3 / UC4 / UC7.
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
};

}  // namespace cubbies
