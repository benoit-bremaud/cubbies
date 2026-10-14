# Architecture and UML study

UML-first: every behaviour of the backlog is modelled before it is coded, and the diagrams are
the contract the firmware must satisfy.

## Conventions

- Format: Mermaid, under `diagrams/<feature>/<NN>-<type>.md`.
- Diagram types, created only when their question applies: use-case, sequence, component, class,
  state, data-flow.
- Use cases are actor-initiated goals ("Adopt a cube", "Pet the cube", "Gather the cubes for a
  choir"), never system events. They are grouped by domain (Identity, Mood, Social, Settings),
  never by which module implements them.
- The component diagram is the only place where the split between behaviour logic
  (`firmware/lib/`) and adapters (`firmware/src/`) appears.

## Planned first study (before any firmware)

| Feature | Diagrams |
|---------|----------|
| `cube-core` | use-case (all domains), component (logic vs adapters, ports), state (mood engine, sleep) |
| `identity` | sequence (first boot, adoption ritual, reset) |
| `social-presence` | sequence (broadcast, neighbour table, greeting), data-flow (frame format) |
