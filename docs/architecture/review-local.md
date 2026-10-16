# Local review — cubbies (docs/cdr-paper-items)

- Date: 2026-10-16
- Commit: 94066d9 (+ review fixes 6657386, a7eee9d)
- Scope: branch diff main...HEAD (C-0013 XC6210 LDO, C-0014 GPIO8 pull-up R6, C-0015 second
  method `crosscheck.py`; issues #30, #31, #32)
- Checks run: correctness + design + security (one reviewer on Opus, numbers recomputed by hand)
  · docs + tests (doc-auditor, test-quality angle)
- Verdict: ship after fixes (applied) · Must 5 · Should 9 · Nice 4

## Pre-merge gate (head SHA a7eee9d)
| Step | Result |
|---|---|
| 1 Tests | ✅ `sh hardware/electronics/sim/run.sh`: 5 benches PASS, `test_schematic_values.py`, `test_crosscheck.py`, `crosscheck.py` 139 rows agree; firmware unchanged (native suite not affected) |
| 2 Coverage | Python 96 % (coverage.py over `calc.py`, `crosscheck.py`, `schematic_values.py` and both tests); every new function documented |
| 3 SonarQube | project `cubbies` (localhost:9000): ✅ QUALITY GATE PASSED after fixing 1 issue (caught SystemExit) and a regex backtracking hotspot (regex removed) |
| 4 Secrets | ✅ gitleaks clean (origin/main..HEAD) · no tracked .env · diff read: datasheet figures only |
| 5 QA pass | ✅ no UI: the artefact is the bench suite (`run.sh`) and the schematic (`kicad-cli sch erc`: 0 violations, PDF re-exported) |
- Gate: **GREEN — merge authorised** (on owner approval)

## 🔴 Must Have
- `sim/buzzer-driver.cir:9`, `sim/calc.py` C-0005/C-0010 `[correctness|docs]` · the rail corners still used the TLV75733 window 3.25–3.35 V · fixed: 3.234/3.366 V everywhere; buzzer peak 37.4 mA, recorded in C-0013.
- `docs/architecture/verification-plan.md:29` `[docs]` · "18 corners" for I2C · fixed: 48 + C-0015.
- `docs/hardware/calculations/README.md:47` `[docs]` · Tier 1 row said the second engine was open · fixed.
- `sim/crosscheck.py` reset RC `[correctness|tests]` · the "never low" case was skipped · fixed: checked.
- `sim/crosscheck.py` `[tests]` · no self-test, sensitivity claim manual · fixed: `test_crosscheck.py` in `run.sh`.

## 🟡 Should Have
- `sim/strapping-gpio2.cir` G8 control `[correctness]` · tautological (−105 V) · fixed: floating input modelled mid-rail.
- VIH 2.52 V rounded down `[correctness]` · fixed: 2.525 V.
- C-0014 text: R6 load 0.35 mA, case B rail wording `[docs]` · fixed.
- `sim/crosscheck.py` tolerances 30–5000× the observed deviation `[tests]` · fixed: about 5×, stated.
- `sim/crosscheck.py` `rows()` silently dropped rows `[tests]` · fixed: expected counts 48/24/64.
- Strapping constants mirror the bench `[design]` · fixed: documented as a wiring guard only.
- `agree()` contract undocumented `[docs]` · fixed.
- Module ran at import time `[tests]` · fixed: `main()`.
- Dropout scaled from 25 °C `[correctness]` · fixed: stated in calc.py and C-0013.

## 🔵 Nice to Have
- `.gitignore` `coverage.xml` · fixed.
- calc.py header entries · fixed.
- `review-local.md` previous verdict quotes 231 mA · historical record, left.
- `run.sh` `^G8` filter comment · left (the header lists the summary lines).

## ⚪ Disagree / Intentional
- `log.md` C-0002/C-0003/C-0006/C-0007/C-0008/C-0012 still say TLV75733 or "second engine OPEN" · the log is append only; C-0013/C-0014/C-0015 carry the Supersedes lines.

## Per-dimension summary
- Design: one model per bench, no over-engineering · Docs: stale references fixed · Tests: the second method is now self-tested and sensitive · Security: no secrets, regex hotspot removed · Correctness: numbers recomputed by hand and matching.
