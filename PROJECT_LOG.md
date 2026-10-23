# Project Log — cubbies

Reverse-chronological operational logbook (most recent first). It complements `git log` with the
human context: what was done, why, and by which PR.

---

## 2026-10-23

### G5 collective sleep (issue #59) — **PR pending**

- G5 picked by MCDA (465/500 vs G6 jealousy 415, voice study 375, S9 295).
- PDR `social-group` (G5 part) validated by the owner: REQ-F-20, behaviour B4 (system-initiated,
  outcome of UC5 + UC8), `diagrams/social-group/` 05d yawn rows and 02d sequence, traceability.
  Scored options (effect 30, owner safety 25, simplicity 25, robustness 20): D1 only a Bored cube
  catches the yawn 440/500; D2 it enters Tired and yawns, then the usual sleep cycle 470/500.
  No new frame: PRESENCE mood byte 3 (Tired/dozing) or 5 (asleep) = drowsy; T_contagion 6 s reused.
- TDD (red commit first): `MoodTally::drowsy`, `MoodEngine::contagion()` yawn hold evaluated
  before G2. No `main.cpp` change (already wired by G2).
- review-local: Must 1 doc (fixed) · Should 3 (fixed) · Nice 6 (2 fixed). Gate: 305 native tests,
  both boards, benches PASS, ERC 0, ruff, gitleaks clean; SonarQube red only for #53.

### G4 synchronised choir (issue #57) — **PR #58** (31b40d0)

- Project resumed after the 2026-10-22 pause; G4 picked by MCDA (410/500 vs voice study 390, S8 330).
- PDR `social-group` (G4 part) validated by the owner: UC10 with extensions,
  `diagrams/social-group/` 05c choir rows and 02c sequence, CHOIR type 0x03 in social-presence/06,
  traceability, FS8 drift row in the verification plan. Scored options (effect 30, robustness 25,
  simplicity 25, fit 20): D1 stroke with a present neighbour 430/500; D2 one-shot start anchored
  on the receive time + lead rest in the Sound gate 475/500; D3 voice = MAC rank mod 3 445/500;
  dossier 445/500. kLeadMs 500, T_choir 60 s, three voices at rank 3 (raised from 2 after Codex P2: a Grumpy must not block the choir), never relayed.
- TDD (red commit first): `encodeChoir`/`decodeChoir`, `Choir`, `SocialProtocol::startChoir()`,
  `SoundGate::play(id, restMs)` and three placeholder voices; `main.cpp` wiring.
- review-local: Must 0 · Should 19 (fixed) · Nice 8 (4 fixed, 4 documented). Fixed bug: a CHOIR
  frame received before the last start and drained a tick later passed the 60 s cooldown.
