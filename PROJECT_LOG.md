# Project Log — cubbies

Reverse-chronological operational logbook (most recent first). It complements `git log` with the
human context: what was done, why, and by which PR.

---

## 2026-10-14

### Brick F1 — firmware skeleton and eyes (branch `feat/f1-eyes`, ADR-0014)

- **Decisions** (ADR-0014, scored options): `display adapter: LovyanGFX 1.2.33` — SSD1331 and
  ST7735 in one package, DMA keeps the 40 ms tick; Adafruit rejected (no DMA on ESP32), raw
  SPI rejected (driver to maintain). `tests: TDD on lib/, golden images for the eyes, bench
  for adapters` — the conception is tabular, each row is a test; test-after rejected (tests
  would describe the code, not the table).
- First code of the project, TDD in two commits (red tests, then green): ports `IDisplay` and
  `IClock`, `Prng` (PCG32, reference value checked), `Framebuffer` 96×64 RGB565 with clipped
  primitives, `EyesRenderer` (two rounded eyes in the identity colour, five mood shapes,
  blink every 2–6 s aligned on the 40 ms tick, idle glances, one-pixel drift per minute in every
  state, closed eyes for sleep). 34 native tests green, both board envs build.
- Golden images (10 PNG) generated and reviewed on the contact sheet; vendored stb headers
  (MIT / public domain) for PNG read/write in tests.
- Pre-push `review-local` gate (design, docs, tests, correctness): 5 Must Have and 22 Should
  Have applied before push, notably the blink-gap test that only passed thanks to its seed
  (FS1 reworded: 2–6 s of open eyes measured from the reopening), the golden update mode that
  passed silently, `openEyes()` rescheduling the blink, framebuffer primitives bounded against
  extreme inputs, the 12 KB-on-the-stack hazard documented for `main.cpp`.
- Owner validated the five mood frames and the closed eyes as golden references (2026-10-15)
  and chose to complete the face vocabulary in a dedicated brick **F1b** before F2 (reactions
  and identity frames with a shared `Timeline`; ADR-0014 amended), after scored options.

### cube-core PDR passed (branch `docs/pdr-cube-core`, ADR-0013)

- Four-pass review of the eight diagrams against the requirements: five blocking
  contradictions (use-case numbering, ritual entry, touch latency vs double-tap window, sleep
  timings vs FS5, three tier-A requirements neither modelled nor deferred) and 14 Should Haves.
- **Decisions** (ADR-0013, scored options and two deeper analyses):
  - `randomness: lib-owned PCG32` — seeded from `esp_random()` after Wi-Fi init; the `IRng`
    port was a single-implementation port whose fake re-implemented a PRNG.
  - `ritual entry: post-boot invitation window` — the TTP223 calibrates during its first 0.5 s,
    so "hold while plugging in" may never be detected; rejected as unreliable.
  - `volume: second step of the adoption ritual (V1.x)` — discoverable, no collision with the
    awake gestures; triple tap rejected (lengthens tap classification), mother cube rejected
    (takes the setting away from the colleague).
  - `sleep timings: doze 10 min, sleep 30 min` — matches FS5 and distinguishes a coffee break
    from a meeting; any stimulus leaves Tired.
  - `REQ-F-11/12/13: deferred` — their own `social-group` PDR so cube-core firmware can start.
- Diagrams 01, 02a, 02b, 03, 05a, 05b, presence and data-flow amended (MAC by value, mood byte in
  the codec, tick-driven ritual, entry press consumed, touch-down stimulus, sound classes, tick
  budget, mic privacy note, greeting policy owned by the presence sequence); specs UC1/UC3/UC4/UC6
  rewritten, UC2 retired, matrix corrected, CLAUDE.md rules aligned. Pre-push gate: design and
  docs reviews, 6 Must Haves (ADR totals, entry press, codec edge, stale wording) applied.
  Next: firmware T1/T2/T8 can start.

### Architecture README restructured as arc42-lite — **PR #7 merged** (`83a4ae2`)

- Post-merge pass after PRs #1–#6: every 2026-10-14 entry now carries its PR number and merge
  SHA; stale "next" sentences in merged entries updated in place.
- Dependency bumps are not logged individually (PR #2, `0e2d7c1`, Dependabot `actions/cache`
  5.1.0 → 6.1.0, is the first).
