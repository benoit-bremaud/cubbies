# Local review — cubbies (feat/42-freecad-case)

- Date: 2026-10-19
- Commit: 76b1190 (+ review fixes 0103da7, 2321680 and the final commit of the branch)
- Scope: branch diff main...HEAD: `hardware/case/cubbies_case.py` (parametric FreeCAD enclosure,
  issue #42), its generated exports, mechanical spec, verification plan, case README
- Checks run: design (architecture-guardian) · docs (doc-auditor) · correctness + security
  (geometry reviewer, figures recomputed by hand) · tests = the scripted dimension checks,
  mutation-tested
- Verdict: ship after fixes (applied) · Must 4 · Should 9 · Nice 4

## Pre-merge gate (head of the branch)
| Step | Result |
|---|---|
| 1 Tests | ✅ 220 native Unity tests · ✅ `pio run` xiao_c3 + c3_supermini · ✅ `sh run.sh` (ngspice benches, crosscheck) · ✅ `kicad-cli sch erc` 0 · ✅ 17 FreeCAD dimension checks, 2 mutations caught (reversed barb, vent past the corner) |
| 2 Coverage | Python new code 93.9 % (Sonar); `cubbies_case.py` 91.9 % lines under FreeCAD's Python (export path not run in check mode) · docs 20/22 functions (91 %; `box` and `centered_box` are self-describing) |
| 3 SonarQube | ✅ PASSED (after fixing S3776 twice: `shell_hooks` 30, then `run_checks` 16) |
| 4 Secrets | ✅ gitleaks `main..HEAD` clean · no tracked `.env*` · diff read: geometry and docs only |
| 5 QA pass | ✅ no UI: the product is the geometry. Script run headless, exports regenerated, six-view render of the STLs in `docs/architecture/qa/2026-10-19-case-parts.png`. Print-and-fit stays the mechanical CDR bench step |
- Gate: **GREEN — merge authorised on owner approval**

## 🔴 Must Have
- `hardware/case/cubbies_case.py` `barb` `[correctness]` · barb step led on insertion, so the shells could not close · ramp now leads, step retains; check "barb ramps lead on insertion" (fails on the old profile) · fixed
- `hardware/case/cubbies_case.py` `rear_cover` `[correctness]` · cover barbs stopped 1.5 mm short of the wall and nothing stopped the cover being pushed in · plate seated in a rebate with 1 mm ledges, 6 mm hooks catching the wall's inner face; checks "barbs engage" and "push-in stop" · fixed
- `hardware/case/cubbies_case.py` vents `[correctness][design]` · fourth slot cut the right wall and rounded corner, no check caught it · 4 horizontal slots at the bottom right on the flat face; vents now checked against the corner · fixed
- `hardware/case/cubbies_case.py` hooks `[correctness]` · 2 mm cover hooks strained ~18 % (PLA breaks at 2–3 %), shell hooks 3.6 % · 1.0 mm arms, 0.5 mm barbs, 6 mm cover hooks: 2.2 % / 1.3 %; check against a 2.5 % limit · fixed
- `docs/hardware/mechanical-spec.md` `[docs]` · "about 20 mm" free depth, the code gives 26 mm · fixed (merged with the Must above for the count)

## 🟡 Should Have
- `cubbies_case.py` J1 check `[design][correctness]` · compared constants only · relabelled "(parameters)"; J1 is placed at the fit · fixed
- `cubbies_case.py` bay check `[correctness]` · volume equality was vacuous · now: bay inside the cavity, no intrusion, ≥ 0.2 mm to the cover hooks · fixed
- `cubbies_case.py` checks `[tests]` · no check that the openings exist · `openings_in_place` probes window, USB-C, vents, mic holes, dimple · fixed
- `cubbies_case.py` constants `[design]` · mixed tuple axis orders, unused `OLED_GLASS[2]` and `J1[1]` · axis orders commented, bay as named scalars, dead fields removed · fixed
- `cubbies_case.py` `[design]` · magic `0.5`, `1.2 + 1.6`, `arm_len = 2.0` · `BARB_OFFSET`, `XIAO_PCB`, `USB_C_HEIGHT`, `COVER_HOOK_LENGTH` · fixed
- `cubbies_case.py` `REAR_COVER_HOOKS` `[design]` · only meaningful at 2 (YAGNI) · removed, always 2 · fixed
- `cubbies_case.py` `wedge_barb` `[design]` · dead branch, float equality · `barb()` always extrudes along z · fixed
- `cubbies_case.py` wall probes `[design][tests]` · hand-placed · back probe derived from the rebate, others commented · fixed
- `cubbies_case.py` docstrings `[docs]` · `export`, `main`, shells undocumented · added · fixed

## 🔵 Nice to Have
- `cubbies_case.py` XIAO position written twice `[design]` · `xiao_box()` shared by the rails and the check · fixed
- `cubbies_case.py` notch `[correctness]` · groove on the plate gave no purchase · recess cut in the wall above the plate · fixed
- `cubbies_case.py` dimple, USB constants `[docs]` · WHY comments added · fixed
- `firmware/tools/golden_sheet.py`, `hardware/electronics/sim/calc.py` `[style]` · pre-existing ruff findings on `main`, outside this diff · follow-up

## ⚪ Disagree / Intentional
- `cubbies_case.py` module-level `main()` `[design]` · freecadcmd runs the file under its file name, not `__main__` (verified), so an `if __name__` guard would never run · documented in `main`'s docstring
- `cubbies_case.py` `box`, `centered_box` without docstrings `[docs]` · self-describing one-liners

## Per-dimension summary
- Design: flat functions fit a ~400-line script; small YAGNI items removed · Docs: spec, plan and README match the code, 91 % docstrings · Tests: checks now exercise the real geometry, mutation-tested · Security: no issue (writes only under `export/`, no subprocess, no input) · Correctness: four snap-fit and vent defects found and fixed before any print
