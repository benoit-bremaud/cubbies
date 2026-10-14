# ADR-0013: cube-core PDR decisions — lib-owned PRNG, invitation window and ritual volume step, sleep timings, deferrals

**Status:** Accepted — 2026-10-14 · Amends ADR-0008 (ritual entry)

## Context

The preliminary design review of `cube-core` ([gates.md](../architecture/gates.md)) cross-checked the eight validated
diagrams against `specs/requirements.md` and found five blocking contradictions (two use-case
numberings, an adoption ritual described differently in the spec and the diagrams, a touch
latency that tap/double-tap classification cannot meet, sleep timings that did not sum to FS5,
and three tier-A requirements neither modelled nor formally deferred) plus design questions left
open by the UML study. Facts verified for this review:

- The ESP32-C3 RNG is only pseudo-random until the RF subsystem is up; `esp_random()` is true
  random once Wi-Fi is initialised. xorshift32 fails PractRand; PCG32 passes with 64-bit state
  and one multiply.
- The TTP223 calibrates its baseline during the first 0.5 s after power and its datasheet forbids
  touching the pad during that window: a finger already on the pad at power-up may be calibrated
  away and never detected. The validated "hold while plugging in" entry is therefore unreliable on
  the real part.
- The TTP223 is a binary pad: the gesture alphabet is taps and press durations only.
- Tap classification must wait for the double-tap window (≈ 250 ms), above the 150 ms latency
  budget of REQ-NF-01.

## Decisions (owner, scored options in the PDR session of 2026-10-14)

### D1 — Randomness: lib-owned PCG32 instead of an `IRng` port

| Criterion (weight) | A. `IRng` port | B'. PCG32 in lib, seeded from `esp_random()` after Wi-Fi init | C. PCG32 seeded from hash(MAC) |
|---|---|---|---|
| KISS (3) | 1 | 3 | 3 |
| Determinism in tests (3) | 3 | 3 | 3 |
| No single-implementation port (2) | 1 | 3 | 3 |
| Statistical quality (2) | 3 | 3 | 3 |
| V2 fleet key (2) | 2 | 2 | 1 |
| **Total /36** | 24 | **34** | 32 |

One `Prng` instance per consumer (eyes, mood, social jitter), seeded by `main.cpp`; tests pass a
fixed seed. Keys (V2) never come from this PRNG.

### D2 — Ritual entry and volume: invitation window after boot, volume as a second ritual step

The ritual is entered by holding the pad during an **invitation window** shown after boot
(question mark + gauge, T_invite 4 s, after T_settle_touch 1 s), not by holding while plugging
in. Volume options:

| Criterion (weight) | A. nothing reserved | B. triple tap | C'. longer hold in the window | D. mother cube (V1.5) | E. second step of the ritual |
|---|---|---|---|---|---|
| YAGNI (3) | 3 | 2 | 1 | 3 | 2 |
| No collision with awake gestures (3) | 1 | 2 | 3 | 3 | 3 |
| Tap latency preserved (2) | 3 | 1 | 3 | 3 | 3 |
| Discoverable by a colleague (2) | 1 | 2 | 1 | 1 | 3 |
| **Total /30** | 20 | 18 | 20 | 26 | **27** |

D (mother cube) scores 26 but depends on V1.5 infrastructure; E is chosen because it ships in
V1.x without it and keeps the volume in the colleague's hands.

V1 ships a constant volume; V1.x adds the step (tap cycles four levels with a sample beep, long
stroke confirms, `settings.volume` in NVS read by the Sound gate).

### D3 — Sleep timings: doze at 10 min, sleep at 30 min, any stimulus leaves Tired

| Criterion (weight) | A. keep ~15/~20 min, amend FS5 | B. retune to 10/30 (FS5) | C. 10/20 |
|---|---|---|---|
| Fidelity to FS5 (2) | 1 | 3 | 2 |
| Nuanced absences (coffee break vs meeting) (3) | 1 | 3 | 2 |
| Chain coherence (2) | 3 | 3 | 3 |
| OLED (1) | 2 | 2 | 3 |
| **Total /24** | 13 | **23** | 19 |

Constants: T_bored 3 min, T_tired 5 min, T_doze 2 min, T_fall_asleep 20 min (tuning values,
revisable after the prototypes without an ADR). Every gesture, voice or clap leaves Tired.

### D4 — REQ-F-11, F-12, F-13 deferred to a `social-group` PDR

Contagion and chain reaction are tier A, the rare gag is REQ-F-13 with its backlog item S12 in
tier C; all three get their own PDR (G2, G3, S12) so the cube-core firmware can start; they are drawn as deferred goals and marked so in the matrix.

### D5 — Structural simplifications from the review

MAC passed by value to the Identity rules (`localMac` dropped from `IRadio`); the presence mood
byte computed by the codec, not by the Sleep cycle; the ritual tick-driven (no loop in `lib/`);
`IAudio` speaks of a volume level, not a PWM duty; an immediate `touch-down` stimulus satisfies
REQ-NF-01 while classification completes; the sound classes (voice, clap, loud) are defined with
their thresholds and a call never annoys the cube.

## Consequences

- Specs updated: UC1 rewritten (invitation window, UC2 merged), UC6 becomes the ritual step,
  numbering unified on `specs/use-cases.md`; diagrams 01, 02a, 02b, 03, 05a, 05b, 02-presence,
  06 amended; traceability matrix corrected (F-11 no longer claimed by 05a).
- `cube-core` PDR passes with these edits; the next gate is the `social-group` PDR, then the CDR
  before ordering the series.
- The TTP223 power-on behaviour is added to the assembly end-of-line checks (invitation window
  must detect a touch on every cube).
