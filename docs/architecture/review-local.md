# Local review — cubbies (feat/g2-contagious-mood-52)

- Date: 2026-10-22
- Commit: branch head after the review fixes (base `origin/main` 0361300)
- Scope: branch diff main...HEAD: G2 contagious mood (issue #52) — `core/Mood.{h,cpp}`,
  `core/MoodEngine.{h,cpp}`, `core/SocialProtocol.{h,cpp}`, `src/main.cpp`, native tests,
  social-group PDR docs
- Checks run: design (architecture-guardian) · docs (doc-auditor) · tests (test-auditor, mental
  mutation pass) · security + correctness (Opus reviewer)
- Verdict: ship after fixes (applied) · Must 0 · Should 9 · Nice 7

## Pre-merge gate
| Step | Result |
|---|---|
| 1 Tests | ✅ 237 native Unity tests (16 contagion, tally test in social) · ✅ `pio run` xiao_c3 + c3_supermini · ✅ `sh run.sh` exit 0 (benches PASS; the "FAIL C-0015 tr" line predates this branch) |
| 2 Coverage | new lib code 30/30 lines (100 %, gcov per function: `contagion` 17/17, `pulledMood` 8/8, `moodTally` 5/5) · docs 6/6 new public symbols (100 %) |
| 3 SonarQube | ❌ FAILED on the project, not on this diff: `new_violations` 1 = python:S3776 in `hardware/case/cubbies_case.py:380` (from PR #46), `new_coverage` 0 % = no Python `coverage.xml` supplied (Community Build does not analyse C++). This branch changes no Python file. Owner waiver or a separate fix needed |
| 4 Secrets | ✅ gitleaks clean · no tracked `.env*` · diff read: logic, tests, docs only |
| 5 QA pass | ✅ no UI: the product is the firmware — native suite + both board images build; two-cube bench ripple optional |
- Gate: **RED on step 3 (pre-existing, outside the diff)** — all other steps green

## 🔴 Must Have
_None._

## 🟡 Should Have
- `test_contagion.cpp` row 2 `[tests]` · survived dropping `target == mood_` · asserts the refractory was not consumed · fixed
- `test_contagion.cpp` `[tests]` · no Holding → NoCandidate test · `test_losing_the_candidate_drops_the_hold` · fixed
- `test_contagion.cpp` row 7 `[tests]` · refractory lower edge unpinned · one tick before / exactly at T_refractory · fixed
- `test_contagion.cpp` row 4 `[tests]` · `lastAnyMs_` reset untested · `test_pull_restarts_the_idle_timer` · fixed
- `test_contagion.cpp` worked example `[tests]` · `pullsA <= 1` accepted 0 · exact 1 · fixed
- `test_contagion.cpp` row 11 `[tests]` · refractory kept across restart untested · `test_restart_keeps_the_refractory` · fixed
- `MoodEngine.h` `lastLoudMs_` `[design][docs]` · comment stale (pull sets it) · fixed
- `MoodEngine.h` `contagion()` `[docs]` · entry effect, first-pull and restart contract missing; constants without WHY · fixed
- `Mood.cpp`, `SocialProtocol.cpp` `[docs]` · ambiguous "05a"; asleep exclusion implicit · "social-group/05a", comment · fixed

## 🔵 Nice to Have
- `Mood.cpp` `[design]` · `static_assert` ties `kPriority` to `kMoodCount` · fixed
- `MoodEngine.cpp` `[correctness]` · `Mood target = mood_` (no maybe-uninitialised) · fixed
- `MoodEngine.h` header `[docs]` · mentions the tally input · fixed
- `test_social.cpp` tally `[tests]` · Bored vote and a Waiting entry asserted · fixed
- `MoodEngine.cpp` refractory `[correctness]` · after 49.7 days without a pull the wrap can block one pull for ≤ 60 s · harmless, left
- `06-data-flow.md` `[security]` · forged Annoyed frame → one grumpy beep per minute is the most visible effect of risk (2), under ADR-0012 · follow-up
- traceability footer `[docs]` · long line · left

## ⚪ Disagree / Intentional
- Contagion inside `MoodEngine` (SRP) · owner decision D3 450/500 (social-group/05a)
- Tired excluded twice (tally and priority) · rows 9 and 10, defence in depth
- Booleans rather than a State pattern · two flags suffice

## Per-dimension summary
- Design: clean dependency direction via the `MoodTally` value · Docs: complete, consistent with the
  code · Tests: rows 1–12 mapped, mutation gaps closed · Security: bounds hold, one pull per minute
  whatever the frames · Correctness: matches 05a, tick order right
