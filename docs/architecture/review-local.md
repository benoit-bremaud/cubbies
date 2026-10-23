# Local review — cubbies (claude/g4-choir-43d2ed)

- Date: 2026-10-23
- Commit: 9a54637 (base `origin/main` 6a2807b)
- Scope: branch diff main...HEAD: G4 synchronised choir (issue #57) — `core/Choir.{h,cpp}`,
  `core/PresenceCodec.{h,cpp}` (CHOIR frame, shared header helpers), `core/SocialProtocol.{h,cpp}`,
  `core/SoundGate.{h,cpp}` (lead rest, three voices), `src/main.cpp`, native tests, social-group
  PDR docs (UC10, 02c, 05c, 06, traceability, verification plan), PROJECT_LOG
- Checks run: design (architecture-guardian) · docs (doc-auditor) · tests (test-auditor) ·
  security + correctness (general-purpose, read-only)
- Verdict: ship after fixes (applied in 9a54637) · Must 0 · Should 19 (fixed) · Nice 8 (4 fixed, 4 documented)

## Pre-merge gate (head SHA 9a54637)
| Step | Result |
|---|---|
| 1 Tests | ✅ 292 native tests · ✅ `pio run` xiao_c3 and c3_supermini · ✅ `sim/run.sh` exit 0 |
| 2 Coverage | new lib code: `Choir.cpp` 100 % lines, `SoundGate.cpp` 100 % lines (manual `g++ --coverage`); docs ≈ 100 % of the new public surface (doc-auditor estimate) |
| 3 SonarQube | ❌ FAILED on main outside the diff: one issue in `hardware/case/cubbies_case.py` (#53), new_coverage not imported (Community edition); owner waiver as for G2/G3 |
| 4 Secrets | ✅ gitleaks clean (origin/main..HEAD) · no tracked .env · diff read: nothing sensitive |
| 5 QA pass | firmware: both board images build; FS8 drift bench on two prototypes pending (verification plan) |
- Gate: **GREEN with the #53 waiver**

## 🔴 Must Have
_None._

## 🟡 Should Have (all fixed in 9a54637)
- `core/Choir.cpp` `[correctness|security]` · a CHOIR frame received before the last start and drained a tick later passed the cooldown (`ageMs` wrapped) · cooldown judged in `tick()` on the tick's time; regression test `test_row7_backlogged_frame_meets_the_cooldown`
- `core/SoundGate.cpp` `[design|correctness]` · `buffer_` rewritten while the buzzer timer may read it · `audio_.stop()` first when the previous play used the buffer; tested
- `core/PresenceCodec.cpp` `[design]` · header write/check copied three times · `writeHeader` / `checkHeader`
- `core/SoundGate.cpp`, `src/main.cpp` `[design]` · voice count and lead bound linked by comments only · `kChoirVoices`, `kChoirLeadUnitMs`, `kChoirMelody`, two `static_assert`s
- `core/Choir.h` `[design|docs]` · row 6 relies on cooldown > rest + voice · `static_assert` and 05c note
- `test_choir.cpp` `[tests]` · frame content not checked on the air; row 9 vacuous; protocol rank, not-Awake wiring, pending guard, wrap on row 8 untested · tests added
- `test_soundgate.cpp` `[tests]` · one test for three behaviours; capped-rest duration and stale rest untested · split and added
- `02c`, `05c`, traceability `[docs]` · `ChoirFrame` name, voice names, cooldown anchor, implicit Singing, row ranges · aligned

## 🔵 Nice to Have
- `SoundGate.h` `[design]` · forwarding overload → default argument (fixed)
- `SocialProtocol.cpp` `[design]` · `awake` computed once (fixed)
- `SoundGate.h` `[docs]` · `kMaxNotes` comment (fixed)
- `test_choir.cpp` `[tests]` · unused `announce` parameter, rank 255 (fixed)
- `SocialProtocol.cpp` `[design]` · frame-type dispatch by fall-through; switch on the type byte before a fourth type (open)
- `src/main.cpp` `[correctness]` · voice dropped by a rank-3 sound still starts the cooldown (documented in 05c, accepted)
- `Choir.cpp` `[correctness]` · rest measured at the tick start, a few ms of drift from same-tick sends (documented, bench)
- `test_codec.cpp` `[tests]` · truncated CHOIR frame case (open, same shape as CHAIN)

## ⚪ Disagree / Intentional
- `core/Choir.cpp` `[security]` · a lone cube sings on a forged CHOIR frame · at most once per 60 s at the constant volume, ADR-0012 nuisance risk; 05c note
- `core/SocialProtocol.cpp` `[design]` · `lowerMacs()` next to `rank(sender)` · two different rules (05b vs 05c), Rule of Three not reached
- `src/main.cpp` `[tests]` · wiring untested natively · ADR-0014 adapter policy

## Per-dimension summary
- Design: dependency rule and single Sound gate held; codec duplication and magic links fixed · Docs: aligned, no noise · Tests: every 05c row covered, false-confidence row 9 fixed · Security: one cooldown bypass fixed, forged-frame nuisance bounded · Correctness: no remaining defect found
