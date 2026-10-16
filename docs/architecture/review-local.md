# Local review — cubbies (feat/expansion-port-design)

- Date: 2026-10-16
- Commit: 3c8c7c7 (+ review fixes in the next commit)
- Scope: branch diff main...HEAD (docs, ADR-0016, C-0012 in `calc.py`)
- Checks run: design · docs · correctness · security (one general-purpose reviewer, numbers
  recomputed by hand); tests dimension: no test code changed
- Verdict: ship after fixes (applied) · Must 2 · Should 6 · Nice 4

## Pre-merge gate (head SHA after the review-fix commit)
| Step | Result |
|---|---|
| 1 Tests | ✅ 220/220 native (`pio test -e native`, firmware unchanged) · ✅ `sh hardware/electronics/sim/run.sh`: all benches PASS, C-0012 asserts pass |
| 2 Coverage | Python 94 % (coverage.py over `calc.py`, `schematic_values.py`, its test); firmware unchanged; no new functions to document |
| 3 SonarQube | project `cubbies` (localhost:9000): 0 new issues, 0 % duplication, hotspots reviewed; ✅ QUALITY GATE PASSED (first scan without a coverage report failed on coverage 0 %) |
| 4 Secrets | ✅ gitleaks clean (main..HEAD) · no tracked .env · diff read: nothing sensitive |
| 5 QA pass | ✅ no UI: real artefacts exercised — `calc.py` output, benches; Mermaid views reviewed |
- Gate: **GREEN — merge authorised on owner approval**

## 🔴 Must Have
- `docs/hardware/electronics-spec.md:49` `[correctness]` · 3V3 row still said "typical ≤ 300 mA", which C-0002/C-0012 contradict · **fixed**: sustained ≤ 231 mA in total, module included.
- `docs/decisions/ADR-0016-expansion-port.md:23` `[docs]` · peak trimmed from the validated 80 mA to 75 mA · **fixed**: written down; owner to confirm in the PR.

## 🟡 Should Have
- `docs/hardware/calculations/log.md` (C-0012) `[correctness]` · "2× margin" ambiguous · **fixed**: defined, 1.48× on the total stated.
- `diagrams/expansion-port/02-sequence-boot-scan.md` + ADR D3 `[security]` · stuck bus / unbounded read could block boot · **fixed**: single-byte read with timeout, bus recovery, "absent" on failure, rules row.
- same `[design]` · reserved addresses not enforced · **fixed**: allowlist excludes them, native test required.
- `diagrams/expansion-port/03-component.md` `[docs]` · Mermaid labels, bus drawn as a series element · **fixed**.
- `diagrams/expansion-port/02-…` `[docs]` · no use-case view in the folder · **fixed**: links cube-core/01.
- UC12 actors `[docs]` · Owner vs Colleague · ⚪ already consistent: the diagram declares Colleague as the cube's owner.

## 🔵 Nice to Have
- ADR-0016 `[docs]` · D1 matrix · **fixed** (summary).
- UC12 3b `[tests]` · **fixed**: verified at the module PDR.
- `docs/hardware/mechanical-spec.md` `[correctness]` · 1 inch module wider than the 20 mm bay · **fixed**: modules ≤ 20 × 20 mm, bay 22 × 22 × 6 mm.
- ADR-0016 status Accepted while CDR items are open `[docs]` · ⚪ intentional: the OPEN items are bench checks, as in ADR-0015.

## ⚪ Disagree / Intentional
- See the two ⚪ items above.

## Per-dimension summary
- Design: clean dependency direction, no new port until the first module · Docs: cross-references fixed · Tests: none changed · Security: probe hardened in the design · Correctness: C-0012 arithmetic verified by hand
