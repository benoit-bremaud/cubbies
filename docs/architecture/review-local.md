# Local review — cubbies (brick F3 identity and boot)

- Date: 2026-10-15
- Commit: see `git log` of `feat/f3-identity` (review of the 13 commits on `origin/main...HEAD`)
- Scope: branch diff `origin/main...feat/f3-identity` (firmware/lib, firmware/test, docs)
- Checks run: design · docs · tests · security · correctness
- Verdict: fix Must Have first · Must 4 · Should 6 · Nice 7

## 🔴 Must Have
- `docs/architecture/diagrams/identity/02b-sequence-adoption-ritual.md:44` `[docs]` · "pitch follows the index" contradicts the fixed-tick decision · fix the arrow to `play(RitualTick)`.
- `02b-sequence-adoption-ritual.md:40` `[docs]` · double tap +2 missing from the loop · add an `opt double tap`.
- `02b-sequence-adoption-ritual.md:85` `[docs]` · "8–12 entries" stale · 8 entries.
- `firmware/test/test_native_ritual/test_ritual.cpp` `[tests]` · timeout test does not pin the 60 s length nor the restart by a tap · assert not done at T − tick after a tap, done with Clear at T.

## 🟡 Should Have
- `firmware/lib/cubbies_core/core/InvitationWindow.cpp:43` `[correctness][design]` · a press whose edge falls on the last tick before T_invite is declined (`holdMs()` is 0 on the edge tick and expiry is checked first); a delayed tick can enter Holding with hold ≥ T_secret and wrap the gauge · check the press before the expiry in Inviting and clamp the progress; new 05a row and tests.
- `02b-sequence-adoption-ritual.md:82` `[docs]` · "restarts at every tap" → every gesture; chime at start and `clearIdentityFrame` on timeout missing from the sequence.
- `docs/architecture/diagrams/cube-core/05e-state-sound-gate.md:26` `[docs]` · rank list omits the ritual sounds.
- `firmware/test/test_native_invitation/test_invitation.cpp` `[tests]` · exact T_secret and T_invite edges not pinned.
- `firmware/test/test_native_gesture/test_gesture.cpp` `[tests]` · 05c "any → Consumed" not tested from Pressed2.
- `test_invitation.cpp` `[tests]` · silent-release check cannot fail; `GREATER_THAN(0)` instead of the exact progress.

## 🔵 Nice to Have
- `InvitationWindow.cpp:39` `[correctness]` · consume at the settle end whenever the classifier is not Idle (double press inside 250 ms during calibration).
- `GestureClassifier.h:24`, `Identity.h` `[docs][design]` · state the preconditions of `consumeCurrentPress()` and `paletteColour()` (or assert).
- `AdoptionRitual.cpp:46` `[design]` · separate Tap and DoubleTap branches to mirror the 05b rows.
- `IdentityOutput.h:7` `[design]` · `SoundId` in its own header (Rule of Three not reached).
- `test_ritual.cpp`, `test_invitation.cpp` `[tests]` · stroke as first gesture, re-adoption timeout keeps adopted, Entered returns Keep, re-press after a consumed press, release at exactly T_invite.

## ⚪ Disagree / Intentional
- `IdentityOutput` next to the router's `Route` · PDR F3 decision (445/500).
- `InvitationWindow` ticks the Gesture classifier itself · 05a, the pad is sampled in one place.
- `Consumed` state in the classifier · amended 05c, validated.

## Per-dimension summary
- Design: no SOLID breach or over-engineering; one gauge clamp. · Docs: code headers fine; 02b not updated with the F3 decisions. · Tests: every 05a/05b row has a test; edges and timeout length weak. · Security: NVS value validated, fail-closed write, no secrets; key at the 15-char NVS limit. · Correctness: one edge-tick press lost at the window end.
