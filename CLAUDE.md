# CLAUDE.md — cubbies

## Project overview

**Name:** cubbies.
**Type:** fleet of small cube desk companions on the Seeed XIAO ESP32-C3: colour OLED eyes
(SSD1331, 96×64), passive buzzer, capacitive touch, I2S microphone, I2C IMU and extras. Each cube
lives alone on a desk and gains group behaviours over ESP-NOW broadcast when other cubes are in
range. Gift for a team: one cube per colleague, one colour per cube, 3D-printed enclosure.
**Status:** conception. Decisions are ADRs under [docs/decisions/](docs/decisions/README.md);
the UML study lives under [docs/architecture/](docs/architecture/README.md); the roadmap and
prioritised backlog are in [docs/roadmap.md](docs/roadmap.md).
**Language:** English only in the repository (code, comments, commits, docs, issues, PRs).

Global rules (Conventional Commits, branch from `main`, pre-push review gate, private-first,
security defaults, UML-first, Q&A pattern) are inherited from `~/.claude/CLAUDE.md` and
`~/.agent-rules/common.md` — not duplicated here. Below are the rules that apply **only** to
this repository.

## Non-negotiables (project)

- **Design before code.** A behaviour is implemented only after it passes the PDR checklist of
  `docs/architecture/gates.md`: requirements cited (`specs/requirements.md`), textual use case,
  UML views under `docs/architecture/diagrams/<feature>/`, traceability matrix updated, owner
  validation. Hardware is frozen at the CDR before the series is ordered (ADR-0011).
- **Component choices are verified, not assumed.** Calculations, ngspice netlists and bench
  measurements live in `docs/hardware/` and `hardware/electronics/sim/` and are reviewed in PRs
  (`docs/architecture/verification-plan.md`). Eyes are verified by golden images in the native
  tests.
- **Validated decisions are not reopened silently.** Changing an ADR requires a superseding ADR
  in its own PR.
- **One firmware for every cube.** Identity (colour) comes from NVS, defaulting from the MAC
  address (ADR-0008). No per-cube build flags, ever.
- **The 11-GPIO budget is a hard limit** (ADR-0007). Any new sensor goes on the I2C bus or
  replaces an existing peripheral; the pin map lives in `firmware/include/pins.h`, one block per
  board env.
- **Behaviour logic is pure C++ and tested natively.** Moods, timers, the social protocol state
  machine and the identity rules live in `firmware/lib/` with no Arduino includes, and are
  tested with `pio test -e native`. Display, audio, radio and sensors are thin adapters in
  `firmware/src/`. Mock the boundaries, never the domain.
- **Time and randomness are injected.** No `millis()` or `random()` inside `lib/`; a clock is
  passed in and each consumer gets a lib-owned `Prng` seeded by `main.cpp` after the Wi-Fi
  driver is up (ADR-0013), so behaviours replay in tests.
- **Tests follow the conception (ADR-0014).** On `firmware/lib/` every table row of the
  validated diagrams becomes a failing native test before the code (TDD, Red → Green →
  Refactor). The eyes are verified by golden images under `firmware/test/golden/`
  (regenerate with `CUBBIES_UPDATE_GOLDEN=1 pio test -e native`, review the PNG sheet from
  `firmware/tools/golden_sheet.py` in the PR, then commit). Adapters in `firmware/src/` are
  validated by the board builds and on the bench, never by mocking Arduino.
- **The cube must never be loud.** Every sound goes through the single Sound gate in `lib/`,
  which applies the volume (a constant in V1, the ritual's stored step from V1.x, ADR-0013) and
  forwards a volume level to the one audio adapter, which alone converts it to a PWM duty. The cube is silent when it sleeps and, from
  S8 on, when the office is dark; clock-based quiet hours are out of V1 (no time source,
  ADR-0002, UML study 2026-10-14).
- **Radio frames are versioned and validated.** Every ESP-NOW frame carries a protocol version
  and is parsed by allowlist; unknown or malformed frames are dropped silently (security policy
  INPUT rules).
- **No secrets in the firmware.** Wi-Fi credentials for the future "mother cube" (V1.5) live in
  NVS, provisioned at runtime, never in source or build flags.

## Repository layout

```
docs/bom.md               bill of materials
docs/roadmap.md           versions and prioritised backlog
docs/decisions/           ADRs (Nygard format, ADR-NNNN-<slug>.md)
docs/architecture/        design dossier: README (arc42-lite), specs/ (requirements, use cases),
                          diagrams/ (UML), traceability matrix, verification plan, gates
docs/hardware/            electronics spec (signed GPIO contract), calculations, mechanical spec,
                          assembly guide, measurements
docs/research/            state of the art
firmware/platformio.ini   envs: xiao_c3 (series), c3_supermini (prototypes), native (tests)
firmware/include/         pin maps and shared headers
firmware/lib/             pure behaviour logic (no Arduino)
firmware/src/             adapters and main
firmware/test/            native unit tests (Unity): one folder per test program, shared doubles
                          in test/support, golden images in test/golden
hardware/electronics/     KiCad schematic (native) + PDF export, sim/ (ngspice netlists and plots)
hardware/case/            enclosure: FreeCAD Python script (source) + generated exports (CC BY-SA 4.0)
```

## Toolchain

- PlatformIO with the **pioarduino** platform pinned to a release URL in `platformio.ini`
  (Arduino core 3.3.x on ESP-IDF 5.5). Never float the platform or a library version.
- `pio test -e native` must be green before every push; `pio run` for both board envs must
  build. CI (`.github/workflows/ci.yml`) re-runs both on every pull request and push to `main`.
