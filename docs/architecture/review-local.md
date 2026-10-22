# Local review — cubbies (feat/55-g3-chain-reaction)

- Date: 2026-10-22
- Commit: branch head after the review fixes (base `origin/main` 9c29f79)
- Scope: branch diff main...HEAD: G3 chain reaction (issue #55) — `core/ChainReaction.{h,cpp}`,
  `core/PresenceCodec.{h,cpp}` (CHAIN frame), `core/SocialProtocol.{h,cpp}`,
  `core/StimulusRouter.{h,cpp}` (`routeChain`), `src/main.cpp`, native tests, social-group PDR docs
- Checks run: design (architecture-guardian) · tests (test-auditor) · security + correctness +
  docs (Opus reviewer)
- Verdict: ship after fixes (applied) · Must 0 · Should 13 · Nice 9

## Pre-merge gate
| Step | Result |
|---|---|
| 1 Tests | ✅ 262 native Unity tests (20 chain, 3 CHAIN codec, 1 router) · ✅ `pio run` xiao_c3 + c3_supermini · ✅ `sh run.sh` exit 0 |
| 2 Coverage | new lib code 100 % lines (gcov: `ChainReaction.cpp` 39/39, `encodeChain`/`decodeChain` 18/18, `startChain` 7/7, `rank` 5/5, `presentCount` 5/5, `routeChain` 3/3; `count()` drop branches covered by `test_native_social`) · docs: every new public symbol has a header comment |
| 3 SonarQube | ❌ FAILED on the project, not on this diff: `new_violations` 1 = python:S3776 in `hardware/case/cubbies_case.py:380`, `new_coverage` 0 % (no Python coverage.xml) — same as main, issue #53. No issue on any file of this diff |
| 4 Secrets | ✅ gitleaks clean · no tracked `.env*` · diff read: logic, tests, docs only; CHAIN frames carry no MAC |
| 5 QA pass | ✅ no UI: the product is the firmware — native suite incl. 3-cube loopback wave + both board images build; two-cube bench ripple optional (bench #25) |
- Gate: **RED on step 3 (pre-existing, outside the diff, #53)** — all other steps green

## 🔴 Must Have
_None._

## 🟡 Should Have (all fixed)
- `src/main.cpp` `[design]` · Awake check of 05b row 1 lived in the adapter · moved into `ChainReaction::start(awake)` / `SocialProtocol::startChain(SelfState)`, tested
- `src/main.cpp` `[design]` · chain Joy borrowed `route(DoubleTap)` · `routeChain()` added beside `routeNewcomer()`, router test
- `ChainReaction.cpp` tick `[design|tests]` · silent relay of a sleeping cube starts the cooldown, table said "reaction" · row 12 reworded "last chain fire", header comment
- `SocialProtocol.cpp` startChain `[design]` · Prng drawn even when the trigger is refused · id drawn inside `start()` after the checks, tested
- `test_chain.cpp` row 1 `[tests]` · origin cooldown not asserted · added
- `test_chain.cpp` row 13 `[tests]` · 8-key capacity not proven · oldest kept key asserted
- `test_chain.cpp` row 11 `[tests]` · relay not proven at protocol level, only Asleep · B's frame hop 1 asserted for Dozing, Asleep, Waking
- `test_chain.cpp` echo `[tests]` · total of 3 frames could hide 2+1+0 · one frame per cube asserted
- `test_chain.cpp` row 9 `[tests]` · relay not asserted · added
- `05b-state-chain.md` `[docs]` · Context said the chain reads the table and MAC · reworded "frame, rank, clock"
- `05b`/`06` `[security|docs]` · 8 forged keys can flush a real key from the ring · "one frame per event" qualified, cap stated
- `05b` `[security]` · forged hop-0 frame relayed by every cube · relay cap documented
- `ChainReaction.h` `[correctness]` · two time bases (frame receive time vs tick now) · documented: `isDue`/`ageMs` tolerate a receive time slightly ahead

## 🔵 Nice to Have
- `SocialProtocol.cpp` receive `[design]` · dispatch by `DecodeResult::Type` rather than the type byte · kept: two types only
- `PresenceCodec.h` `[design]` · rename to `FrameCodec` at the third frame type (Rule of Three)
- `SocialProtocol.h` `[design]` · `chain` flag name · kept, commented
- `05b` `[security]` · 1-in-256 event-id collision · documented
- `test_chain.cpp` `[tests]` · wrap tests for T_trigger / T_cooldown / isDue, rank with absent neighbours, hop 0/2 relays · not added (same `ageMs`/`isDue` helpers as row 14, already wrap-tested)

## ⚪ Disagree / Intentional
- `ChainReaction.cpp` hear `[design]` · a key arriving while an event is pending is not remembered · 05b row 10 (validated)
- `kMaxHop` in the codec `[design]` · the hop bound is part of the frame allowlist

## Per-dimension summary
- Design: no over-engineering, dependency rule kept · Docs: matched the code after rewording · Tests: one per 05b row, no wrong-reason pass · Security: forged-frame nuisance bounded by cooldowns under ADR-0012 · Correctness: all ages wrap-safe, no Must
