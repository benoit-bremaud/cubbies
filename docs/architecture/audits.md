# Recurring audits

`gates.md` holds the one-off reviews (PDR per feature, CDR before the series). This file
schedules the **recurring** audits decided on 2026-10-15: what triggers each one, which tool
runs it, what blocks a merge, and where the trace lives. The tools are the skills and agents
already on the owner's machine; this file only fixes when they run. Findings follow the
`review-local` grading (Must Have blocks, Should Have applied or argued, Nice to Have logged),
and every "Kept" names the trigger that reopens it.

| Audit | Trigger | Tool | Blocks when | Trace |
|---|---|---|---|---|
| Per-brick review | every firmware or web brick, before push | `review-local`: `architecture-guardian` (design patterns, anti-patterns, SOLID, dependency rule), `test-auditor` (pyramid, FIRST, mock boundaries only, no vacuous test), `doc-auditor` (contracts, stale comments, over-documentation) | any Must Have: dependency rule broken, speculative abstraction, a GoF pattern without a real force, a test that cannot fail, a comment contradicting the code | `docs/architecture/reviews/<branch>.md` |
| Electronics | every PR under `electronics/` | `electronics-design-conventions` skill (V-SEA, `~/Documents/01_work/vev/vev-smart-evse-adapter/.claude/skills/`, to copy into `~/.claude/skills/` with the first KiCad PR): sheet hierarchy and label scope, IEEE 315 designators, ERC hygiene, KLC, DFM/BOM, version control; `kicad-cli sch erc` as a CI job added by the first KiCad PR | ERC not empty, ESP32-C3 strapping pins (GPIO2, 8, 9) not checked, GPIO table ≠ `firmware/include/pins.h`, a value without its calculation in `docs/electronics/calculations.md` | the PR and `docs/electronics/` |
| Mechanics | every PR under `mechanics/` | checklist written with the first `cubbies_case.py` PR (parameters only, exports regenerated, print tolerances traced to `mechanical-spec.md`, CC-compatible sources credited) | an export edited by hand, a dimension outside the parameter block | the PR |
| Security | bricks touching radio (F4), storage (F3), the web app, the V1.x setup portal, and every CI change | `security-policy` and `security-ci-baseline` skills; CodeQL, Gitleaks, Scorecard, dependency review already in CI; threat model review against ADR-0012 for ESP-NOW | a secret in source or build flags, an unbounded input (frame, NVS value, form field), an unpinned action, a widened token permission | an ADR for a design change, the PR otherwise |

The debt review (re-reading the "Kept" and "Disagree" items of past reports) is a CDR entry
criterion in `gates.md`, not a separate cadence.

## When a row changes

Adding a discipline (a new root directory) adds a row here and an entry in `okh.yml`. A tool
replacement (another agent, another linter) edits the row; the trigger and the blocking rule
are decisions and change through the PR that argues them.