- `docs/architecture/README.md` is now the arc42-lite map decided in ADR-0011 (goals,
  constraints, C4 context, strategy, building blocks and ports, crosscutting concepts, quality,
  risks, glossary) with the UML index kept at the end. Relative links checked, Mermaid rendered.

### Design dossier scaffolded — **PR #4 merged** (`8b86ae8`)

- Owner decisions after scored options (ADR-0011): dossier = arc42-lite overview + functional
  requirements (NF X50-151 style) + Cockburn textual use cases + UML + traceability matrix +
  hardware specs with a signed GPIO contract + OSHWA assembly guide + PDR/CDR gates;
  verification = calculations and targeted ngspice simulations committed with plots, native
  tests, golden-image tests of the eyes through a native display adapter, bench measurements;
  Wokwi for exploration only (no SSD1331, no I2S mic in its catalogue).
- Enclosure tooling (ADR-0010): FreeCAD 1.1 driven by a versioned Python script, generated
  `.FCStd`/STEP/STL as artefacts, `freecad-mcp` for inspection; `.FCStd` is a binary zip, so the
  script is the reviewable source. KiCad follows the V-SEA method (KiUtils + `kicad-cli` ERC,
  no MCP; provisional, no ADR yet).
- Written: `specs/requirements.md` (9 service functions, 10 constraints, 15 functional and 11
  non-functional requirements), `specs/use-cases.md` (11 UCs + 3 traced system behaviours), `verification-plan.md`,
  `gates.md`, `traceability-matrix.md`, `docs/hardware/{electronics-spec, calculations,
  mechanical-spec, assembly}.md`, `docs/research/state-of-the-art.md`; roadmap gains S12 (rare
  gag, owner request, cartoon level, rate-limited) and T8–T10; CLAUDE.md non-negotiables updated.
- The first UML study (eight diagrams, separate session, branch `docs/uml-study`) is kept as a
  first draft and re-validated against the requirements at the `cube-core` PDR. The arc42
  restructure of `docs/architecture/README.md` followed in its own PR (entry above).

### First UML study drafted — **PR #5 merged** (`a027977`)

- Eight Mermaid diagrams under `docs/architecture/diagrams/`: `cube-core` (use case by domain,
  component lib-vs-adapters with eight ports, mood engine state, sleep/wake state), `identity`
  (boot resolves identity, adoption ritual entered by the held-at-boot gesture),
  `social-presence` (presence sequence with greeting policy, frame data-flow with the 8-byte v1
  PRESENCE layout).
- Every block rendered with `mmdc` without a parse error. Three critical review passes
  (architecture guardian) before the `review-local` gate. First pass: ten Must items (MAC in the
  payload in one diagram and not the other, quiet hours with no time source, audio policy and
  gesture timers in adapters against the lib-purity rule, ports not matching the sequence calls,
  a system event modelled as a use case, shake needing the tier-B IMU). Second pass: four Must
  (wake-up melody owned by three diagrams, a Sleep/Mood circular dependency, a Router → Identity
  edge the ritual never uses, a storage edge with nothing stored) and a greeting policy that
  could skip a newcomer. All fixed. Third pass: no Must left, two stale use-case numbers from the renumbering and
  the "greet while awake" wording fixed. `review-local` gate (design, docs, security,
  correctness): four Must (receive-path rate budget and length check in the callback, spoofing
  risk stated honestly, one stale reference), all applied; report in
  `docs/architecture/review-local.md`.
- Design choices submitted as scored options (Q&A, two rounds with deeper analysis) and
  **validated by the owner**: quiet hours out of V1 (no time source; silence comes from sleep,
  then from the light sensor in S8), discrete moods, wake restarts in Curious, greeting as a
  local rate-limited stimulus (GREET frame re-evaluated with G3), palette index on the air with
  a version bump on palette change (RGB565 revisited in V1.5), ritual entered only by the
  held-at-boot gesture, derived MAC colour not persisted, constant default volume until a UC6
  gesture is chosen, unauthenticated ESP-NOW broadcast accepted as residual risk. Consequences
  applied: `CLAUDE.md` "never loud" rule reworded, roadmap S4/S5/S8 reworded.
