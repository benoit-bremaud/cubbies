// Transient reactions drawn as an overlay on the mood frame (05a: "reactions are not states").
// Each has a duration (Purr holds until stopReaction) and a shape rule in EyesRenderer.
#pragma once

#include <cstdint>

namespace cubbies {

enum class Reaction : uint8_t {
  TouchDown,  // pressed edge: eyes widen one notch, ≤ 150 ms (REQ-NF-01)
  Surprise,   // tap: round, big eyes
  Joy,        // double tap: happy arcs bouncing twice
  Purr,       // long stroke: soft closed arcs, held while touched
  LookUp,     // voice or clap: eyes up and slightly wider (UC4)
  Yawn,       // tired / waking: slow close then wide open
  Wake,       // from asleep: progressive open and a stretch
  Greet,      // newcomer seen: quick sideways glance
};

}  // namespace cubbies
