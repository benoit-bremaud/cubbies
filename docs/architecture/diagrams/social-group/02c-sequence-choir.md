# Sequence diagram — social-group — a stroke starts a synchronised choir

> **Frame**: `sd — Choir`
> **Feature**: `social-group` — backlog item G4 in [docs/roadmap.md](../../../roadmap.md), issue #57
> **Related ADRs**: ADR-0005 (versioned frames), ADR-0012, ADR-0013 (single Sound gate)
> **Use case**: UC10 Gather the cubes for a choir in [specs/use-cases.md](../../specs/use-cases.md)
> **Rules**: [05c-state-choir.md](05c-state-choir.md) — **Frame**: CHOIR in [social-presence/06](../social-presence/06-data-flow.md)

> **Status**: design choices validated by the owner 2026-10-23 (issue #57), see 05c.

## Context

This diagram answers "how does one stroke become one melody, started on the same millisecond by
every cube in range?". The Social protocol owns the radio and the table; its `Choir` member
decides; `main.cpp` hands the voice and the lead rest to the Sound gate; the buzzer adapter's
timer plays the rest then the notes.

## Diagram

```mermaid
sequenceDiagram
  actor Owner
  participant Main as main.cpp (cube A)
  participant SA as Social protocol + Choir (A, lib, rank 1)
  participant GA as Sound gate (A)
  participant SB as Social protocol + Choir (B, lib, rank 0)
  participant GB as Sound gate (B)

  Owner->>Main: stroke
  Main->>Main: route(Stroke) → Purr eyes
  Main->>SA: startChoir()
  SA->>SA: present neighbour? cooldown over? (rows 1–3)
  SA-)SB: CHOIR {palette A, melody 0, lead 50}
  SA-->>Main: SocialOutput.choir {voice 1, rest 500 ms}
  Main->>GA: play(ChoirVoice1, 500)
  Note over SB: receive callback stamps rxTime (≈ A's send time + 1–5 ms)
  SB->>SB: Awake, cooldown over, rxTime + 500 still ahead (row 4)
  SB-->>SB: SocialOutput.choir {voice 0, rest = rxTime + 500 − now}
  SB->>GB: play(ChoirVoice0, rest)
  Note over GA,GB: both buzzer timers end their rest on the same ms (drift = radio latency)
  GA->>GA: melody voice
  GB->>GB: low voice
```

## Notes

- **Dependency direction**: `Choir` knows `IClock` time values and the decoded `ChoirFrame`; it
  never sees the radio or the audio port.
- **Order in the tick**: `startChoir()` is called after the gesture of the tick, before
  `social.tick()`, as `startChain()` (02b); the output is applied after the gesture's route, so
  the choir voice replaces the Purr sound (05c note).
