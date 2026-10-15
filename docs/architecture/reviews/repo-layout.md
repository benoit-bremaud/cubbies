# Local review — cubbies (branch diff)

- Date: 2026-10-15
- Commit: chore/repo-layout (findings applied before the first push)
- Scope: repository reorganisation, no firmware change: `hardware/case` → `mechanics/`,
  `docs/hardware` split into `docs/electronics/` and `docs/mechanics/`, `electronics/` and
  `web/` reserved with a README each, `okh.yml`, `docs/architecture/audits.md`, CLAUDE.md
  layout / dependency rule / scopes, README, ADR path notes, PROJECT_LOG
- Checks run: docs (doc-auditor) · design (architecture-guardian) · link checker over every
  relative Markdown link · `pio test -e native` (74 green, unchanged)
- Verdict: fix Must Have first, then ship · Must 4 · Should 7 · Nice 4 — **all Must and Should
  applied before push**

## 🔴 Must Have

- `docs/mechanics/assembly.md:28` `[docs]` · Link to `electronics-spec.md` broken by the split. ·
  Applied: `../electronics/electronics-spec.md`.
- `mechanics/README.md:17` `[docs]` · Claimed a repository `.mcp.json` that is not committed. ·
  Applied: "owner-local MCP configuration".
- `CLAUDE.md` dependency rule `[design]` · Said "one-way" then used two-way arrows; silent on the
  goldens a web test must read, on the enclosure needing board dimensions, on root aggregators,
  and on which side owns the GPIO map. · Applied: explicit allow-list, GPIO contract named as the
  source of truth, `pins.h` hand-written against it.
- `okh.yml` licence block `[design]` · `hardware:` covered `mechanics/` only; a schematic would
  silently fall under MIT. · Applied: note that the first KiCad PR decides it (superseding ADR).

## 🟡 Should Have

- `audits.md` three per-brick rows `[design]` · One `review-local` event described three times. ·
  Applied: one "Per-brick review" row.
- `audits.md` trace path `[design]` · A single `review-local.md` overwritten by every branch. ·
  Applied: `docs/architecture/reviews/<branch>.md`; the F1b report moved there.
- `audits.md` debt review "every five bricks" `[design]` · No counter, would not happen. ·
  Applied: folded into the CDR checklist in `gates.md`.
- `audits.md` mechanics checklist `[design]` · Written before any script exists, copied a spec
  value. · Applied: deferred to the first `cubbies_case.py` PR, one-line summary kept.
- `audits.md` electronics row `[docs]` · Claimed an ERC CI job that does not exist; the V-SEA
  skill is not in `~/.claude/skills/`. · Applied: job attributed to the first KiCad PR, skill path
  given and to be copied then.
- `CLAUDE.md` scopes `[design]` · `docs` vs discipline overlap, `eyes` not a directory, the
  bridge exception missing. · Applied.
- `okh.yml` `date-updated`, "measurements" claims `[docs]` · Hand-maintained date would go stale;
  `measurements.md` does not exist. · Applied: date dropped, "to come" markers.

## 🔵 Nice to Have

- CLAUDE.md audits section duplicated `audits.md` · Applied: one line.
- `web/README.md` cites ADR-0015 before it exists · Kept: says "to be written"; the first web PR
  must add the ADR.
- `electronics/README.md` and `web/README.md` as placeholders · Kept: they hold the decision and
  the dependency rule, no empty scaffolding.
- Dependabot `npm` for `/web` and a web CI job · Deferred to the simulator PR.

## ⚪ Disagree / Intentional

- ADR-0009/0010 bodies still name `hardware/case/` · ADR bodies are immutable; dated path notes
  appended instead.
- `okh.yml` kept although no KiCad file exists · 40 lines, licences per discipline readable by
  machines, sommaire of the three disciplines; `schematics: []` matches the tree.

## Per-dimension summary

- Design: one discipline per root directory, explicit cross-discipline allow-list, CI untouched
  (`working-directory: firmware`, no `paths:` filter).
- Docs: no stale path outside ADR history, no broken relative link, placeholders say what will
  live there and why.
- Tests: unchanged, 74 native tests green.
- Security: no new input, no secret, no workflow change.
