#include "core/SoundGate.h"

namespace cubbies {

namespace {

// Provisional note tables (05e: tuning by ear on the bench, tests check routing, not music).
constexpr Note kSurprise[] = {{1568, 60}, {2093, 90}};
constexpr Note kJoy[] = {{1319, 70}, {1568, 70}, {2093, 120}};
constexpr Note kPurr[] = {{196, 120}, {0, 40}, {196, 120}, {0, 40}, {196, 120}};
constexpr Note kHello[] = {{1047, 80}, {1319, 120}};
constexpr Note kYawn[] = {{784, 200}, {659, 200}, {523, 300}};
constexpr Note kGrumpy[] = {{220, 150}, {185, 250}};
constexpr Note kWakeMelody[] = {{523, 150}, {659, 150}, {784, 150}, {1047, 300}};
constexpr Note kRitualChime[] = {{1047, 100}, {1568, 100}, {2093, 160}};
constexpr Note kRitualTick[] = {{1760, 40}};  // one fixed pitch until the sound-design brick
constexpr Note kAdoption[] = {{784, 100}, {1047, 100}, {1319, 100}, {1568, 100}, {2093, 250}};
// Choir placeholder (05c): three voices of one 2.4 s phrase, same length, until the sound brick.
constexpr Note kChoirLow[] = {{262, 600}, {349, 600}, {392, 600}, {262, 600}};
constexpr Note kChoirMid[] = {{659, 300}, {784, 300}, {880, 300}, {784, 300}, {659, 300}, {587, 300}, {523, 600}};
constexpr Note kChoirHigh[] = {{1047, 300}, {1175, 300}, {1319, 300}, {1175, 300}, {1047, 300}, {988, 300}, {1047, 600}};

struct Sound {
  const Note* notes;
  uint8_t count;
  uint8_t rank;
};

template <uint8_t N>
constexpr Sound sound(const Note (&notes)[N], uint8_t rank) {
  return {notes, N, rank};
}

// Indexed by SoundId.
constexpr Sound kSounds[] = {
    sound(kSurprise, 0), sound(kJoy, 0),    sound(kPurr, 0),       sound(kHello, 0),
    sound(kYawn, 1),     sound(kGrumpy, 2), sound(kWakeMelody, 3),
    sound(kRitualChime, 0), sound(kRitualTick, 0), sound(kAdoption, 0),
    sound(kChoirLow, 2),    sound(kChoirMid, 2),   sound(kChoirHigh, 2),
};

static_assert(sizeof(kSounds) / sizeof(kSounds[0]) == kSoundCount,
              "kSounds must have one entry per SoundId");

constexpr bool fitsTheRestBuffer() {
  for (const Sound& s : kSounds)
    if (s.count > SoundGate::kMaxNotes) return false;
  return true;
}
static_assert(fitsTheRestBuffer(), "a note table is longer than SoundGate::kMaxNotes");

const Sound& lookup(SoundId id) { return kSounds[static_cast<uint8_t>(id)]; }

}  // namespace

SoundGate::SoundGate(IClock& clock, IAudio& audio) : clock_(clock), audio_(audio) {}

SoundId choirVoice(uint8_t voice) {
  return static_cast<SoundId>(static_cast<uint8_t>(SoundId::ChoirLow) + voice % kChoirVoices);
}

uint8_t SoundGate::rank(SoundId id) { return lookup(id).rank; }

uint32_t SoundGate::durationMs(SoundId id) {
  const Sound& s = lookup(id);
  uint32_t total = 0;
  for (uint8_t i = 0; i < s.count; ++i) total += s.notes[i].ms;
  return total;
}

bool SoundGate::play(SoundId id, uint32_t restMs) {
  if (sleep_ != SleepState::Awake && sleep_ != SleepState::Waking) return false;
  if (playing_ && rank(id) <= rank(current_)) return false;
  const Sound& s = lookup(id);
  // The buzzer's timer task may still read buffer_: stop it before rewriting (IAudio contract).
  if (playing_ && restMs_ > 0) audio_.stop();
  restMs_ = restMs < kMaxRestMs ? restMs : kMaxRestMs;
  if (restMs_ == 0) {
    audio_.play(s.notes, s.count, kVolume);
  } else {
    buffer_[0] = Note{0, static_cast<uint16_t>(restMs_)};
    for (uint8_t i = 0; i < s.count; ++i) buffer_[i + 1] = s.notes[i];
    audio_.play(buffer_, static_cast<uint8_t>(s.count + 1), kVolume);
  }
  playing_ = true;
  current_ = id;
  startMs_ = clock_.nowMs();
  return true;
}

void SoundGate::setSleepState(SleepState s) {
  sleep_ = s;
  if (playing_ && (s == SleepState::Dozing || s == SleepState::Asleep)) {
    audio_.stop();
    playing_ = false;
  }
}

void SoundGate::tick() {
  if (playing_ && ageMs(clock_.nowMs(), startMs_) >= restMs_ + durationMs(current_)) playing_ = false;
}

}  // namespace cubbies
