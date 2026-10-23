# Sequence diagram — social-group — a neighbour's yawn makes my idle cube sleepy

> **Frame**: `sd — Yawn contagion`
> **Feature**: `social-group` — backlog item G5 in [docs/roadmap.md](../../../roadmap.md), issue #59
> **Related ADRs**: ADR-0005 (versioned PRESENCE frame), ADR-0012, ADR-0013 (single Sound gate)
> **Behaviour**: B4 Collective sleep in [specs/use-cases.md](../../specs/use-cases.md)
> **Rules**: [05d-state-yawn.md](05d-state-yawn.md) — **Base**: [02-sequence-contagion.md](02-sequence-contagion.md)

> **Status**: design choices validated by the owner 2026-10-23 (issue #59), see 05d.

## Context

This diagram answers "which modules carry a yawn from cube A to cube B?". It is the G2 path with
one more field: the tally now also counts drowsy neighbours. No radio traffic is added.

## Diagram

```mermaid
sequenceDiagram
  participant SA as Social protocol (cube A, lib)
  participant SB as Social protocol (cube B, lib)
  participant Main as main.cpp tick (cube B)
  participant MB as Mood engine (cube B, lib)
  participant Gate as Sound gate (cube B, lib)

  Note over SA: A is Tired (or dozing, or asleep)
  SA->>SB: PRESENCE {mood = 3 or 5} every T_presence (± jitter)
  loop every Awake tick (40 ms)
    Main->>SB: moodTally()
    SB-->>Main: votes + drowsy count
    Main->>MB: contagion(tally)
    MB->>MB: Bored? drowsy held and calm ≥ T_contagion? (05d rows)
    alt rule 4 holds
      MB-->>Main: MoodEffect::Yawn (mood now Tired)
      Main->>Gate: yawn sound at the gate's volume
    else
      MB-->>Main: G2 result, or MoodEffect::None
    end
  end
  Note over MB: B's next PRESENCE carries 3; the sleep cycle dozes B after T_doze
```

## Notes

- **No new wiring**: `main.cpp` already calls `mood.contagion(social.moodTally())` and applies
  its effect; the Yawn effect is already mapped to the yawn sound and eyes.
- **Dependency direction** unchanged: `MoodTally` is a value type in `core/Mood.h`.
