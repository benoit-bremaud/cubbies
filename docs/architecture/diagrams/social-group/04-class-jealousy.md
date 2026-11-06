# Class diagram — social-group — jealousy repertoire types (G6a)

> **Frame**: `class — Jealousy`
> **Feature**: `social-group` — backlog item G6a in [docs/roadmap.md](../../../roadmap.md), issue #85
> **Related ADRs**: ADR-0013 (lib-owned `Prng`), ADR-0014 (tests follow the conception)
> **Rules**: [05e-state-jealousy.md](05e-state-jealousy.md) — **Sequence**: [02e-sequence-jealousy.md](02e-sequence-jealousy.md)
> **Requirement**: REQ-F-21

> **Status**: validated with the G6a PDR (issues #85, #88); implemented in issue #90.

## Context

This diagram answers "which types does G6a add, who owns what, and which way do the
dependencies point?". G6a adds three domain types (`PetContext`, `JealousReaction`,
`JealousyRepertoire`) and a contexts set, past the architecture README's threshold for a class
view.

## Diagram

```mermaid
classDiagram
  class PetContext {
    <<struct, core/Mood.h>>
    +uint8_t neighbours
    +uint32_t strokedAgeMs
  }
  class JealousContexts {
    <<value>>
    +bool bored
    +bool crowd
    +bool newcomer
    +bool friend
    +bool neglected
    +bool weary
  }
  class JealousReaction {
    <<enumeration>>
    None
    SideGlance
    Stare
    TurnAway
    Pout
    PuppyEyes
    MeToo
    FakeSleep
    Sulk
  }
  class JealousyRepertoire {
    -Prng prng_
    -JealousReaction lastStep1_
    -JealousReaction lastStep2_
    +weights(step, contexts, last) array
    +draw(step, contexts) JealousReaction
  }
  class MoodEngine {
    -uint32_t lastOwnerMs_
    -uint32_t noticeMs_
    -bool noticed_
    -ring step2Ms_[4]
    +jealousy(padHeld, PetContext, JealousyRepertoire&) JealousReaction
  }
  class SocialOutput {
    +bool pet
    +PetContext petContext
  }
  class Neighbour {
    +uint32_t presentSince
  }
  class Prng

  SocialOutput *-- PetContext
  JealousyRepertoire *-- Prng : owns stream 3
  MoodEngine ..> PetContext : reads
  MoodEngine ..> JealousContexts : computes
  MoodEngine ..> JealousyRepertoire : draws through (passed per call)
  JealousyRepertoire ..> JealousContexts
  JealousyRepertoire ..> JealousReaction : returns
  MoodEngine ..> JealousReaction : returns
  SocialOutput ..> Neighbour : petContext from presentSince
```

## Notes

- **Dependency direction**: `PetContext` lives in `core/Mood.h`, so `MoodEngine` never includes
  `SocialProtocol.h`; the Social protocol fills it from `Neighbour::presentSince` and its table.
- **Ownership**: `Seeded` (built after the Wi-Fi driver, ADR-0013) owns the repertoire; the mood
  engine, a static global built earlier, receives it per call and holds no reference.
- **Eyes**: each non-`None` value maps to a renderer `Reaction` of the same name through the lib
  `effects(JealousReaction) → Route` (05e Repertoire).
