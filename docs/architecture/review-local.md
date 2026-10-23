# Local review — cubbies (feat/g5-collective-sleep)

- Date: 2026-10-23
- Commit: branch head (base `main` 31b40d0)
- Scope: branch diff main...HEAD: G5 collective sleep (issue #59) — `core/Mood.h` (`MoodTally::drowsy`),
  `core/MoodEngine.{h,cpp}` (yawn hold), `core/SocialProtocol.cpp` (drowsy count), native tests,
  social-group PDR docs (REQ-F-20, B4, 02d, 05d, traceability), roadmap, PROJECT_LOG
- Checks run: design · docs · tests · security · correctness
- Verdict: ship after the fixes below (applied) · Must 0 (1 doc Must fixed) · Should 3 (fixed) · Nice 6 (2 fixed)

## Pre-merge gate

| Step | Result |
|---|---|
| 1 Tests | ✅ 305 native (`pio test -e native`) · ✅ `pio run -e xiao_c3 -e c3_supermini` · ✅ benches PASS · ✅ ERC 0 · ✅ ruff |
| 2 Coverage | new lib lines 100 % executed (`MoodEngine.cpp` 89 % overall from the contagion suite alone; the uncovered lines are pre-G5, covered by `test_native_mood`) · docs: every new symbol commented |
| 3 SonarQube | ❌ FAILED on main-level conditions only (#53: 2 issues in `hardware/` Python outside the diff, new_coverage not imported) — owner waiver requested |
| 4 Secrets | ✅ gitleaks clean (main..HEAD) · no tracked .env · diff read: nothing sensitive |
| 5 QA pass | firmware, no UI: native tests and both board builds; bench check on two prototypes open (no new hardware path) |

- Gate: **GREEN except SonarQube (#53, waiver)**

## 🔴 Must Have
- `docs/architecture/diagrams/social-group/05d-state-yawn.md` row 6 `[docs]` · touch-down does not move Bored to Curious, it only breaks the calm · **fixed**: row split into 6 / 6b.

## 🟡 Should Have
- `05d-state-yawn.md` row 11 `[docs]` · caught yawn resets the calm, undocumented · **fixed**.
- `core/SocialProtocol.cpp:204` `[docs]` · stale comment on byte 5 · **fixed**.
- `test_contagion.cpp` `[tests]` · calm term of the yawn untested (mutation survived) · **fixed**: `test_yawn_row6b_touch_down_mid_hold_waits_fresh_calm`.

## 🔵 Nice to Have
- `test_contagion.cpp` row 2 `[tests]` · Annoyed not covered · **fixed**.
- `core/Mood.h`, `core/MoodEngine.h` `[docs]` · comments mention drowsy / caught yawn · **fixed**.
- `test_yawn_row11` `[tests]` · dropping `holding_ = false` on catch stays green (G2 re-holds and waits a fresh calm anyway) · documented, behaviour equivalent.
- row 12 example builds the tally by hand, codec path tested separately in `test_native_social` · accepted (unit level).
- row 10 (not Awake) lives in `main.cpp` (engine paused) · covered by the existing G2 wiring, no new adapter test.
- `catchesYawn` also mutates the hold · name kept, comment in the header.

## ⚪ Disagree / Intentional
- Yawn hold duplicates the G2 hold shape · second occurrence only, Rule of Three.
- A sleeping neighbour makes any cube that turns Bored beside it Tired 6 s later · by design (D1, owner 2026-10-23); forged frames accepted under ADR-0012.

## Per-dimension summary
- Design: one guarded transition, no new frame, lib pure · Docs: two table rows aligned · Tests: every 05d row tested, calm term now pinned · Security: bounded counter, allowlisted mood byte · Correctness: no bug, tick order correct.
