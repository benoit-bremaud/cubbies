# Local review — cubbies (branch diff)

- Date: 2026-10-14
- Commit: b28246c (findings applied in this amended commit)
- Scope: branch diff `main...HEAD` on `docs/pdr-cube-core` (cube-core PDR: eight diagrams, specs,
  traceability matrix, ADR-0013, ADR-0008 amendment note, roadmap, assembly check, CLAUDE.md rules,
  PROJECT_LOG)
- Checks run: design (architecture-guardian) · docs (doc-auditor) · correctness: datasheet facts
  of the decisions verified in the PDR session (ESP32-C3 RNG, TTP223 calibration, PCG32) ·
  security: not applicable (no code) · tests: not applicable
- Verdict: fix Must Have first, then ship · Must 6 · Should 15 · Nice 7 — **all Must and Should
  applied before push**

## 🔴 Must Have

- `ADR-0013:53,66` `[design|docs]` · Two scoring totals wrong (D2 option D = 26 not 23, D3
  option C = 19 not 20); E now beats D by one point. · Applied, with a sentence on why E still
  wins (ships in V1.x without V1.5 infrastructure).
- `identity/02a:44-50`, `02b`, `use-cases.md` UC1 `[design]` · The 2 s entry hold could be read
  as the 1.5 s confirming stroke and write the MAC-default colour to NVS. · Applied: the entry
  press is consumed by the Gesture classifier; first ritual gesture is the next pressed edge
  (diagram step, notes, UC1 step 1).
- `cube-core/03:66-67` `[design]` · The codec computes `moodByte(sleepState, wakeOrigin, mood)`
  but no `Social --> MoodE` edge was drawn; Waking's origin was not exposed. · Applied: edge
  added, `wakeOrigin` named in 03 and 05b.
- `cube-core/03:166` `[docs]` · "until the UC6 gesture is decided" after the decision. ·
  Applied.
- `docs/architecture/README.md:29` `[docs]` · Index still named "held-at-boot" as the only
  entry. · Applied.
- `CLAUDE.md:41-46` `[docs|design]` · Rules contradicted ADR-0013 (RNG injected, duty level,
  volume gesture). · Applied.

## 🟡 Should Have

- ADR-0013 amends ADR-0008 without saying so · Applied: status line, index row, forward note in
  ADR-0008.
- `03:78` BuzzerAudio "duty level" · Applied: "volume level → duty → tone".
- `README.md` "Maintenance" domain vs "Making" · Applied.
- `02a` hold starting late in the window undefined · Applied: a hold in progress extends the
  window by up to T_secret, gauge never resets mid-hold.
- `use-cases.md:78` UC7 "30 s" vs T_ttl 15 s · Applied.
- UC5 requirement lists differ between 01 and the spec · Applied: REQ-F-06, F-05, NF-08 in both.
- ADR-0013 D4 called the rare gag tier A while S12 is tier C · Applied: wording fixed.
- `requirements.md:59` "secret gesture" · Applied.
- `02b` confirm path lacked the tick plumbing · Applied: `M->>I: tick(stroke, nowMs)`.
- `02a`/`02b` colour applied before the ritual, not re-applied after · Applied: setColour
  re-applied with the final identity, 02b note reworded.
- PROJECT_LOG entry as prose · Applied: decisions as labelled bullets with rejected alternatives.
- `03` sound table: loud also wakes from Dozing · Applied.
- `03` tick budget 16 + 24 ms · Applied.
- `traceability-matrix.md` "sleep controller" → "Sleep cycle" · Applied.
- `ADR-0013` link to gates.md, T_settle_touch rationale, T_doubletap listed with the other
  gesture durations · Applied.

## 🔵 Nice to Have

- `03` Prng node without edges · Intentional (cross-cutting like `IClock`), label says so.
- Historical mentions of `IRng`, `localMac`, UC2, "held while plugging in" in explanatory or
  dated sentences · Kept, they record the change.
- The previous report (design dossier) mentioned UC12/UC13 · Superseded by this file (git keeps
  it).

## ⚪ Disagree / Intentional

- Deferred ovals UC8/UC9 and the `Team` actor in the use-case diagram `[design]` · Justified:
  the matrix needs a target for explicitly deferred tier-A goals.
- One `Prng` instance per consumer `[design]` · Keeps test sequences independent; ADR-0013 D1.
- `kFrameSize` included by the adapter from `lib/` `[design]` · src → lib is the allowed
  direction and removes a duplicated constant.
- Touch-down stimulus as a second gesture output `[design]` · Smallest change meeting REQ-NF-01
  with a 250 ms double-tap window.

## Per-dimension summary

- Design: dependency rule holds (no lib → src edge, Identity no longer depends on `IRadio`);
  one behavioural defect (entry press) and one missing edge fixed; no over-engineering.
- Docs: numbers consistent everywhere (1 s / 4 s / 2 s invitation, 250 ms double tap, 40 ms
  tick, 10 / 30 min sleep chain), UC numbering unified, every diagram keeps the template, all
  blocks render with `mmdc`, no broken relative link.
- Tests: not applicable; the native test seams (fakes per port, seeded Prng) are named in 03.
- Security: privacy boundary of the microphone written into 03; nothing else in scope.
- Correctness: facts behind the decisions verified against primary sources during the PDR.
