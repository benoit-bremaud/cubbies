// Shared radio-facing value types, kept out of core/ so ports depend on nothing in the domain.
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace cubbies {

using Mac = std::array<uint8_t, 6>;

// Length of every v1 frame (social-presence/06); the radio adapter compares against it.
constexpr size_t kFrameSize = 8;

}  // namespace cubbies
