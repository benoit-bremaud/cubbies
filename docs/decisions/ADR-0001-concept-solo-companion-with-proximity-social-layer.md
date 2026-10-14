# ADR-0001: Concept — a complete solo companion with a proximity social layer

**Status:** Accepted — 2026-10-14

## Context

The owner wants a fleet of small cube robots (colour OLED eyes, microphone, buzzer) that connect
to each other and interact, to be offered to office colleagues on an "adoption day", one colour
per cube. Desk research shows the solo desk companion is well covered by open-source Dasai Mochi
clones (RoboEyes, OliEyes, several ESP32-C3 Super Mini builds); none of them has a multi-robot
layer. Constraints that follow from the gift context: a cube spends most of its time alone on a
desk; recipients are not makers (no app install, no flashing, no pairing); everything is
multiplied by the number of cubes.

## Options and scoring

Criteria from the project context: daily solo experience (3), zero friction for a colleague (3),
originality / adoption-day effect (3), KISS firmware (2), cost and assembly ×N (2), technical
risk (2).

| Criterion (weight) | A. Pure social swarm | B. Solo companion | C. Swarm + mobile app | D. Solo + proximity social layer |
|---|---|---|---|---|
| Daily solo experience (3) | 1 | 3 | 2 | 3 |
| Zero friction (3) | 2 | 3 | 1 | 3 |
| Originality (3) | 3 | 1 | 3 | 3 |
| KISS (2) | 2 | 3 | 1 | 2 |
| Cost ×N (2) | 2 | 3 | 2 | 2 |
| Risk (2) | 2 | 3 | 1 | 2 |
| **Total /45** | 30 | 37 | 27 | **40** |

## Decision

**Option D.** Each cube is a complete solo companion (eyes, moods, sleep, reactions to touch and
sound) and gains group behaviours when other cubes are in radio range, without pairing, app or
server. The solo layer is the daily use; the social layer is what makes the gift unique.

## Consequences

- Two behaviour layers to design: the solo mood engine and the social protocol; the social layer
  must degrade to nothing when a cube is alone.
- No mobile app and no server in V1; a phone-facing BLE service may come later.
- Roadmap by versions (ADR-0002) with a prioritised backlog from simplest to hardest
  (`docs/roadmap.md`), as the owner asked for crescendo iterations.
