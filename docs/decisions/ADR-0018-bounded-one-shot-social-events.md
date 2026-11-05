# ADR-0018: Security — a received frame may trigger a bounded one-shot social event

**Status:** Proposed — 2026-11-05 — Amends ADR-0012 (the "nudge only" consequence; the accepted
residual risk and its rules stand)

## Context

ADR-0012 accepted the unauthenticated ESP-NOW broadcast in V1 and, in its consequences, asked
that G2 contagion "must not let a received mood byte do more than nudge the local mood engine,
so a forged frame cannot force a state". G2 (hold of 6 s on a plurality, calm, refractory) and
G5 (same hold, Bored only) satisfy it.

The G6a jealousy study (issue #73, `diagrams/social-group/05e-state-jealousy.md`, PR #76) needs
the opposite shape: a stroke is a moment, and a jealous reaction is only cute if it lands within
a tick of the stroke. Its PET frame `0x04` is sent once and, on the second PET within
`T_notice`, can put an idle awake cube into Annoyed. Holding the signal for seconds, or asking
for two frames in 3 s, kills the timing (scored 305/500 in the study, against 450 for this ADR).

The shape is not new. The CHAIN (G3) and CHOIR (G4) frames already make a cube react or sing on
a single well-formed frame, bounded by deduplication and cooldowns; they never touch the mood,
which is why they did not collide with the clause. The clause, read literally, forbids any
single-frame effect on the mood, which is stricter than the risk ADR-0012 actually accepted
("nothing worse than a wrong face or a beep can follow").

## Options and scoring

Criteria: respect of the accepted risk envelope (40), cute timing of social reactions (35),
simplicity of the rule for future features (25). Scores 0–5, totals /500.

| Criterion (weight) | A — Allow bounded one-shot events | B — Require a held or repeated signal | C — Argue each feature's equivalence in its own PDR |
|---|---|---|---|
| Risk envelope (40) | 5 — same effects as ADR-0012: RAM, eyes, one quiet sound, rate-limited | 4 — stricter than needed | 1 — the clause stays and is bent case by case |
| Cute timing (35) | 5 — reaction within one tick | 2 — seconds late, after the stroke ends | 5 |
| Simple rule (25) | 3 — one checklist every one-shot feature cites | 3 — one rule, but each feature fights it | 5 — nothing to write now |
| **Total /500** | **450** | **305** | **350** |

Sensitivity: with simplicity at 40 and timing at 20, A still leads (420 against 365 for C).

## Decision

**A single received frame may trigger a one-shot social event, including a mood change, when
every bound below holds.** The feature's state diagram lists each bound as a tested row.

1. **Allowlisted and versioned**: its own frame type, every field checked, reserved bytes zero,
   as ADR-0012 already requires.
2. **Only from a present neighbour** where the event names one (sender MAC in the table,
   `Waiting` or `Greeted`), so a forged MAC must first spend presence frames.
3. **Rate-limited per cube**: at most one effect per stated cooldown, never shorter than 60 s for
   an effect that changes the mood; the cooldown is the bound on forged traffic.
4. **Never while not Awake**: no event wakes a cube, and nothing received while dozing or
   asleep is replayed at the wake.
5. **Same effect envelope as ADR-0012**: RAM only, eyes, at most one short sound through the
   single Sound gate at its volume; nothing persisted, relayed beyond the feature's own bound, or
   logged with the MAC.
6. **Reversible by the owner**: any touch overrides the effect at once; a mood entered this way
   leaves by the mood engine's usual rules.
7. **No echo**: the feature states how its effect cannot ripple back through G2 contagion (a
   refractory, a guard on recently attended cubes, or both).

G2 and G5 keep their holds: contagion is a drift, not an event, and this ADR does not loosen it.

## Consequences

- G6a jealousy (PET `0x04`) is allowed as designed in PR #76: present sender, two frames within
  `T_notice`, one step-2 reaction per `T_jealous` (60 s), Awake only, touch forgives, sulk starts
  the G2 refractory and a just-attended cube is never pulled into Annoyed.
- CHAIN and CHOIR are recognised as one-shot events already within these bounds.
- Future one-shot features (G7 pass the ball, G10 flirting, G12 clap wave) cite this checklist
  in their PDR instead of reopening ADR-0012.
- The worst a forged frame can do stays what ADR-0012 tells the adoption-day briefing: a wrong
  face or a beep, now at most once per minute per event type.
- Fleet authentication remains the V2 ADR scoped by ADR-0012; this ADR does not change it.
