# ADR-0004: Stack — PlatformIO with the pioarduino platform, Arduino core 3.3.x

**Status:** Accepted — 2026-10-14

## Context

Verified on 2026-10-14: the official PlatformIO `espressif32` platform (v7.1.3) still ships
Arduino core 2.0.17 on ESP-IDF 4.4.7, frozen since 2024. Espressif's active core is 3.3.12 on
ESP-IDF 5.5 (4.0 alpha in progress), with the new I2S driver (used by the microphone), current
NimBLE and ESP-NOW v2. The community platform **pioarduino** (release 55.03.312-1, 2026-09-22)
provides core 3.3.12 inside the PlatformIO workflow; its maintainer team is small. Arduino CLI
with GitHub Actions uses the official core but pins libraries manually, lists transitive
dependencies by hand and has no native host tests. Pure ESP-IDF would force rewriting the whole
display ecosystem. The colour eyes will be a custom procedural renderer (RoboEyes and OliEyes are
1-bit monochrome), drawn into a 12 KB framebuffer over SPI; the graphics library (Adafruit GFX +
SSD1331, or LovyanGFX) is chosen by a benchmark on the prototypes.

## Options and scoring

Criteria: KISS (3), current core (3), native tests + CI (2), pinned dependencies (2),
sustainability (2).

| Criterion (weight) | A. Official PlatformIO (2.0.17) | B. PlatformIO + pioarduino (3.3.x) | C. Arduino CLI + Actions | D. ESP-IDF + Arduino component |
|---|---|---|---|---|
| KISS (3) | 3 | 3 | 2 | 1 |
| Current core (3) | 1 | 3 | 3 | 3 |
| Native tests + CI (2) | 3 | 3 | 1 | 2 |
| Pinned deps (2) | 3 | 3 | 2 | 3 |
| Sustainability (2) | 2 | 2 | 3 | 3 |
| **Total /36** | 28 | **34** | 27 | 28 |

## Decision

**PlatformIO with the pioarduino platform pinned to a release archive URL**, Arduino framework,
core 3.3.x. Behaviour logic is pure C++ under `firmware/lib/` and tested on the host with
`platform = native` and Unity; adapters live in `firmware/src/`. CI builds both board envs and
runs the native tests.

## Consequences

- The platform URL and every `lib_deps` entry are pinned; the risk of the fork stalling is
  contained because a pinned build keeps compiling identically.
- Falling back to the official platform would mean rewriting the I2S layer (legacy driver); this
  is accepted and documented rather than hedged with abstractions.
- The graphics library choice is a prototype-phase decision, recorded as a follow-up ADR.
