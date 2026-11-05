# Sequence diagram — social-group — strokes on cube A make cube B jealous

> **Frame**: `sd — Jealousy`
> **Feature**: `social-group` — backlog item G6a in [docs/roadmap.md](../../../roadmap.md), issue #73
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
  participant MB as Mood engine → JealousyRepertoire (cube B, lib)
  participant Gate as Sound gate (cube B, lib)

  Owner->>GA: stroke on A
  GA-->>SA: Gesture::Stroke (A purrs, 05a; may conduct a choir, 05c)
  SA->>SA: sendPet(self): Awake, a present neighbour, ≥ T_pet_tx? (05e rows 19, 20)
  SA->>SB: PET {stroked palette index = A} (broadcast, once)
  Main->>SB: tick(self with inJealousReaction)
  SB->>SB: decode (allowlist), sender present, index matches its entry, self Awake?
  SB-->>Main: SocialOutput {jealous, jealousOf = entry age, crowd}
  Main->>MB: jealousy(jealousOf, repertoire), right after social.tick, before the choir voice
  MB->>MB: guards G? step 1 or 2? context → weights, no repeat, Prng draw (05e)
  alt a reaction is drawn
    MB-->>Main: JealousReaction (eyes overlay, optional sound; Sulk enters Annoyed)
    Main->>SB: refuseChoir(out) if step 2 (clears the returned choir cue, 05e row 22b)
    Main->>Gate: its sound at the gate's volume, if any
  else guards fail
    MB-->>Main: none
  end
  Note over SB: while a step-2 reaction is held, SelfState::inJealousReaction makes Choir return no voice (05c row 13)
  Note over MB: a stroke on B stops the reaction and turns it Happy (05a)
```

## Notes

- **New wiring**: `main.cpp` calls `social.sendPet(self)` on `Gesture::Stroke` next to
  `startChoir(self)`, calls `mood.jealousy(out.jealousOf, seeded->repertoire)` right after `social.tick(self)` and
  before the choir voice is played, calls `social.refuseChoir(out)` on a step-2 reaction before any choir voice is played, plays the returned eyes
  and sound, and fills `SelfState::inJealousReaction` from `mood.inJealousReaction()`. Both
  refusals are decided in `lib/` and tested natively.
- **Attention memory**: the mood engine already receives every touch-down (`touchDown()`),
  gesture and call (`stimulus()`); it keeps the time of the last one, excluding `Loud` and
  `NewcomerSeen`, for T_ignored and T_lonely; no new port. `Neighbour::presentSince` is the only new field.
- **Randomness**: `JealousyRepertoire` owns its `Prng(seed, 3)` in `Seeded`, built in `setup()`
  after the Wi-Fi driver is up (ADR-0013); `main.cpp` passes it to `mood.jealousy(out.jealousOf, seeded->repertoire)`, since
  `MoodEngine` is a static global built before `Seeded`.
- **Dependency direction** unchanged: the flag and context live in `SocialOutput` in `lib/`.
