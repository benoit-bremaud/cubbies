// What the invitation window and the adoption ritual ask main.cpp to show and play on a tick
// (identity/05a, 05b): main applies it to the Eyes renderer and the Sound gate, like a Route.
#pragma once

#include <cstdint>

#include "core/SoundGate.h"
#include "eyes/Framebuffer.h"

namespace cubbies {

// Keep: leave the current frame. Otherwise the matching EyesRenderer identity call.
enum class IdentityFrame : uint8_t { Keep, Invitation, Ritual, Adoption, Error, Clear };

struct IdentityOutput {
  IdentityFrame frame = IdentityFrame::Keep;
  uint8_t progress = 0;   // Invitation: gauge 0..255
  Rgb565 colour{0};       // Ritual: candidate, Adoption: chosen
  bool hasSound = false;
  SoundId sound = SoundId::RitualTick;
};

}  // namespace cubbies
