# Sequence diagram — social-group — a double tap ripples through the group

> **Frame**: `sd — Chain reaction`
> **Feature**: `social-group` — backlog item G3 in [docs/roadmap.md](../../../roadmap.md), issue #55
> **Related ADRs**: ADR-0005 (versioned frames), ADR-0012, ADR-0013
> **Use case**: UC9 Start a chain reaction in [specs/use-cases.md](../../specs/use-cases.md)
> **Rules**: [05b-state-chain.md](05b-state-chain.md) — **Frame**: CHAIN in [social-presence/06](../social-presence/06-data-flow.md)

> **Status**: proposed, awaiting owner validation (PDR `social-group`, G3, issue #55).

## Context

This diagram answers "which modules carry a chain event from the tapped cube to its neighbours,
and who decides when each one reacts?". The Social protocol owns the radio and the table; its
`ChainReaction` member decides; `main.cpp` turns the returned flag into eyes and sound.

## Diagram

```mermaid
sequenceDiagram
  actor Owner
  participant Main as main.cpp (cube A)
  participant SA as Social protocol + ChainReaction (A, lib)
  participant SB as Social protocol + ChainReaction (B, lib, rank 0)
  participant SC as Social protocol + ChainReaction (C, lib, rank 1)

  Owner->>Main: double tap
  Main->>Main: route(DoubleTap) → Joy (unchanged)
  Main->>SA: startChain()
  SA->>SA: present neighbour? cooldown over? event id = Prng, remember key (rows 1–3)
  SA-)SB: CHAIN {palette A, id, hop 0}
  SA-)SC: CHAIN {palette A, id, hop 0}
  SB->>SB: key unseen → delay 300 ms × (1 + 0) (row 4)
  SC->>SC: key unseen → delay 300 ms × (1 + 1) (row 4)
  Note over SB: +300 ms
  SB-->>SB: SocialOutput.chain = true → Joy eyes + Joy sound (row 5)
  SB-)SC: CHAIN {palette A, id, hop 1} (row 6)
  SB-)SA: CHAIN {palette A, id, hop 1}
  SC->>SC: same key → ignored (row 8)
  SA->>SA: own key → ignored (row 9)
  Note over SC: +600 ms
  SC-->>SC: SocialOutput.chain = true → Joy eyes + Joy sound
  SC-)SB: CHAIN {hop 1} → ignored by A and B (rows 8, 9)
```

## Notes

- **Dependency direction**: `ChainReaction` knows `Mac`, `IClock` time values and the decoded
  `Chain` struct; it never sees the radio. The Social protocol calls it from `receive()` and
  `tick()` and broadcasts what it returns.
- **Order in the tick**: `startChain()` is called after the gesture of the tick, before
  `social.tick()`, so the origin frame leaves on the same tick.
