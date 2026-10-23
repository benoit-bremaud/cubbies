// The five discrete moods of the mood engine (05a). Values are the wire encoding 0..4; the
// presence frame adds 5 = asleep, which is a sleep-cycle state, not a mood.
#pragma once

#include <cstdint>

namespace cubbies {

enum class Mood : uint8_t { Curious = 0, Happy = 1, Bored = 2, Tired = 3, Annoyed = 4 };

constexpr uint8_t kMoodCount = 5;

// Votes of the present, awake neighbours per mood (social-group/05a), filled by the Social
// protocol and read by the Mood engine, so neither knows the other.
struct MoodTally {
  uint8_t votes[kMoodCount] = {};
  uint8_t drowsy = 0;  // present neighbours Tired, dozing or asleep (mood byte 3 or 5, 05d)
};

// The mood most neighbours show, ties broken Happy > Annoyed > Curious > Bored; Tired never
// votes. False when nobody votes.
bool pulledMood(const MoodTally& tally, Mood& out);

}  // namespace cubbies
