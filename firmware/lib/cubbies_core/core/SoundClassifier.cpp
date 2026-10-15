#include "core/SoundClassifier.h"

namespace cubbies {

SoundClassifier::SoundClassifier(IClock& clock, IMic& mic) : clock_(clock), mic_(mic) {}

SoundClass SoundClassifier::tick() {
  const uint32_t now = clock_.nowMs();
  const MicLevels in = mic_.read();
  if (!seeded_) {
    baselineQ8_ = static_cast<uint32_t>(in.rms) << 8;
    seeded_ = true;
  }
  const uint32_t baseline = baselineQ8_ >> 8;
  const uint32_t ref = baseline > kMinLevel ? baseline : kMinLevel;
  const bool clapPeak = in.peak >= kClapRatio * ref;
  const bool voiceLevel = in.rms >= kVoiceRatio * ref;
  const bool loudLevel = in.rms >= kLoudRatio * ref;
  const uint32_t age = ageMs(now, sinceMs_);

  switch (state_) {
    case State::Quiet:
      if (clapPeak) {
        state_ = State::Burst;
        sinceMs_ = now;
      } else if (voiceLevel) {
        state_ = State::Sound;
        sinceMs_ = now;
        voiceSent_ = loudSent_ = false;
      } else {  // learn only in quiet: running mean over kBaselineTicks
        const int32_t delta = static_cast<int32_t>(static_cast<uint32_t>(in.rms) << 8) -
                              static_cast<int32_t>(baselineQ8_);
        baselineQ8_ = static_cast<uint32_t>(static_cast<int32_t>(baselineQ8_) +
                                            delta / static_cast<int32_t>(kBaselineTicks));
      }
      return SoundClass::None;

    case State::Burst:
      if (!clapPeak) {
        state_ = State::Quiet;
        return age < kClapMs ? SoundClass::Clap : SoundClass::None;
      }
      if (age >= kClapMs) {  // a long bang: treat it as a sound from the burst start
        state_ = State::Sound;
        voiceSent_ = loudSent_ = false;
      }
      return SoundClass::None;

    case State::Sound:
      if (!voiceLevel) {
        state_ = State::Quiet;
        return SoundClass::None;
      }
      if (!voiceSent_ && age >= kVoiceMs) {
        voiceSent_ = true;
        return SoundClass::Voice;
      }
      if (!loudSent_ && loudLevel && age >= kLoudMs) {
        loudSent_ = true;
        return SoundClass::Loud;
      }
      return SoundClass::None;
  }
  return SoundClass::None;
}

}  // namespace cubbies
