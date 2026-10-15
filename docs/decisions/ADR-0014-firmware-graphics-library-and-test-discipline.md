# ADR-0014: Firmware — graphics library for the display adapter, test discipline, brick plan

**Status:** Accepted — 2026-10-14

## Context

The cube-core PDR passed (ADR-0013), so firmware can start. The eye renderer is our own code
drawing into a 96×64 RGB565 framebuffer behind the `IDisplay` port; the display adapter only
pushes that buffer and sets the brightness, so the graphics library choice is confined to one
file in `firmware/src/` and is reversible. Facts verified on 2026-10-14: LovyanGFX (FreeBSD
licence, commit the same day, registry 1.2.33) ships `Panel_SSD1331` and `Panel_ST7735` (the two
candidate screens of ADR-0006) and pushes over SPI with DMA on the ESP32 family; the Adafruit
SSD1331 library (BSD, last commit 2025-05) transfers without DMA on ESP32, blocking the CPU ≈ 16
ms per frame out of the 40 ms tick budget. The conception is written as tables (mood
transitions, sleep cycle, sound classes, frame layout, ritual sequences), each row a ready-made
test; the adapters contain no logic and cannot run on the host.

## Options and scoring (graphics library)

Criteria: both screens on core 3.x (3), speed and DMA overlap (3), simplicity (2),
maintenance (2), binary size (1).

| Criterion (weight) | A. LovyanGFX 1.2.33 pinned | B. Adafruit GFX + SSD1331 + BusIO | C. raw SPI driver of our own |
|---|---|---|---|
| Both screens, core 3.x (3) | 3 | 2 | 1 |
| Speed / DMA (3) | 3 | 1 | 2 |
| Simplicity (2) | 2 | 3 | 1 |
| Maintenance (2) | 3 | 2 | 1 |
| Binary size (1) | 2 | 3 | 3 |
| **Total /33** | **30** | 22 | 16 |

## Options and scoring (test discipline; TDD is opt-in per project)

Criteria: fit with a tabular, validated spec (3), fidelity of code to the conception (3),
cost (2), avoided false confidence (2).

| Criterion (weight) | A. TDD on `lib/` (tables → tests), golden images for the eyes, bench for adapters | B. Test-after everywhere | C. TDD everywhere |
|---|---|---|---|
| Fit (3) | 3 | 2 | 1 |
| Fidelity conception → code (3) | 3 | 1 | 3 |
| Cost (2) | 2 | 3 | 1 |
| False confidence avoided (2) | 3 | 2 | 1 |
| **Total /30** | **28** | 19 | 16 |

## Decision

- **LovyanGFX 1.2.33, pinned**, for the SSD1331 adapter (and the ST7735 comparison on the
  prototypes). Fallback: the Adafruit library, a change of that one adapter.
- **Test discipline**: on `firmware/lib/` every table row of the conception becomes a failing
  native test before the code that satisfies it (Red → Green → Refactor); the eyes are verified
  by **golden images** (characterisation tests: regenerated with `CUBBIES_UPDATE_GOLDEN=1`,
  reviewed as PNGs in the pull request, then committed); adapters in `firmware/src/` are
  validated by the board builds in CI and on the bench, never by mocking Arduino.
- **Brick plan**, one pull request each, in this order: F1 skeleton + eyes (ports `IDisplay`,
  `IClock`, `Prng`, `Framebuffer`, eyes renderer, fakes, golden images); F2 behaviours (gesture
  and sound classifiers, stimulus router, mood engine, sleep cycle, Sound gate); F3 identity
  and boot (identity rules, invitation window, tick-driven ritual); F4 social (frame codec,
  neighbour table, greeting policy, loopback radio, fuzz); F5 adapters and `main.cpp` (both
  board envs build; bench when the prototypes arrive). Ports and fakes are created by the
  brick that first uses them (YAGNI).
- Randomness: PCG32 (Apache-2.0 minimal implementation) as decided in ADR-0013.
- Golden-image tooling: `stb_image.h` / `stb_image_write.h` vendored under `test/support/`
  (dual licence MIT / public domain, upstream commit and versions recorded in `STB_VERSION`).

## Consequences

- `platformio.ini` gains `lib_deps = lovyan03/LovyanGFX@1.2.33` on the board envs in F5 only.
- A pull request on `lib/` is reviewed for "every table row has its test"; a golden image
  changes only in a PR whose description shows the before/after sheet
  (`firmware/tools/golden_sheet.py`).
- The project `CLAUDE.md` records the discipline as a non-negotiable.
