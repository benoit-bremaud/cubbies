#include "core/Mood.h"

namespace cubbies {

bool pulledMood(const MoodTally& tally, Mood& out) {
  // Tie-break priority, highest first; Tired is absent on purpose (05a row 9).
  static constexpr Mood kPriority[] = {Mood::Happy, Mood::Annoyed, Mood::Curious, Mood::Bored};
  uint8_t best = 0;
  for (Mood m : kPriority) {
    const uint8_t v = tally.votes[static_cast<uint8_t>(m)];
    if (v > best) {
      best = v;
      out = m;
    }
  }
  return best > 0;
}

}  // namespace cubbies
