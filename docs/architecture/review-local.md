# Local review — cubbies (branch diff)

- Date: 2026-10-14
- Commit: b5a7449 (findings applied in the amended commit that replaces it)
- Scope: branch diff `main...HEAD` on `docs/design-dossier` (design dossier: requirements, use
  cases, verification plan, gates, traceability matrix, hardware specs, ADR-0010/0011, research,
  roadmap, CLAUDE.md, pins.h comments)
- Checks run: design (architecture-guardian) · docs (doc-auditor) · correctness (technical
  fact-check of every hardware value against datasheets) · security: not applicable (no code; the
  privacy constraint C7 was reviewed by the design pass) · tests: not applicable
- Verdict: fix Must Have first, then ship · Must 8 · Should 14 · Nice 9 — **all Must and Should
  applied before push**, Nice applied where one line sufficed

## 🔴 Must Have

- `docs/architecture/traceability-matrix.md` `[design|docs]` · Links to diagrams that exist only
  on `docs/uml-study`; broken on `main` if merged first. · Applied: header states the dependency;
  merge order decided: `docs/uml-study` merges first.
- `traceability-matrix.md` `[design]` · Port names drifted from the UML component diagram
  (`ITouch`, `ISensors` grab-bag). · Applied: `ITouchSource`, `IMic`, `IMotion` (created at S9),
  one port per source.
- `firmware/include/pins.h:27` `[design]` · Comment claimed GPIO20/21 free while SCL is on
  GPIO21. · Applied: comment corrected.
- `electronics-spec.md`, `calculations.md` `[design|correctness]` · Buzzer on strapping pins
  (GPIO2 XIAO, GPIO8 Super Mini) with a gate pull-down, undiscussed. · Applied: datasheet facts
  recorded, CDR checklist item added, swap plan stated.
- `electronics-spec.md:55` `[correctness]` · SPI 8 MHz exceeds the SSD1331 maximum (6.67 MHz).
  · Applied: 6 MHz.
- `calculations.md:10` `[correctness]` · Wi-Fi TX peak 300 mA not in the datasheet (335 mA at
  1 Mbps). · Applied, total peak ≈ 420 mA.
- `calculations.md:28` `[correctness]` · R_s = 47 Ω drives 52 mA into a buzzer rated 40 mA. ·
  Applied: 68 Ω → 39 mA; AO3400 only (2N7002 unspecified at 3.3 V gate drive).
- `CLAUDE.md:43`, `roadmap.md` S4/S8 `[docs]` · Stale "quiet hours" wording. · Intentional:
  those lines are rewritten on `docs/uml-study` (owner-validated); duplicating the edit would
  conflict. They arrive with that merge.

## 🟡 Should Have

- `ADR-0011` totals `[docs]` · Two scoring totals wrong (G 26, E 32). · Applied.
- `requirements.md` `[design]` · Solutions leaking into requirements (buzzer, Sound gate,
  neighbour table, audio adapter, 96×64). · Applied: reworded solution-neutral; the Sound gate
  lives in the matrix and the component diagram.
- `requirements.md` `[design|docs]` · IMU fitted without a requirement. · Applied: REQ-F-15
  (tier B, S9) with the "fitted in V1" rationale.
- `gates.md` `[design]` · Full PDR on every backlog item. · Applied: scoped to features that add
  a requirement, port, frame field or state.
- `verification-plan.md`, `calculations.md` `[design]` · ngspice for an RC with 500× margin and
  for decoupling. · Applied: ngspice kept for the buzzer stage only; RESET verified by hand
  calculation + 10 cold boots.
- `use-cases.md` `[docs]` · UC11/UC12 were not actor-initiated goals; UC11 cited C8. · Applied:
  "not use cases" list B1–B3, UC13 renumbered UC11.
- `requirements.md` C10 `[docs]` · `IClock` could read as a wall clock. · Applied: monotonic
  uptime stated.
- `electronics-spec.md:32` `[docs]` · "confirmed by simulation" before any simulation. ·
  Applied.
- `electronics-spec.md:62` `[docs]` · Garbled sentence on RESET. · Applied.
- `calculations.md:16` `[docs|correctness]` · Buzzer peak 30 mA vs 52 mA computed. · Applied
  (39 mA after resizing).
- `calculations.md` `[correctness]` · LDO part and dissipation missing; INMP441 1.5 mA is the
  1.8 V figure; OLED typical overstated; reset pulse ≥ 3 µs not 2; GPIO drive default 10/20 mA.
  · Applied.
- `mechanical-spec.md` `[correctness]` · Window 20.5 × 14 mm below print tolerance over the
  active area; module size unverifiable. · Applied: window on the viewing area 22.2 × 15.5 mm,
  SKU to name at the CDR.
- `PROJECT_LOG.md` `[docs]` · Counts wrong (constraints, UCs); KiCad method claim without source.
  · Applied (10 constraints, 11 UCs + 3 behaviours; "provisional, no ADR yet").
- `requirements.md`, matrix, verification plan `[design]` · REQ↔UC mapping written three times.
  · Applied: the Acceptance column removed from requirements; UCs own UC→REQ, the matrix owns
  REQ→design→verification.

## 🔵 Nice to Have

- `requirements.md` C9/C10 order and C9 criterion · Applied.
- `ADR-0011` option letters A/E/F/G · Applied: note on discarded variants.
- `ADR-0010` heaviest weight is a tooling convenience · Applied: stated in Context.
- `assembly.md` "3 V tone" · Applied: 3.3 V square wave at 2 kHz on the gate.
- `electronics-spec.md` "NPN or N-MOSFET" vs calculations · Applied: N-MOSFET.
- `calculations.md` wall thickness 1.5–2 mm vs 1.6 mm · Applied.
- `traceability-matrix.md` `IRng` port vs lib-owned seeded generator · Deferred: recorded as a
  PDR gap; the UML study owns the component diagram.
- `calculations.md` TTP223 "3 mm of plastic" unverifiable · Applied: "measured on the prototype".
- `docs/hardware/measurements.md` cited but absent · Intentional: filled at bench time.

## ⚪ Disagree / Intentional

- The Sound gate in `lib/` `[design]` · Domain policy (volume, silence asleep/dark, gag rate
  limits) at the single exit point for sound; one plain class calling `IAudio`, no interface of
  its own. Kept.
- The CDR gate `[design]` · Runs once before ordering parts for > 10 units; proportionate. Kept.
- Constraints C3/C5/C6 restating ADRs `[design]` · Accepted decisions are constraints, not
  solutions in disguise. Kept.
- Five hardware/architecture documents for a scaffold `[design]` · The owner required a formal
  design dossier before any code; each file has one role and is short. Kept.

## Per-dimension summary

- Design: ports aligned with the UML study, PDR scoped, no dependency-rule violation found,
  one speculative port (`ISensors`) removed, `IRng` flagged for the next PDR.
- Docs: 100 % of planned artefacts present, 6 accuracy fixes, counts corrected, no doc-noise.
- Tests: not applicable (no code); firmware builds re-run after the `pins.h` comment edits
  (both envs succeed).
- Security: not applicable to this diff; privacy constraint C7 and REQ-NF-07 carried into the
  matrix.
- Correctness: 17 hardware claims checked against datasheets; 6 wrong values corrected (SPI
  clock, TX peak, buzzer current, reset pulse, INMP441 current, GPIO drive), 3 unverifiable
  deferred to the CDR with the SKU choice.