- Gate: 292 native tests, both board builds, benches PASS, gitleaks clean, new lib code 100 %
  lines. SonarQube red on main outside the diff (#53), owner waiver. FS8 drift bench pending.

## 2026-10-22

### G3 chain reaction (issue #55) — **PR #56** (6a2807b)

- PDR `social-group` (G3 part) validated by the owner: UC9 with extensions,
  `diagrams/social-group/` 05b chain rows and 02b sequence, CHAIN type in social-presence/06,
  traceability. Scored options (effect 30, robustness 25, simplicity 25, fit 20): D1 double tap
  with a present neighbour 425/500; D2 MAC-rank delay 300 ms × (1 + rank) 430/500; D3 8-byte
  type 0x02 frame, (palette, 8-bit id) key, hop ≤ 3, protocol v1 kept 445/500; D4 `ChainReaction`
  owned by the Social protocol 420/500. T_trigger 10 s, T_cooldown 5 s, T_seen 30 s (8 keys).
- TDD (red commit first): `encodeChain`/`decodeChain`, `ChainReaction`, `SocialProtocol::startChain()`,
  `routeChain()`; `main.cpp` reads the MAC before building the Social protocol.
- review-local: Must 0 · Should 13 (fixed) · Nice 9 (documented). Codex P1 (first hop 600 ms when
  the origin has the lower MAC) fixed: the frame sender is not ranked; P2 declined (row 12 caps
  received events, the owner trigger has its own 10 s cooldown).
- Gate: 262 native tests, both board builds, benches PASS, gitleaks clean, new lib code 100 %
  lines. SonarQube red on main outside the diff (#53), as for G2.

### G2 contagious mood (issue #52) — **PR #54**

- PDR `social-group` (G2 part) validated by the owner: UC8 with extensions,
  `diagrams/social-group/` (05a contagion rows, 02 sequence), traceability. Scored options
  (effect 30, stability 25, simplicity 25, robustness 20): D1 neighbours-only plurality, ties
  Happy > Annoyed > Curious > Bored, 60 s refractory 430/500 (365 self-voting, 275 Prng voter);
  D2 every mood but Tired 450/500; D3 mood-engine extension fed a `MoodTally` value 450/500.
  T_contagion 6 s, T_refractory 60 s.
- TDD (red commit first): `MoodTally` + `pulledMood()`, `MoodEngine::contagion()` and
  `touchDown()`, `SocialProtocol::moodTally()`; one line per Awake tick in `main.cpp`. No new
  radio field; forged frames capped at one mood change per minute (ADR-0012).
- review-local: Must 0 · Should 9 (fixed) · Nice 7 (4 fixed). Codex P2 (a touch-down did not
  break the calm before contagion) fixed in the PR.
- Gate: 238 native tests, both board builds, benches PASS, gitleaks clean, CI green, new lib
  code 100 % lines. SonarQube red on main outside the diff (S3776 in `cubbies_case.py` from #46,
  no Python coverage.xml): owner waiver, tracked in #53.
- Tooling: PlatformIO needs `REQUESTS_CA_BUNDLE=/etc/ssl/certs/ca-certificates.crt` behind the
  TLS-inspecting endpoint agent.

## 2026-10-19

### Print pack for the Grasse fablab (issue #45) — **PR #46 merged** (`e7effe1`)

- `cubbies_case.py` cuts a tolerance coupon from the real shells (left hook and its pocket, the
  back-wall frame around the cover rebate, printed with a real cover) and checks it (check 18:
  whole hook, whole pocket, full rebate). It also writes `hardware/case/export/print/`: parts
  already turned and resting on the bed, plus `coupon-plate.3mf` and `cubbies-plate.3mf`.
- `hardware/case/PRINT.md`: one-page fablab sheet (files, PLA / 0.2 mm / 4 perimeters / no
  supports, coupon first, what to check, results to `docs/hardware/assembly.md`).
- Owner: fablab printer and slicer unknown, so the 3MF plates are geometry only and open in any
  slicer; a slicer project is saved once the slicer is known. Front shell prints front face down
  (mechanical spec), not up as the brief said.
- Codex raised 1 valid point (the coupon check could pass without the pocket), fixed in the PR.
- Gate: 220 native tests, both board builds, electronics benches (`hardware/electronics/sim/run.sh`,
  all PASS), ruff and gitleaks clean on the diff, CI green. Not run: SonarQube.
- Open: print-and-fit at the fablab; pre-existing ruff findings unchanged.

### FreeCAD enclosure script with the module bay (issue #42) — **PR #43 merged** (`d94528e`)

- `hardware/case/cubbies_case.py` (ADR-0010, CC BY-SA 4.0) generates the front shell, the rear
  shell and the ADR-0016 bay cover headless (`freecadcmd`), then STL/STEP/FCStd under
  `hardware/case/export/`. It runs 17 dimension checks first and exports nothing if one fails
  (envelope, walls, openings, bay, J1 parameters, hook clearance, barb direction, engagement
  and release, hook strain against PLA, cover push-in stop, part collisions); mutation-tested.
- Owner MCDA: 40 mm cube with the bay above the XIAO (460/500, 46 mm shell 340); FreeCAD 1.1.1
  AppImage behind a `~/.local/bin/freecadcmd` wrapper (440/500, snap 275): no sudo, pinned
  version, headless for agents.
- review-local: Must 4 (shell barbs reversed, cover hooks never engaged and no push-in stop, a
  vent slot through the corner, cover hook strain ~18 %) · Should 9 · Nice 4, all fixed. Codex
  (enabled on the repository) then raised 3 valid points, fixed in the PR: 45° releasable barb
  back faces (UC12 cover removal), front shell printed face down, DS1 held by 3M foam tape.
- Gate: 220 native tests, both board builds, benches, ERC 0, gitleaks clean, SonarQube PASSED
  (new coverage 93.9 %, `cubbies_case.py` 92 % under FreeCAD's Python), docstrings 91 %, CI green.
- Open: print-and-fit at the mechanical CDR (OLED module and XIAO sizes are estimates); print
  pack for the Grasse fablab proposed as a follow-up; pre-existing ruff findings in
  `firmware/tools/golden_sheet.py` and `hardware/electronics/sim/calc.py`.

## 2026-10-16

### CDR paper items (issues #30, #31, #32) — C-0013, C-0014, C-0015 — **PR #38 merged** (`1580cd5`)

- Source reading: the XIAO ESP32-C3 LDO is a **Torex XC6210B332MR** behind a 500 mA PTC and a
  Schottky (Seeed schematic), not a TLV75733. C-0013 supersedes C-0002: θJA derived 132–400 °C/W
  (167 nominal); rail window 3.234–3.366 V carried to every bench (buzzer peak 37.4 mA).
- Owner MCDA: J1 budget 50/75 mA kept provisional, bench LDO temperature at 50 mA on J1 is a CDR
  exit criterion (415/500); 10 kΩ pull-up R6 on GPIO8 (no pull-up on the XIAO, 435/500);
  closed-form second method `crosscheck.py` instead of a second SPICE engine (410/500).
- review-local: Must 5 · Should 9 · Nice 4, all Must/Should fixed; SonarQube PASSED after 1 issue
  and 1 regex hotspot fixed; Python coverage 96 %; gitleaks clean; ERC 0; benches PASS; CI green.
- Still open for the bench (#33): LDO temperature, Schottky Vf from its datasheet.
- Follow-up **PR #40 merged** (`0e68452`): root README (status, XC6210, J1, layout) and
  architecture risks (R6 on GPIO8, provisional J1 budget). Its full local gate ran after the
  merge (all green); owner rule since then: the full local suite runs before every push, docs
  included.

### Expansion port design (issue #18) — ADR-0016, C-0012 — **PR #36 merged** (`fc3b183`)

- REQ-F-20, REQ-NF-12, UC12 "Add a capability by plugging a module", views under
  `diagrams/expansion-port/` (boot probe sequence, component), traceability and verification
  rows.
- C-0012: a J1 module may draw ≤ 50 mA continuous, ≤ 75 mA peak (owner validated 50/80, confirmed 75 before the PR; the
  calculation trims the peak because C-0001's peak is 420.3 mA). `calc.py` asserts both.
- Owner MCDA: internal module bay behind a clipped rear cover (415/500 vs outside slot 365,
  fixed internal 335); bay 22 × 22 × 6 mm in the mechanical spec (FreeCAD script not written yet).
- Plug-and-play principle (ADR-0016 D3): allowlisted I2C probe at boot with identity check,
  timeout, reserved sensor addresses, one firmware; code at the first module's PDR.
- review-local: Must 2 · Should 6 · Nice 4, all fixed; SonarQube PASSED (coverage 94 %), gitleaks
  clean, 220/220 native, benches PASS.
- Owner PDR validation recorded in the PR. No Copilot/Codex review (owner: local gates only for now); CI green.

### KiCad schematic `cubbies-v1.kicad_sch` — benches fed from the schematic — **PR #34 merged** (`afb4ede`)

- Native KiCad 9 schematic + PDF from the GPIO contract (ADR-0015), BOM and assembly guide:
  XIAO (U1, 2×7), SSD1331, INMP441, TTP223, GY-521, GY-302, buzzer stage, RES# RC, 47 kΩ SDA
  pull-up. Every symbol has a Justification field citing C-NNNN / ADR. ERC: 0 violations.
- I2C expansion port J1 (issue #18): JST-SH 4-pin Qwiic/STEMMA QT, owner MCDA 450/500 (2.54 mm
  header 410, 5-pin with interrupt 310).
- `sim/schematic_values.py` exports the SPICE netlist with kicad-cli and writes `schematic.lib`
  (`.param` + `.csparam` per R/C); the four benches read R1–R5 and C1 from it, `run.sh` refreshes
  it (committed copy used, with a message, when kicad-cli is absent). Tolerances, corners and
  module pull-ups stay in the benches; `i2c-bus.txt` changed only because the networks are now
  computed exactly. Calculation log inputs unchanged. Sensitivity checked: R1 set to 68 Ω on the
  sheet makes `run.sh` fail on C-0003.
- review-local: Must 1 · Should 9 · Nice 7, all Must/Should fixed; SonarQube: 1 issue fixed, new coverage 93.4 %, regex hotspot reviewed Safe, quality gate PASSED.
- No external review: Codex hit its usage limit. CI green (native tests, both board builds,
  CodeQL, secret scan, dependency review).

### Hardware validation — calculation log and ngspice benches — **PR #28 merged** (`4b72e59`, ADR-0015)

- V-SEA method applied: append-only `docs/hardware/calculations/log.md` (C-0001..C-0011, tiered),
  ngspice 42 benches with worst-case corners under `hardware/electronics/sim/` (`sh run.sh`),
  tier-3 lines in `calc.py`.
- Three drafted choices failed and were fixed (owner, scored options, ADR-0015): buzzer and SDA
  swapped so the strapping pin GPIO2 reads 1 at reset (430/500); display RES# RC 10 kΩ / 1 µF +
  1N4148 (470/500); buzzer series resistor 82 Ω (440/500); after the pre-push review, a 47 kΩ
  board pull-up so GPIO2 boots high with no I2C module fitted (485/500).
- Benches are self-validating (`sh run.sh` fails on any `FAIL C-NNNN`, negative controls print
  `EXPECTED-FAIL`); review-local: Must 3 · Should 8 · Nice 4, all Must/Should fixed.
- No external review: Codex hit its usage limit, Copilot cannot be requested (not a collaborator).
  CI green (native tests, both board builds, CodeQL, secret scan, dependency review).
- Open before the CDR: LTspice second engine on Tier 1 entries, LDO θJA/dropout from the
  datasheet, bench boot/download check, I2C rise-time measurement.

## 2026-10-15

### Brick F5 — adapters and `main.cpp` — **PR #26 merged** (`ff7b509`, ADR-0014)

- Seven thin adapters in `firmware/src/`: `MillisClock`, `Ttp223Touch`, `NvsStorage`
  (Preferences, typed), `BuzzerAudio` (LEDC, duty table 2/8/20/50 %), `I2sMic` (rms + peak,
  samples never leave), `Ssd1331Display` (LovyanGFX 1.2.33 pinned), `EspNowRadio` (channel 1,
  8-slot queue, length drop in the callback). `main.cpp` wires the boot (Wi-Fi, seeded Prngs,
  identity, invitation window, ritual) and the 40 ms tick in the 03-component order.
- Owner decisions (scored options): note stepping by `esp_timer` (470/500), FreeRTOS queue for
  RX (480/500), explicit channel constant 1 (380/500, after a deeper analysis). The volume table
  came from calculations.md (no new decision). Decisions recorded in 03-component.
- Local review: Must 3 · Should 4 applied (I2S partial reads, init-failure guards, STA power
  save off, stale boot frames purged); Nice grouped in issue #25.
- Codex review (3 rounds), every thread answered inline: 4 P2 fixed in `main.cpp` (gate ticked
  before any sound-producing output, wake-up melody at normal start per 02a, played after the
  adoption sound ends, MAC zeroed on a read failure). Third round clean.
- Both board envs build with no warning, 220 native tests green; bench checklist in
  `docs/hardware/bench-f5.md` (issue #16 checks included).

### Brick F4 — social presence — **PR #22 merged** (`63b0103`, ADR-0014)

- PDR status: REQ-F-10 was already covered by the cube-core PDR (ADR-0013); the `social-group`
  PDR (G2/G3) stays deferred. New state diagram social-presence/05a (neighbour entry and
  greeting) with its transition table; 03-component note and traceability matrix amended.
- Owner decisions (scored options): `IRadio` pulled by the lib, which drains 4 frames per tick
  (475/500); greeting budget as a sliding window over the last 6 greetings (470/500); the cube's
  own state passed per tick as `SelfState`, mood byte computed by the codec (445/500); F4
  review-local report overwrites F3's (475/500).
- Design gap found while writing the tests: a newcomer that leaves before its greeting is still
  owed one (`greeted` bit kept while Absent, 05a amended).
- TDD: `PresenceCodec` (allowlist, drop reasons, 05b mood byte, 200 000-input fuzz) and
  `SocialProtocol` (16-slot table, absent eviction, cooldown, re-greet hysteresis, strict sliding
  budget, jittered broadcast, wrap), `LoopbackRadio` fake, two cubes greeting each other once.
- Local review: Must 1 · Should 11 applied (Waiting refresh row, exact boundaries, flood
  assertions, `ports/Mac.h` so the port no longer includes core, `findSlot()`, 2^32 aliasing of
  absent entries and of the greeting ring); Nice 6 grouped in issue #23.
- Codex review (2 rounds, no Copilot), every thread answered inline: one P2 fixed in TDD, an
  Absent entry older than 2^32 ms could alias as recent and mislead the eviction order, so the
  tick now frees Absent entries past T_regreet (05a row added). Second round clean.
- 220 native tests green, both board envs build, CI green. `EspNowRadio` and the `main.cpp`
  wiring are F5.

### Brick F3 — identity and boot — **PR #20 merged** (`93c9019`, ADR-0014)

- Design first: new state diagrams identity/05a (invitation window) and 05b (adoption ritual),
  amendments to 02a, 02b, 03, 05c (`holdMs()`, `consumeCurrentPress()`, `Consumed` state) and
  05e (ritual sounds). PDR validated by the owner.
- Owner decisions (scored options): 8-colour palette, forward only (445/500); one fixed ritual
  tick, pitch per index deferred to the sound brick (425/500); `InvitationWindow` as its own lib
  class (435/500); double tap = +2 (470/500); a press already down at the end of the settle is
  consumed (470/500); window and ritual return an `IdentityOutput` that `main.cpp` applies
  (445/500); any gesture restarts the 60 s timeout (470/500). Adoption sound rank 0.
- Five TDD steps, each red then green: `resolveIdentity` (FNV-1a MAC hash mod 8, typed
  `IStorage` port, `FakeStorage`); Gesture classifier hold/consume; ritual sounds in the Sound
  gate; `InvitationWindow`; `AdoptionRitual` (single NVS write, fail closed, timeout).
- Local review: Must 4 · Should 6 applied (02b aligned, window press read before expiry +
  gauge clamp, edge tests); Nice grouped in #19.
- Codex review (2 rounds, no Copilot): two P2 threads, both verified and fixed in TDD — a Tap
  pending at the window close leaked to the normal loop, and a press whose edge was the
  settle-end tick was not consumed. `consumeCurrentPress()` now drops a pending Tap and returns
  to Idle at once when the pad is released; the window consumes at the settle end and on
  Declined (owner choice 465/500). Second round clean.
- 189 native tests green, both board envs build. NVS adapter and `main.cpp` wiring are F5.

### Brick F2 — behaviours — **PR #12 merged** (`ef10f61`, ADR-0014)

- Seven TDD steps, each a red then a green commit: Surprise/Joy blend from a held TouchDown;
  `GestureClassifier` ← `ITouchSource` (new 05c); `SoundClassifier` ← `IMic` rms + peak over an
  adaptive baseline (new 05d); `MoodEngine` (05a); `SleepCycle` (05b); `SoundGate` → `IAudio`
  (new 05e); `StimulusRouter` as pure functions (03 routing rule, S3 reactions, effect mappings).
- Owner decisions (scored options): stroke at 1.5 s, tap max 400 ms; sound thresholds as ratios
  over a baseline frozen during a sound; any TouchDown wakes; Waking covers its wake animation,
  never under 1 s (1 s from Dozing, 1.2 s from Asleep, PR #12 review);
  sounds are note tables in `lib/`, volume level 0..3 (V1 constant 1), gate muted while Dozing
  or Asleep via `setSleepState()`, overlap by rank (strictly higher replaces, no queue);
  `Released` is a mood stimulus so a medium press wakes a Bored or Tired mood (note in 05a).
- Codex review (5 rounds, no Copilot: out of credits), every thread answered inline. Fixed in
  TDD: Loud started a tick late; a clap leaked its loud run; an expired burst whose peak fell was
  dropped; Voice was timed from a low-rms bang (voice and loud now use their own runs of
  at-level ticks); the Asleep wake Yawn (1.2 s) outlived Waking (now 1 s from Dozing, 1.2 s from
  Asleep). Doc fix: the 03 sound-class table in ratio form (`e0df866`). Kept by owner decision:
  the 40 ms tick resolution of the double-tap window (05c note).
- 155 native tests green, both board envs build. `main.cpp` wiring and adapters are F5.
- Backlog: #13 (lib refactor), #15 (test nits), #16 (F5 wiring checks); #14 fixed in the PR.

### Brick F1b — reactions overlay and identity frames (branch `feat/f1b-expressions`, ADR-0014)

- TDD in two commits (red, then green) on the extraction trigger recorded in the F1 review:
  `Timeline` (start + duration + phase, wrap-safe, 64-bit phase) now drives blinks, lid moves,
  reactions and identity animations. `Reaction` enum (TouchDown, Surprise, Joy, Purr, LookUp,
  Yawn, Wake, Greet) drawn as an overlay on the mood frame; identity frames (invitation with
  gauge, ritual with pulsing border, adoption wink + arcs, error flashes) replace the face.
  Three layers: identity > reaction > mood. Blinks are deferred, never dropped.
- Pre-push `review-local` gate (design, docs, tests): Must Haves applied — identity frames now
  drift like the mood frame (REQ-NF-08), Purr has its own ramp/hold/release on the reaction
  timeline instead of borrowing the sleep lid move and never opens eyes the sleep cycle closed,
  the identity sequence diagrams 02a/02b now use the code names (`playAdoption`, `showError`,
  `clearIdentityFrame`, `showRitualFrame(palette[i])`), the no-blink-during-reaction test was
  vacuous (now seeded from the observed blink deadline, 8 seeds), the smile carve on a closing
  eye was non-monotonic at 18 px (edge depth now follows the centre depth). `Lids` flags
  became an `Arc` enum, tuning numbers named, static reactions keep one golden each.
- 73 native tests green, 28 golden states on the contact sheet (owner validated the first
  draft of the full face vocabulary on 2026-10-15), both board envs build (xiao_c3 273 KB,
  c3_supermini 298 KB flash).
- Open for F1c: TouchDown widening vs the double-tap window (owner decision), generalized
  12-parameter lids, `eyes_lab` native tool, emotion catalogue.

## 2026-10-14

### Brick F1 — firmware skeleton and eyes — **PR #9 merged** (`211137e`, ADR-0014)

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
- Golden images (10 states) generated and reviewed on the contact sheet. References are raw
  RGB565 dumps; the PNGs come from a minimal encoder of our own (stored deflate, CRC-32,
  Adler-32, known-answer tests). The vendored stb headers were dropped the next day: 9 700
  third-party lines and 13 high CodeQL alerts for two functions used (owner decision after
  scored options; the CodeQL exclusion alternative rejected).
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