- The accepted ESP-NOW residual risk and the V2 fleet-key hook are recorded in ADR-0012 (amends
  ADR-0005, PR #3). Double tap cycling backwards in the ritual stays open until the palette size.
- Next: backlog items S1–S7 and G1 become issues with these diagrams as
  their contract.

### Repository public, private-repository guards removed — **PR #6 merged** (`96d781f`)

- The repository went public after PR #1 (security CI baseline) merged.
- Removed the `if: ${{ !github.event.repository.private }}` guards from `codeql.yml`,
  `dependency-review.yml` and `scorecard.yml`; the three workflows now run for real (CodeQL on
  `actions` and `c-cpp`, dependency review on PRs, Scorecard on `main` after merge).

### ADR-0012: unauthenticated ESP-NOW broadcast recorded as an accepted residual risk (amends ADR-0005) — **PR #3 merged** (`c8649ae`)

- The social-presence UML study (branch `docs/uml-study`, data-flow and sequence diagrams) found
  that ESP-NOW broadcast cannot be encrypted, that the source MAC is forgeable, and that V1 has
  no path to provision a key. An attacker in radio range can flood the 16-slot neighbour table,
  impersonate a cube's presence and mood byte, or trigger one greeting per cooldown.
- Four options scored (accept bounded risk / fleet key in NVS at adoption / per-peer unicast
  keys / BLE instead). Decision: accept in V1, bounded by eviction of absent entries only,
  greeting cooldown plus a global budget, allowlist and rate-bounded receive path, RAM-only and
  eyes-and-beep-only effects. ADR-0005's transport choice stands; its status line now reads
  "Amended by ADR-0012".
- V2 hook recorded: fleet key in NVS, never in source. V1.5 relayed OTA must ship a signed image,
  not a hash alone.

### Security CI baseline — **PR #1 merged** (`9b54ecf`)

- Owner decision: the repository goes public (free Copilot review and analysis tools) only
  after the public release checklist passes; the missing item was the CI secret-detection job.
- Added the `security-ci-baseline` set, SHA-pinned, same set as the owner's other repositories:
  Gitleaks (full history, weekly), CodeQL (`actions` + `c-cpp` traced on the native host
  build), dependency review, OSSF Scorecard, Dependabot for GitHub Actions; plus the CI workflow
  (native tests, both board builds) and `.gitleaks.toml`.
- CodeQL, dependency review and Scorecard are guarded by `if: !repository.private` (code
  scanning and dependency review need a public repository or Advanced Security, and Scorecard's
  official template has no `contents: read` for a private checkout); the guards go away in the
  public-release PR. Until then only CI and Gitleaks are meaningful checks.
- Local gitleaks over history and working tree: no leaks. Pre-push `review-local` gate run:
  no Must Have; the Should Haves (private-repo guards, `cancel-in-progress` limited to pull
  requests, comment accuracy, CLAUDE.md mention of CI) applied before push.

### Project kick-off: scoped Q&A session, decisions recorded, repository scaffolded

- Owner's intent: a fleet of small cube desk companions (OLED eyes, mic, buzzer) that interact
  with each other, offered to office colleagues on an "adoption day", one colour per cube,
  3D-printed enclosures. Prototypes first (2–3 cubes), then more than 10.
- Desk research on the existing landscape (Dasai Mochi clones on ESP32-C3, RoboEyes / OliEyes
  eye libraries, ESP-NOW vs BLE for peer discovery, PlatformIO vs pioarduino core status).
  Finding: solo desk companions are well covered; the multi-cube social layer is the
  differentiator.
- Nine structuring decisions taken through scored Q&A and recorded as ADR-0001..0009:
  concept (solo companion + proximity social layer), power roadmap (USB-C → LiPo → Qi), board
  (XIAO ESP32-C3), stack (PlatformIO + pioarduino, Arduino core 3.3.x), radio (ESP-NOW
  broadcast), display (SSD1331 OLED, ST7735 LCD compared on prototypes), sensor tiers under an
  11-GPIO budget, identity (MAC default + adoption ritual in NVS), licensing (MIT code,
  CC BY-SA 4.0 enclosure).
- Repository scaffolded: README, BOM, roadmap with prioritised backlog, ADRs, architecture
  conventions, PlatformIO skeleton with two envs (XIAO C3, C3 Super Mini) and a native test.
  Private GitHub repository `benoit-bremaud/cubbies` created and `main` pushed.
- Next: UML study (use cases, mood state machine, ESP-NOW presence sequence, component view)
  by PR, before any firmware; order prototype parts (BOM).
