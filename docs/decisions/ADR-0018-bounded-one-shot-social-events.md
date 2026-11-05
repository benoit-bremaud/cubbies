# ADR-0018: Security — a received frame may change the mood through a bounded one-shot social event

**Status:** Proposed — 2026-11-05 — Amends ADR-0012 (the "nudge only" consequence only; the
accepted residual risk and its four rules stand; the G3 CHAIN relay already departs from
ADR-0012's "nothing received is relayed" rule, a known exception tracked in issue #78 and not
widened here)

## Context

ADR-0012 accepted the unauthenticated ESP-NOW broadcast in V1 and, in its consequences, asked
that G2 contagion "must not let a received mood byte do more than nudge the local mood engine,
so a forged frame cannot force a state". ADR-0012 names that risk "mood injection". G2 (a 6 s
hold on a plurality, calm, refractory) and G5 (the same hold, Bored only) satisfy it.

The G6a jealousy study (issue #73, `diagrams/social-group/05e-state-jealousy.md`, PR #76) needs a
different shape. A stroke is a moment, and a jealous reaction is only cute if it lands within a
tick of the stroke. G6a already asks for two separate strokes on the neighbour within `T_notice`
(two PET frames) before a reaction can change the mood. The losing option would also hold or
repeat the signal of **each** stroke (for example two frames 3 s apart per stroke), which pushes
every reaction seconds past the stroke.

Accepting this widens ADR-0012's envelope: a forged frame could then do more than set a mood
byte in the neighbour table (ADR-0012's list of reachable effects). It could put the cube's own
mood into Annoyed for `T_calm`. The bounds below are what makes that acceptable.

## Options and scoring

Criteria: respect of the accepted risk envelope (40), cute timing of social reactions (35),
simplicity of the rule for future features (25). Scores 0–5, totals /500.

| Criterion (weight) | A — Allow bounded one-shot mood events | B — Hold or repeat each stroke's signal | C — Argue each feature's equivalence in its own PDR |
|---|---|---|---|
| Risk envelope (40) | 4 — widens "nudge only" to one forced mood per minute, reversible by a stroke | 4 — keeps the nudge shape, but a held forged signal still forces it | 1 — the clause stays and is bent case by case, with no common bound |
| Cute timing (35) | 5 — reaction within one tick | 2 — seconds late, after the stroke ends | 5 — each feature picks its own timing |
| Simple rule (25) | 3 — one checklist every mood-changing feature cites | 3 — one rule, but each feature fights it | 5 — nothing to write now |
| **Total /500** | **410** | **305** | **340** |

Sensitivity: with risk 40, timing 20 and simplicity 40, A still leads (380, against 340 for C and
320 for B).

## Decision

**A single received frame may change the cube's own mood through a one-shot social event when
every bound below holds.** The feature's state diagram lists each bound as a tested row.

1. **Allowlisted and versioned**: its own frame type, every field checked, reserved bytes zero,
   as ADR-0012 already requires.
2. **Only from a present neighbour** (sender MAC in the table, `Waiting` or `Greeted`, and the
   named palette index matching that entry). This raises the cost of a fresh forged MAC;
   impersonating a cube that is already present passes it, which bound 3 caps.
3. **Rate-limited per cube, across features**: at most one radio-triggered mood change per cube
   per 60 s, shared by every feature governed by this ADR (a feature may add a longer cooldown of
   its own). The shared budget is the bound on forged traffic.
4. **Never while not Awake**: no event wakes a cube, and nothing received while dozing or
   asleep is replayed at the wake.
5. **ADR-0012 effect envelope otherwise**: RAM only, eyes, at most one short sound through the
   single Sound gate at its volume; nothing persisted, relayed or logged with the MAC, as
   ADR-0012's "RAM only, eyes and beep only" rule states.
6. **Reversible by the owner**: a stroke overrides the effect at once (an overlay stops; the
   mood leaves by the mood engine's rules, which include the stroke).
7. **No echo**: the feature states how its mood change cannot ripple back through G2 contagion
   (a refractory, a guard on recently attended cubes, or both).

G2 and G5 keep their holds: contagion is a drift, not an event, and this ADR does not loosen it.
One-shot events that **do not change the mood** (CHAIN, CHOIR: reactions and songs only) are not
governed by this ADR; they stay under ADR-0012's nuisance acceptance as designed.

## Consequences

- G6a jealousy (PET `0x04`) is allowed as designed in PR #76: present sender with a matching
  index, two frames within `T_notice`, one step-2 reaction per `T_jealous` (60 s), Awake only, a
  stroke forgives, the sulk starts the G2 refractory, and a just-attended cube is never pulled
  into Annoyed.
- Future mood-changing one-shot features (for example G10 flirting) cite this checklist in their
  PDR instead of reopening ADR-0012.
- The adoption-day briefing adds one line to ADR-0012's: a cube in range can be imitated, and
  the worst that follows is a wrong face or a beep, or one radio-triggered mood change per minute
  whatever the feature, undone by a stroke. Other effects keep their own cooldowns (for example CHAIN 5 s,
  greetings 10 s).
- Fleet authentication remains the V2 ADR scoped by ADR-0012; this ADR does not change it.
