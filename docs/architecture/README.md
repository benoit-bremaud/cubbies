# Architecture and UML study

UML-first: every behaviour of the backlog is modelled before it is coded, and the diagrams are
the contract the firmware must satisfy.

## Conventions

- Format: Mermaid, under `diagrams/<feature>/<NN>-<type>.md`.
- Diagram types, created only when their question applies: use-case, sequence, component, class,
  state, data-flow.
- Use cases are actor-initiated goals ("Adopt a cube", "Pet the cube", "Gather the cubes for a
  choir"), never system events. They are grouped by domain (Identity, Mood, Social, Settings,
  Maintenance), never by which module implements them.
- The component diagram is the only place where the split between behaviour logic
  (`firmware/lib/`) and adapters (`firmware/src/`) appears.

## First study (before any firmware)

Status: drafted, awaiting the owner's validation. Only the views whose question applies to V1 are
present; class diagrams are deferred until a feature introduces three or more domain types.

| Feature | Diagram | Question it answers |
|---------|---------|---------------------|
| `cube-core` | [01-use-case](diagrams/cube-core/01-use-case.md) | Who wants what, by domain (Identity, Mood, Social, Settings, Maintenance) |
| `cube-core` | [03-component](diagrams/cube-core/03-component.md) | Pure logic in `firmware/lib/` versus adapters in `firmware/src/`, and the ports between them |
| `cube-core` | [05a-state-mood-engine](diagrams/cube-core/05a-state-mood-engine.md) | The five moods and what moves them |
| `cube-core` | [05b-state-sleep-wake](diagrams/cube-core/05b-state-sleep-wake.md) | Awake, dozing, asleep, waking, and what wakes the cube |
| `identity` | [02a-sequence-first-boot](diagrams/identity/02a-sequence-first-boot.md) | How every boot ends with a colour (NVS, else MAC-derived) |
| `identity` | [02b-sequence-adoption-ritual](diagrams/identity/02b-sequence-adoption-ritual.md) | Tap cycles the palette, long stroke confirms, the held-at-boot gesture is the only entry |
| `social-presence` | [02-sequence-presence](diagrams/social-presence/02-sequence-presence.md) | Presence broadcast, neighbour table, greeting a newcomer |
| `social-presence` | [06-data-flow](diagrams/social-presence/06-data-flow.md) | The 8-byte versioned frame, allowlist parsing, where untrusted frames die |

Shared vocabulary across the diagrams: module names and port names come from the component
diagram, timer names (`T_*`) are tuning constants listed in each diagram's table, and the mood
byte on the air is the mood enum plus one "asleep" value.
