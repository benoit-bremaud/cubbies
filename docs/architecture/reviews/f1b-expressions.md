# Local review — cubbies (branch diff)

- Date: 2026-10-15
- Commit: feat/f1b-expressions (findings applied in the third commit of the branch)
- Scope: branch diff `main...HEAD` on `feat/f1b-expressions` (brick F1b: `Timeline`,
  `Reaction`, reaction overlay and identity frames in `EyesRenderer`, 28 golden states, sheet
  script, traceability rows, 02a/02b diagram names, PROJECT_LOG)
- Checks run: design (architecture-guardian) · docs (doc-auditor) · tests and correctness
  (test-auditor + general review)
- Verdict: fix Must Have first, then ship · Must 6 · Should 12 · Nice 5 — **all Must and
  Should applied before push**; 73 native tests green, both board envs build

## 🔴 Must Have

- `EyesRenderer.cpp` identity frames `[design|correctness]` · Drawn at fixed coordinates: no
  burn-in drift during a held invitation or ritual (REQ-NF-08 says every state). · Applied:
  `drawIdentityFrame` takes the drift; borders and the gauge stay on the edge; test
  `identity_frames_drift_against_burn_in`.
- `EyesRenderer.cpp` Purr `[design]` · Borrowed `lidMove_`, the sleep lid timeline: a purr during
  a lid move corrupted the sleep state, and stopping it snapped the eyes open. · Applied: ramp
  in, hold and release on `reactionTl_` with `purrReleasing_`; openness is
  `min(towards, moodOpenness)` so a purr never opens eyes the sleep cycle closed; tests for the
  gradual release and the closed-eyes cases.
- `02a`, `02b` diagrams `[docs]` · Called `playAdoptionAnimation`, `showErrorBlink`,
  `clearInvitation`, `showRitualFrame(paletteIndex)`; the code differs. · Applied: diagrams
  aligned on the code (the renderer takes a colour, the identity rules own the palette).
- `test_eyes.cpp` no-blink-during-reaction `[tests]` · Vacuous: the reaction ended before the
  seed's first blink. · Applied: the deadline is observed on a control rig, the reaction starts
  three ticks before it, 8 seeds; same approach for identity frames.
- `EyesRenderer.cpp` smile carve `[correctness]` · The edge depth jumped from 10 to 1 at 18 px
  of open height, so a closing happy eye briefly gained pixels (purr descent not monotonic). ·
  Applied: edge depth follows the centre depth; monotonic descent asserted.
- `EyesRenderer.h` header `[docs]` · "Reaction ends on its own" false for TouchDown and Purr;
  drift claim did not hold for identity frames. · Applied: layer rules, held vs timed, deferred
  blinks stated once in the header.

## 🟡 Should Have

- `Lids` bag of flags (`smile`, `arcWhenClosed`) `[design]` · Applied: `Arc` enum
  `{None, Smile, SmileClosed}`.
- Magic numbers 153, 192, 60, 6, 4, 8, 45 % `[design]` · Applied: named tuning constants in the
  anonymous namespace.
- `ritualStartMs_` next to `Timeline` `[design]` · Applied: `ritualPulse_` timeline restarted
  every `kRitualPulseMs`.
- Happy lids duplicated three times `[design]` · Applied: `lidsForMood(Mood::Happy)` reused by
  Joy and Adoption; `lidsForMood` is static.
- `reactionDurationMs` undocumented, `Reaction.h` without sources `[docs]` · Applied: 0 = held;
  per-reaction sources (05a, 05b, 02-presence, UC3/4/7).
- `Timeline` contract `[docs]` · Applied: zero duration, phase 255 = done, elapsed past the
  end, owners call `stop()`; 64-bit phase test for long durations.
- Duplicate static goldens (touchdown, surprise, lookup ×2) `[tests]` · Applied: one each,
  sheet list in sync.
- `wake-2` timing at 680 ms sat on the stretch boundary; `joy-1 == joy-2` risk `[tests]` ·
  Applied: peak and trough frames differ, wake stretch frame regenerated.
- Missing rule tests `[tests]` · Applied: restart on replay, every timed reaction lasts its
  duration, closeEyes during a yawn, purr on closed eyes, identity frames defer blinks,
  reaction does not leak into an identity frame, purr snap-open.
- Traceability rows REQ-F-01/03/07 `[docs]` · Applied.
- PROJECT_LOG F1b entry and PR #9 stamp `[docs]` · Applied.
- `golden_sheet.py` duplicate `cols` `[tests]` · Applied.

## 🔵 Nice to Have

- TouchDown widening vs the double-tap window (a tap shows TouchDown then Surprise) · Open:
  owner decision in F1c (the gesture classifier does not exist yet).
- Error frame "three flashes" vs "two alternating frames" wording · Applied in the header.
- `Shape` carries `winkLeft` only for adoption · Kept: one field, one consumer, no abstraction.
- `applyReaction` switch length · Kept: eight cases, each three lines; a table of lambdas would
  hide the shapes F1c will generalize.
- Golden README name-sync note · Already present.

## ⚪ Disagree / Intentional

- Reactions as an enum + switch rather than polymorphic objects `[design]` · F1c replaces the
  shapes with a parameter model; objects now would be thrown away.
- `reactionTl_` reused for the purr ramp and release `[design]` · One reaction at a time by
  contract; a second timeline would be a second source of truth for "is a reaction running".
- Exact golden comparison `[tests]` · Deterministic integer rendering.

## Per-dimension summary

- Design: three explicit layers; one timeline type for every animation; no port change; the
  renderer still knows nothing of sleep policy, gestures or sound.
- Docs: header is the single statement of the layer rules; diagrams and code agree on names;
  traceability and log updated.
- Tests: 73 native tests, 28 goldens; seeded tests derive deadlines from observation, not
  from luck; monotonicity and timing rules asserted.
- Security / correctness: no new inputs from outside the lib; 64-bit phase arithmetic;
  wrap-safe through `ageMs`/`isDue` only; both boards build with margin.
