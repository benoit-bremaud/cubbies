# Sequence diagram — social-group — strokes on cube A make cube B jealous

> **Frame**: `sd — Jealousy`
> **Feature**: `social-group` — backlog item G6 in [docs/roadmap.md](../../../roadmap.md), issue #73
> **Related ADRs**: ADR-0005 (versioned frames), ADR-0012 (superseded in part by ADR-0018,
> separate PR), ADR-0013 (single Sound gate)
> **Behaviour**: B5 Jealousy in [specs/use-cases.md](../../specs/use-cases.md)
> **Rules**: [05e-state-jealousy.md](05e-state-jealousy.md) — **Frame**: PET `0x04` in
> [social-presence/06](../social-presence/06-data-flow.md)

> **Status**: design choices proposed 2026-11-05 (issue #73), see 05e.

## Context

This diagram answers "which modules carry strokes on cube A to a glance, then a sulk, on cube
B?". It adds one frame type, one output flag and one engine call; the reactions reuse the
glance overlay and the Annoyed mood with its grumpy beep.

## Diagram

```mermaid
sequenceDiagram
  actor Owner
  participant GA as Gesture classifier (cube A, lib)
  participant SA as Social protocol (cube A, lib)
  participant SB as Social protocol (cube B, lib)
  participant Main as main.cpp tick (cube B)
  participant MB as Mood engine (cube B, lib)
  participant Gate as Sound gate (cube B, lib)

  Owner->>GA: stroke on A
  GA-->>SA: Gesture::Stroke (A purrs, 05a; may conduct a choir, 05c)
  SA->>SA: sendPet(self): Awake, a present neighbour, ≥ T_pet_tx? (05e rows 14, 14b)
  SA->>SB: PET {stroked palette index = A} (broadcast, once)
  Main->>SB: tick(self)
  SB->>SB: decode (allowlist), sender present, index matches its entry, self Awake?
  SB-->>Main: SocialOutput {jealous = true}
  Main->>MB: jealousy()
  alt first step (05e row 2)
    MB-->>Main: MoodEffect::SideGlance (mood unchanged, Noticed)
  else second PET within T_notice (05e row 2b)
    MB-->>Main: MoodEffect::Grumpy (mood now Annoyed, timers from now, G2 refractory)
    Main->>Gate: grumpy beep at the gate's volume
    Note over Main: a CHOIR cue on this tick or during the sulk is not sung (05e row 16b)
  else guards fail
    MB-->>Main: MoodEffect::None
  end
  Note over MB: a later stroke on B turns it Happy (05a), calm T_calm turns it Curious
```

## Notes

- **New wiring**: `main.cpp` calls `social.sendPet(self)` on `Gesture::Stroke` next to
  `startChoir(self)`, and calls `mood.jealousy()` (clock injected, like every engine call) when
  `SocialOutput::jealous` is set, after the gestures and before `contagion()` (05e row 15). It
  skips the choir voice while `mood.sulking()` is true.
- **Attention memory**: the mood engine already receives every touch-down (`touchDown()`),
  gesture and call (`stimulus()`); it keeps the time of the last one for T_ignored, no new port.
- **Dependency direction** unchanged: the flag lives in `SocialOutput` in `lib/`.
