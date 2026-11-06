# Sequence diagram — social-group — a stroke on cube A makes cube B react

> **Frame**: `sd — Jealousy`
> **Feature**: `social-group` — backlog items G6 (issue #73) and G6a (issue #85) in
> [docs/roadmap.md](../../../roadmap.md)
> **Related ADRs**: ADR-0005 (versioned frames), ADR-0012, ADR-0013 (single Sound gate,
> lib-owned `Prng`)
> **Behaviour**: B5 Jealousy in [specs/use-cases.md](../../specs/use-cases.md)
> **Rules**: [05e-state-jealousy.md](05e-state-jealousy.md) — **Base**: [02c-sequence-choir.md](02c-sequence-choir.md)

> **Status**: G6 design choices validated by the owner 2026-11-05 (issue #73); G6a additions
> (context, repertoire draw) proposed for issue #85, see 05e.

## Context

This diagram answers "which modules carry a stroke on cube A to a reaction on cube B?". It
reuses the G4 stroke path: the PET frame is sent only when the stroke did not start a choir. G6a
adds the context the Social protocol already knows and the draw in the `JealousyRepertoire`.

## Diagram

```mermaid
sequenceDiagram
  participant MA as main.cpp (cube A)
  participant SA as Social protocol (cube A, lib)
  participant SB as Social protocol (cube B, lib)
  participant MainB as main.cpp tick (cube B)
  participant MB as Mood engine (cube B, lib)
  participant Rep as JealousyRepertoire (cube B, lib)
  participant Gate as Sound gate (cube B, lib)

  MA->>SA: startChoir(self) on Gesture::Stroke
  alt choir started (05c)
    SA-->>MA: true (no PET, 05e S1)
  else choir cooling down
    SA-->>MA: false
    MA->>SA: sendPet(self)
    SA->>SB: PET {A's palette} (05e S2, never relayed)
  end
  MainB->>SB: tick(self)
  SB-->>MainB: SocialOutput {pet, petContext {neighbours, strokedAgeMs}} (05e input, row 11)
  MainB->>MB: jealousy(padHeld, petContext, repertoire)
  MB->>MB: guards G, crescendo step, cooldown, budget (05e rows 1–12)
  alt a step is due
    MB->>Rep: draw(step, contexts)
    Rep-->>MB: reaction (weights, no repeat, Weary; 05e rows 14–19)
    MB-->>MainB: JealousReaction (mood unchanged)
    MainB->>Gate: reaction eyes, and its sound at the gate's volume when step 2
  else no step
    MB-->>MainB: JealousReaction::None (no draw)
  end
```

## Notes

- **Wiring**: `main.cpp` calls `sendPet()` after a `startChoir()` that sent nothing, and
  `mood.jealousy(gestures.holdMs() > 0, out.petContext, seeded->repertoire)` when the tick's
  `SocialOutput::pet` is set; it maps the returned `JealousReaction` to its eyes and sound.
- **Dependency direction** unchanged: the Social protocol and the Mood engine stay unaware of
  each other; `main.cpp` passes the flag and the context. The repertoire depends only on `Prng`.
