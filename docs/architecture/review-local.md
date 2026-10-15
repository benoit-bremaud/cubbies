# Local review — cubbies (branch diff)

- Date: 2026-10-15
- Commit: feat/f1-eyes (findings applied in the third commit of the branch)
- Scope: branch diff `main...HEAD` on `feat/f1-eyes` (firmware brick F1: ports `IDisplay` and
  `IClock`, `Prng`, `Mood`, `Framebuffer`, `EyesRenderer`, test doubles, native tests, golden
  images, ADR-0014, CLAUDE.md test rule, PROJECT_LOG)
- Checks run: design (architecture-guardian) · docs (doc-auditor) · tests (test-auditor) ·
  correctness and security (general review with `-Wall -Wextra` compile and PCG32 reference check)
- Verdict: fix Must Have first, then ship · Must 5 · Should 22 · Nice 15 — **all Must and
  Should applied before push**; 39 native tests green, both board envs build

## 🔴 Must Have

- `test_eyes.cpp` blink-gap test `[tests]` · Passed only because of the seed: the gap was
  measured start to start while the renderer schedules from the reopening, and the deadline
  rounds up to the next tick. · Applied: FS1 now says "eyes stay open 2–6 s between two blinks
  (measured from the reopening)"; the test runs 32 seeds with the open-interval semantics and a
  three-tick tolerance.
- `Golden.h` (then `Png.h`) update mode `[tests]` · Any value of `CUBBIES_UPDATE_GOLDEN` regenerated and passed
  silently. · Applied: value must be exactly `1`, refused when `CI` is set; a mismatch writes
  `<name>.actual.png` and the message names it; a missing file fails explicitly.
- `ADR-0014:30` `[docs]` · Option A total 31 instead of 30. · Applied.
- `EyesRenderer.cpp` blink comment `[docs]` · "half, closed, closed, half, ≈ 3 ticks" did not
  match the thresholds. · Applied: named constants `kBlinkHalfMs`, `kBlinkClosedMs`,
  `kBlinkTotalMs` on the tick, comment describes the real frames, a fourth golden `blink-4`.
- `IDisplay.h` `[docs]` · No buffer-lifetime contract, wrong "two test doubles" claim. ·
  Applied: single-buffer contract (present() finishes reading before returning), brightness
  levels explained.

## 🟡 Should Have

- `EyesRenderer.cpp` `[design]` · Magic blink numbers duplicated; `0x80000000u` idiom written
  twice; zero used as "never moved" sentinel; geometry hidden in the .cpp. · Applied: constants
  in the header, `isDue()` in `IClock.h`, `lidsSettled_` flag, geometry constants public.
- `EyesRenderer.cpp` `[correctness]` · `openEyes()` did not reschedule the blink: waking always
  blinked on top of the lid opening, and a deadline older than 24.8 days would stall. ·
  Applied, with a test ("no blink in the first two seconds after waking").
- `Framebuffer.cpp` `[correctness|security]` · `fillCircle` was O(r²) unbounded (a huge radius
  would trip the watchdog); `fillRect`/`fillRoundRect` had signed overflow on `x + w` for
  extreme inputs. · Applied: 64-bit span clipping, circle iterated over its on-screen bounding
  box only, test with `INT_MAX`/`INT_MIN` inputs.
- `Framebuffer.h`, `EyesRenderer.h` `[correctness|docs]` · 12 KB object by value undocumented
  as a stack hazard on the 8 KB loop task. · Applied: both headers say "static in main.cpp,
  never a local".
- `test_eyes.cpp` `[tests]` · Drift test did not prove a one-pixel shift; thresholds arbitrary;
  golden loop hid failures; no test for the `setLook` clamp, the no-blink-while-closed rule,
  the clock wrap, the clock-start independence. · Applied: shift test over the 4-step square
  open and closed, named `kClosedDivisor`, per-mood golden tests, four new rule tests.
- `test_framebuffer.cpp` `[tests]` · Tautological constant asserts, banded count. · Applied:
  removed, exact deterministic count.
- `test_prng.cpp` `[tests]` · Rejection loop never exercised. · Applied: large-bound test.
- `STB_VERSION`, `ADR-0014` `[docs]` · stb licence and versions. · Superseded on 2026-10-15: the
  vendored stb headers were removed after CodeQL raised 13 high alerts in them (raw RGB565
  references + an own PNG writer with known-answer tests; no third-party image code).
- Golden workflow `[docs]` · Documented only in CLAUDE.md. · Applied: `test/golden/README.md`,
  pointer in `lib/README.md`, script docstring; `sheet.png` and `*.actual.png` ignored by git.
- `EyesRenderer.h` `[design]` · `setIdleGlances` without a production reason. · Applied:
  documented as "something else owns the gaze" (reactions in F2); `frame()` removed.
- `EyesRenderer.cpp` `[docs]` · Happy-arc comment said open/3. · Applied (45 %).
- `IClock.h`, `Prng.h` `[docs]` · "MillisClock" without its brick; `next()` undocumented. ·
  Applied.

## 🔵 Nice to Have

- `static_assert(kEyeW > 1)` for the annoyed wedge division · Applied.
- `kGoldenDir` cwd-relative · Kept: `pio test` runs from `firmware/`, documented in the golden
  README; a build flag would add a second source of truth.
- `fillRoundRect` corners slightly fuller than a true circle (3 px cut at r = 6) · Kept:
  cosmetic and pinned by the goldens; changing it invalidates every image.
- Single-responsibility of `EyesRenderer` · Kept at this size; split trigger recorded: the
  third timed animation in F2 (blink and lid move already share the start+duration+phase
  lifecycle).
- `Lids` as a bag of flags, `kMoodCount` without consumer · Kept; revisit when F2 adds a third
  shape modifier or the mood engine consumes the count.

## ⚪ Disagree / Intentional

- `Framebuffer` by value inside the renderer `[design]` · The single-buffer design of the
  component diagram; the port receives a `const&`, nothing is copied; documented as static-only.
- `Prng` concrete class, no port `[design]` · ADR-0013 D1.
- Long simulated runs (120–180 s of virtual time) `[tests]` · Milliseconds of wall time on the
  fake clock; FIRST "fast" holds.
- Exact pixel comparison of the goldens `[tests]` · Deterministic integer rendering; no
  tolerance needed.

## Per-dimension summary

- Design: conforms to 03-component (renderer owns the buffer and the port, brightness through
  the renderer, Prng injected, no Arduino include); no over-engineering; split trigger noted.
- Docs: contracts stated on every port and class; ADR total fixed; golden workflow documented
  where contributors look.
- Tests: 34 native tests, one per rule of FS1 / REQ-F-01 / REQ-NF-08 plus goldens per mood;
  doubles only at the ports; no false-confidence test left.
- Security: no network or storage in this brick; primitives bounded against hostile inputs;
  stack hazard documented for the MCU.
- Correctness: PCG32 and `uniform()` verified against the reference; `-Wall -Wextra` clean;
  wrap arithmetic through `ageMs`/`isDue` only.
