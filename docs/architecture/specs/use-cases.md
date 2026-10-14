# Use cases — cubbies V1 (textual specifications)

> **Method**: Cockburn textual use cases. Each use case is an actor-initiated goal with a main
> success scenario and extensions. The use-case diagram in
> [diagrams/cube-core/01-use-case.md](../diagrams/cube-core/01-use-case.md) is the visual index.
> **Actors**: `Owner` (the colleague who adopted the cube), `Maker` (who builds and flashes),
> `Neighbour cube` (another cube in range, an external system), `Team` (several owners together).
> **Status**: draft; each UC cites the requirements it realises.

## Domain: Identity

### UC1 — Adopt a cube
- Primary actor: Owner. Requirements: REQ-F-07, REQ-F-08.
- Preconditions: the cube is powered and not yet adopted (or the secret gesture was performed).
- Minimal guarantee: the cube always has a colour (MAC-derived).
- Success guarantee: the chosen colour is stored and shown.
- Main success scenario:
  1. The cube shows its default colour with a "waiting for adoption" expression.
  2. The owner taps; the eyes take the next colour of the palette.
  3. The owner repeats step 2 until the colour pleases them.
  4. The owner strokes the cube for two seconds; the cube confirms with a happy expression and a
     short melody, and stores the colour.
- Extensions:
  - 2a. No tap for 60 s: the cube keeps the current colour as provisional and continues living;
    the ritual resumes at the next tap until a confirmation happens.
  - 4a. Power lost before confirmation: on reboot the default colour returns (REQ-F-07).

### UC2 — Re-adopt a cube
- Primary actor: Owner or Maker. Requirements: REQ-F-07.
- Main success scenario: hold the touch pad while plugging the cube; the cube enters UC1.

## Domain: Mood

### UC3 — Pet the cube
- Primary actor: Owner. Requirements: REQ-F-03, REQ-NF-01.
- Main success scenario:
  1. The owner taps / double taps / strokes the cube.
  2. Within 150 ms the cube reacts (surprise / joy / purr with closed eyes) and its mood shifts
     toward happy.
- Extensions: 1a. The cube is asleep: it wakes first (UC5), then reacts.

### UC4 — Call the cube
- Primary actor: Owner. Requirements: REQ-F-09.
- Main success scenario: the owner claps or speaks loudly near the cube; the cube looks up,
  reacts according to its mood, and wakes if it was dozing.
- Extensions: 1a. Sustained office noise: ignored (threshold adapts to the ambient level).

### UC5 — Let the cube sleep and wake it
- Primary actor: Owner (by inaction, then by attention). Requirements: REQ-F-06, REQ-NF-08.
- Main success scenario: no interaction for 10 min, the cube dozes (half-closed eyes, dimmed);
  after 30 min it sleeps (dream animation, screen dim); a touch or a sound wakes it with a yawn.

### UC6 — Adjust the volume (deferred: volume is a constant in V1)
- Primary actor: Owner. Requirements: REQ-F-05.
- Main success scenario (once a gesture is defined): the gesture cycles the four volume steps;
  the eyes show the level; the setting is stored.
- Open question: the gesture must not collide with UC1 and UC3 gestures; decided at the PDR of
  the feature that introduces it, not in V1.

## Domain: Social

### UC7 — Introduce a cube to the group
- Primary actor: Owner (carrying the cube into range). Requirements: REQ-F-10, REQ-NF-05.
- Main success scenario:
  1. The cube hears presence frames from neighbours and adds them to its table.
  2. Neighbours see the newcomer and greet (short look + beep); the newcomer greets back.
- Extensions: 1a. A neighbour falls silent for 30 s: it leaves the table without ceremony.

### UC8 — Let moods spread
- Primary actor: Team (by being together). Requirements: REQ-F-11.
- Main success scenario: each cube weighs its neighbours' moods into its own; a happy group makes
  a bored cube curious, a sulking cube dampens the group.

### UC9 — Start a chain reaction
- Primary actor: Owner (taps during a group moment). Requirements: REQ-F-12.
- Main success scenario: the tapped cube emits an event; each neighbour reacts once after a short
  delay proportional to its position in the table, then relays once; the wave dies out.
- Extensions: 2a. The same event id is heard again: ignored.

### UC10 — Gather the cubes for a choir (tier B)
- Primary actor: Team. Requirements: REQ-F-14.

## Not use cases (system-initiated behaviours, listed for traceability only)

- **B1 — Silence when nobody is there**: an outcome of UC5 (sleep) and, from S8, of darkness;
  no clock exists in V1 (C10), so there are no configured hours. Requirements: REQ-F-05, FS4.
- **B2 — Rare gag**: an extension of the mood engine, rate-limited on awake time. Requirement:
  REQ-F-13.
- **B3 — Motion reactions** (tier B, S9): tilt, shake, upside down. Requirement: REQ-F-15.

## Domain: Making

### UC11 — Build and flash a cube
- Primary actor: Maker. Requirements: REQ-NF-09, REQ-NF-10.
- Main success scenario: follow `docs/hardware/assembly.md`; flash the single image; the cube
  boots into UC1.
