# Local review — cubbies (brick F2 behaviours)

- Date: 2026-10-15
- Commit: 14d4ccd
- Scope: branch diff `main...feat/f2-behaviours` (15 commits, firmware/lib, firmware/test, docs)
- Checks run: design · docs · tests · security · correctness
- Verdict: fix Must Have first · Must 3 · Should 8 · Nice 9

## 🔴 Must Have
- `firmware/lib/cubbies_core/core/SoundClassifier.cpp:57` `[correctness]` · `Loud` fires on a single loud tick once the sound is 500 ms old; 05d requires rms ≥ R_loud·ref sustained for T_loud. A voice then one loud tick annoys the cube and deep-wakes a sleeping one (S7). · Track `loudSinceMs_` (set on the first loud tick, cleared when not loud) and test its age; red test "voice then one loud tick".
- `docs/architecture/README.md:145` `[docs]` · 05e missing from the diagram index. · Add the 05e row.
- `docs/architecture/diagrams/cube-core/03-component.md:28,30,44,121,123,129,133` `[docs][design][correctness]` · Stale against the code: Gesture box (edges, hold → level, released), Router box and edges (also picks reactions and sounds, `Router → Eyes`, `Router → Gate` via `Route` applied by main.cpp), IAudio (note sequence + stop), `route(...)` signatures, touch-down does not reach the Mood engine, baseline frozen during a sound (not re-seeded). · Align the text and the diagram.

## 🟡 Should Have
- `firmware/lib/cubbies_core/core/StimulusRouter.cpp:76` `[design]` · Greeting in Bored/Tired not in 05a. · Owner decision 2026-10-15: greet in every Awake mood; amend the 05a NewcomerSeen row, add a router test note.
- `firmware/lib/cubbies_core/core/SoundGate.h:22` `[correctness][docs]` · If main.cpp syncs the sleep state only after `SleepCycle::tick()`, the wake-up melody (entered from `gesture()`/`sound()`) is rejected. · Document the contract: call after every SleepCycle call that returns an effect, before applying `effects()`; mention Dozing/Asleep stops the sound in progress.
- `firmware/lib/cubbies_core/core/SoundGate.cpp:30` `[design][security]` · `kSounds` indexed by `SoundId` without a size check. · `static_assert` one entry per `SoundId`.
- `firmware/test/test_native_{mood,sleep,soundgate}` `[tests]` · Boundary windows of 2–3 ticks let `>=` → `>` slips pass. · Step tick by tick and assert `T ≤ age < T + kTick`.
- `firmware/test/test_native_sound/test_sound.cpp` `[tests]` · No test at exact thresholds (2×, 4×, clap 120 ms) nor "5× too short → Voice only". · Add boundary cases.
- `firmware/test/test_native_soundgate/test_soundgate.cpp:382` `[tests]` · Replacement only counts plays. · Assert the new notes and that the new duration ends the sound.
- `firmware/test/test_native_sleep/test_sleep.cpp`, `test_native_mood/test_mood.cpp` `[tests]` · Missing: Waking state after Asleep + Loud, Waking → Awake from Asleep, Annoyed "unchanged" rows (NewcomerSeen, Voice, Clap). · Add the asserts and loops.
- `docs/architecture/diagrams/cube-core/05e-state-sound-gate.md`, `05a-state-mood-engine.md:77` `[docs]` · 05e lacks "Each row is one native test (ADR-0014)"; 05a line overlong. · Add and re-wrap.

## 🔵 Nice to Have
- `SleepCycle.h`, `SoundGate.h` `[design]` · Whole headers included for one enum. · Small enum headers like `core/Mood.h`.
- `StimulusRouter.h:24` `[design]` · Flag + value pairs with real defaults. · `std::optional`.
- `SoundClassifier.cpp:40` `[correctness]` · A burst whose peak drops after T_clap is dropped without becoming a Sound. · Check age first.
- `test_soundgate.cpp:438` `[tests]` · Compile-time fact tested at run time. · `static_assert`.
- `test_mood.cpp:150` `[tests]` · Yawns counted, not spaced. · Record timestamps.
- `test_sleep.cpp` `toTired()` `[tests]` · `sleep.tick()` without clock advance. · Comment or drive through `run`.
- Router `stopReaction` on a Released with nothing held `[correctness]` · Check the renderer ignores it while Wake/Yawn runs. · At F5 wiring.
- Self-hearing `[correctness]` · The buzzer may be classified as Voice. · Bench check (F5).
- `SoundGate.h:26` `[docs]` · `rank()` uncommented. · One-line pointer to 05e.

## ⚪ Disagree / Intentional
- `SleepCycle.cpp` Awake stimulus writes `[design]` · Flagged as dead writes; intentional: 03 idle-timer rule recorded in 05b, guards future stimuli that do not move the mood. · 05b owner-decision note.
- `SleepCycle` holds a concrete `MoodEngine&`; router as pure functions `[design]` · Match 03. · 03-component.
- 05b mood-byte table untested `[tests]` · No code yet; belongs to the radio brick. · 05b, ADR-0013.

## Per-dimension summary
- Design: clean dependency rule, no over-engineering; diagram drifts. · Docs: code headers good, dossier stale (03, index). · Tests: every row covered, boundaries loose. · Security: none beyond the table size guard. · Correctness: one real bug (Loud not sustained), one wiring contract.
