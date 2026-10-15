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
  // Voice and loud are timed on their own runs of samples at level, tracked in every state, so a
  // run starts on the sample that enters Sound (or during a Burst that turns into a sound), and a
  // low-rms bang lends no time to a voice that rises after it; a clap ending resets both.
  voiceRun_.update(voiceLevel, now);
  loudRun_.update(loudLevel, now);

  switch (state_) {
    case State::Quiet:
      if (clapPeak) {
        state_ = State::Burst;
        sinceMs_ = now;
      } else if (voiceLevel) {
        state_ = State::Sound;
        voiceSent_ = loudSent_ = false;
      } else {  // learn only in quiet: running mean over kBaselineTicks
        const int32_t delta = static_cast<int32_t>(static_cast<uint32_t>(in.rms) << 8) -
                              static_cast<int32_t>(baselineQ8_);
        baselineQ8_ = static_cast<uint32_t>(static_cast<int32_t>(baselineQ8_) +
                                            delta / static_cast<int32_t>(kBaselineTicks));
      }
      return SoundClass::None;

    case State::Burst:
      // Age first: a burst that reached T_clap is a long bang, a sound, even if its peak falls on
      // this very tick (05d); its voice and loud runs are kept.
      if (ageMs(now, sinceMs_) >= kClapMs) {
        state_ = State::Sound;
        voiceSent_ = loudSent_ = false;
        return SoundClass::None;
      }
      if (!clapPeak) {
        state_ = State::Quiet;
        voiceRun_.on = loudRun_.on = false;  // the clap is over: a tail is a new sound (05d)
        return SoundClass::Clap;
      }
      return SoundClass::None;

    case State::Sound:
      if (!voiceLevel) {
        state_ = State::Quiet;
        return SoundClass::None;
      }
      if (!voiceSent_ && voiceRun_.lastedMs(now) >= kVoiceMs) {
        voiceSent_ = true;
        return SoundClass::Voice;
      }
      // Loud only when the loud level itself has lasted T_loud (05d), not the whole sound.
      if (!loudSent_ && loudRun_.on && loudRun_.lastedMs(now) >= kLoudMs) {
        loudSent_ = true;
        return SoundClass::Loud;
      }
      return SoundClass::None;
  }
  return SoundClass::None;
}

}  // namespace cubbies
