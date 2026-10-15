#include "core/AdoptionRitual.h"

namespace cubbies {

namespace {

IdentityOutput frame(IdentityFrame f, Rgb565 colour) {
  IdentityOutput o;
  o.frame = f;
  o.colour = colour;
  return o;
}

IdentityOutput withSound(IdentityOutput o, SoundId sound) {
  o.hasSound = true;
  o.sound = sound;
  return o;
}

}  // namespace

AdoptionRitual::AdoptionRitual(IClock& clock, IStorage& storage) : clock_(clock), storage_(storage) {}

IdentityOutput AdoptionRitual::start(const Identity& identity) {
  identity_ = identity;
  candidate_ = identity.paletteIndex;
  lastMs_ = clock_.nowMs();
  state_ = State::Cycling;
  return withSound(frame(IdentityFrame::Ritual, paletteColour(candidate_)), SoundId::RitualChime);
}

IdentityOutput AdoptionRitual::tick(Gesture gesture) {
  if (state_ != State::Cycling) return {};
  const uint32_t now = clock_.nowMs();

  if (gesture == Gesture::None) {
    if (ageMs(now, lastMs_) < kTimeoutMs) return {};
    state_ = State::Done;  // abandoned: identity unchanged
    return frame(IdentityFrame::Clear, kBlack);
  }
  lastMs_ = now;

  switch (gesture) {
    case Gesture::Tap:
    case Gesture::DoubleTap:
      candidate_ = (candidate_ + (gesture == Gesture::Tap ? 1 : 2)) % kPaletteSize;
      return withSound(frame(IdentityFrame::Ritual, paletteColour(candidate_)), SoundId::RitualTick);

    case Gesture::Stroke:
      state_ = State::Done;
      if (!storage_.writeU8(kColourKey, candidate_)) return frame(IdentityFrame::Error, kBlack);
      identity_.paletteIndex = candidate_;
      identity_.adopted = true;
      return withSound(frame(IdentityFrame::Adoption, paletteColour(candidate_)), SoundId::Adoption);

    case Gesture::None:
    case Gesture::TouchDown:
    case Gesture::Released:
      return {};
  }
  return {};
}

}  // namespace cubbies
