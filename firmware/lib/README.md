# firmware/lib

Pure behaviour logic, one PlatformIO library per domain, **no Arduino includes**: moods, sleep,
identity rules, the social protocol state machine, frame encoding. Time is injected (a clock
port) and randomness is a lib-owned `Prng` seeded by `main.cpp` (ADR-0013); nothing here reads
`millis()` or `random()`, so every
behaviour replays identically in the native tests under `../test/`.

Adapters (display, audio, touch, radio, sensors) live in `../src/` and are the only code allowed
to include Arduino or ESP-IDF headers.

Layout of `cubbies_core/`: `ports/` (interfaces owned by lib), `core/` (Prng, Mood),
`eyes/` (Framebuffer, EyesRenderer); `behaviour/`, `identity/` and `social/` arrive with
bricks F2–F4 (ADR-0014). Include paths are relative to `cubbies_core/`, e.g.
`#include "eyes/EyesRenderer.h"`.

Golden images of the eyes: see `../test/golden/README.md`.
