// The five discrete moods of the mood engine (05a). Values are the wire encoding 0..4; the
// presence frame adds 5 = asleep, which is a sleep-cycle state, not a mood.
#pragma once

#include <cstdint>

namespace cubbies {

enum class Mood : uint8_t { Curious = 0, Happy = 1, Bored = 2, Tired = 3, Annoyed = 4 };

constexpr uint8_t kMoodCount = 5;

}  // namespace cubbies
