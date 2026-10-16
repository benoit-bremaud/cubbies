# Local review — cubbies (feat/kicad-schematic)

- Date: 2026-10-16
- Commit: cc00f9c (+ review fixes in the next commit)
- Scope: branch diff main...HEAD
- Checks run: design · docs · tests (doc-auditor, three dimensions) · correctness · security
  (general-purpose reviewer, schematic parsed and netlist exported)
- Verdict: ship after fixes (applied) · Must 1 · Should 9 · Nice 7 · Sonar: 1 issue fixed (SystemExit swallowed in the test → ValueError)

## Pre-merge gate (head SHA after the review-fix commit)
| Step | Result |
|---|---|
| 1 Tests | ✅ 220/220 native (`pio test -e native`, firmware unchanged) · ✅ `sh hardware/electronics/sim/run.sh`: all benches PASS + parser self-test · ✅ ERC 0 violations |
| 2 Coverage | Python new code 93.4 % (Sonar), `schematic_values.py` 90 %; firmware unchanged; docs 2/2 functions |
| 3 SonarQube | project `cubbies` (localhost:9000): new coverage 93.4 % (Python, coverage.py), 0 new issues, 0 % duplication; 1 regex hotspot reviewed Safe (input = sheet values); ✅ QUALITY GATE PASSED |
| 4 Secrets | ✅ gitleaks clean (main..HEAD) · no tracked .env · diff read: nothing sensitive, no absolute paths in the schematic |
| 5 QA pass | ✅ no UI: real artefacts exercised — kicad-cli ERC, PDF export inspected, netlist export, benches; sensitivity: R1 = 68 Ω on the sheet makes `run.sh` fail on C-0003 |
- Gate: **GREEN — merge authorised on owner approval**

## 🔴 Must Have
- `docs/hardware/calculations/README.md:33` `[docs]` · said "no KiCad schematic yet" · **fixed**: values now from `schematic.lib`, corners stay in benches.

## 🟡 Should Have
- `hardware/electronics/sim/i2c-bus.cir:29` `[correctness|docs]` · hard-coded module conductances (two 0.5 µS off) · **fixed**: computed expressions, network/tolerance comment.
- `hardware/electronics/sim/schematic_values.py:28` `[correctness|tests]` · `1 µF` would read 1 F; parser untested · **fixed**: spaced units rejected, `test_schematic_values.py` run by `run.sh`.
- `hardware/electronics/sim/run.sh:11` `[design]` · silent fallback to committed lib · **fixed**: message printed; keep-going comment.
- `hardware/electronics/sim/buzzer-driver.cir:6`, `oled-reset-rc.cir:8` `[docs]` · headers hard-coded values · **fixed**: name R1–R4/C1 as schematic values.
- `hardware/electronics/sim/strapping-gpio2.cir:23` `[docs]` · cryptic expression · **fixed**: comment.
- `PROJECT_LOG.md:10` `[docs]` · completeness · **fixed** in the log entry.

## 🔵 Nice to Have
- `schematic_values.py` `[correctness]` · ref filter `[RC]\d+`, kicad-cli stderr surfaced · **fixed**.
- `cubbies-v1.kicad_sch` `[docs]` · module symbols use a logical pin order · **fixed**: note on the sheet.
- CI check that regenerating `schematic.lib` / the PDF leaves the tree clean · open (CI has no kicad-cli).
- Draft RC negative control uses R4 with literal 100 nF · accepted.
- Sheet layout is functional, not polished (overlapping power symbols on adjacent pins) · tidy in KiCad later.

## ⚪ Disagree / Intentional
- Tolerances and corners kept in the benches, nominal values from the sheet (V-SEA method).
- `.param` + `.csparam` per part: one for netlist expressions, one for `.control` blocks.

## Per-dimension summary
- Design: proportionate (one pure parser, no classes) · Docs: one stale paragraph fixed · Tests: benches self-validating, parser test added · Security: nothing sensitive, subprocess list-form · Correctness: nets match the GPIO contract and assembly.md pin by pin.
