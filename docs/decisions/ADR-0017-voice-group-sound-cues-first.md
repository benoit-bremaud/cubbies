# ADR-0017: Voice — group sound cues on the V1 hardware first, no words, no cloud

**Status:** Proposed — 2026-10-23

## Context

The owner wants cubbies to interact with humans by voice in an advanced version, maybe with an AI
later (issue #61). The V1 hardware is frozen (ADR-0011): an ESP32-C3 without PSRAM, an INMP441
microphone, a passive buzzer. Espressif's ESP-SR does not target the C3; TinyML keyword spotting
fits only a tiny vocabulary there. An always-on microphone in an office raises GDPR questions.
The research note [voice-and-ai.md](../research/voice-and-ai.md) compares three directions with
an MCDA matrix (identity 30, feasibility 25, privacy 20, wow 15, BOM 10): group effect 460/500,
prosody 395/500, voice commands through an ear cube 360/500. The owner chose the group effect.

## Options and scoring

Weights: identity 30, feasibility on V1 25, privacy 20, wow 15, BOM/power 10 (detail in the
research note).

| Option | Total /500 |
|--------|-----------|
| A. Group sound cues, no words | **460** — chosen |
| C. Prosody reactions per cube | 395 |
| B. Voice commands through an S3 ear cube (V1.5) | 360 |

## Decision

- **First voice brick = group sound cues:** a human sound (clap, knock, loud call) heard by the
  cubes triggers a group behaviour. Detection by energy and spectral features, no speech
  recognition, no model.
- **Audio never leaves the cube and is never stored:** features are computed per frame in RAM and
  discarded; no audio on ESP-NOW.
- **The detector is pure logic in `firmware/lib/`** fed with feature frames, tested natively; the
  I2S capture stays a thin adapter.
- **Deferred:** prosody reactions (same hardware, after the cues), voice commands through a V1.5
  ear cube, any cloud speech or LLM (mother cube only, secrets in NVS, owner-approved privacy
  notice), an S3 in every cube (V4).

## Consequences

- No BOM change, no new hardware, one firmware still holds (ADR-0008).
- Self-hearing must be handled with the buzzer (issue #16) and thresholds measured on the bench.
- The roadmap S7 "sound reactions" gains a group variant; a new social item is added.
- A later ADR supersedes this one if words or a cloud link are ever adopted.
