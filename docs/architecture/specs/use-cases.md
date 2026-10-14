# Use cases — cubbies V1 (textual specifications)

> **Method**: Cockburn textual use cases. Each use case is an actor-initiated goal with a main
> success scenario and extensions. The use-case diagram in
> [diagrams/cube-core/01-use-case.md](../diagrams/cube-core/01-use-case.md) is the visual index.
> **Actors**: `Owner` (the colleague who adopted the cube), `Maker` (who builds and flashes),
> `Neighbour cube` (another cube in range, an external system), `Team` (several owners together).
> **Status**: draft; each UC cites the requirements it realises.

## Domain: Identity

### UC1 — Adopt or re-adopt a cube
- Primary actor: Owner (Maker during pre-adoption tests). Requirements: REQ-F-07, REQ-F-08.
- Preconditions: the cube was just plugged in; every boot resolves a colour first (stored, else
  MAC-derived) and then shows an invitation for 4 s (question mark and an empty gauge). First
  adoption and re-adoption are the same scenario (ADR-0013).
- Minimal guarantee: the cube always has a colour (MAC-derived); an abandoned ritual changes
  nothing.
- Success guarantee: the chosen colour is stored and shown.
- Main success scenario:
  1. During the invitation, the owner holds the pad; the gauge fills in 2 s and the ritual starts
     with a chime, eyes wide open in the candidate colour. Releasing that entry hold does nothing:
     the first ritual gesture is the next touch.
  2. The owner taps; the eyes take the next colour of the palette, with a tick whose pitch follows
     the index.
  3. The owner repeats step 2 until the colour pleases them.
  4. The owner strokes the cube for 1.5 s; the cube confirms with an adoption animation and a
     short melody, and stores the colour.
  5. (V1.x, UC6) A second step offers the volume the same way; V1 skips it.
- Extensions:
  - 1a. No hold during the invitation: the cube starts normally with its current colour.
  - 2a. No gesture for 60 s: ritual abandoned, previous colour kept, nothing written.
  - 4a. NVS write fails: error blink, identity unchanged (fail closed).
  - 4b. Power lost before confirmation: on reboot the previous colour returns.
- Note: the invitation replaces "hold while plugging in" because the TTP223 calibrates during
  its first 0.5 s and a finger present at power-up may never be detected (ADR-0013).

### UC2 — retired
- Merged into UC1 (re-adoption uses the same invitation and steps). The number is not reused.

## Domain: Mood

### UC3 — Pet the cube
- Primary actor: Owner. Requirements: REQ-F-03, REQ-NF-01.
- Main success scenario:
  1. The owner touches the cube; within 150 ms the eyes widen (touch-down reaction, the same in
     every mood).
  2. Once the gesture is classified (tap, double tap, stroke; a tap needs the 250 ms double-tap
     window to elapse) the cube reacts specifically (surprise / joy / purr with closed eyes) and
     its mood shifts toward happy.
- Extensions: 1a. The cube is asleep: it wakes first (UC5), then reacts.

### UC4 — Call the cube
- Primary actor: Owner. Requirements: REQ-F-09.
- Main success scenario: the owner claps or speaks near the cube; the cube widens its eyes and
  looks up, a Bored or Tired cube returns to Curious, a dozing cube wakes. A call never annoys the
  cube: only a sustained loud level does.
- Extensions: 1a. Sustained office noise: ignored (the threshold adapts to the ambient level).

### UC5 — Let the cube sleep and wake it
- Primary actor: Owner (by inaction, then by attention). Requirements: REQ-F-06, REQ-F-05, REQ-NF-08.
- Main success scenario: no interaction for 10 min, the cube dozes (half-closed eyes, dimmed);
  after 30 min it sleeps (dream animation, screen dim); a touch or a sound wakes it with a yawn.

### UC6 — Adjust the volume (V1.x: second step of the adoption ritual; V1 ships a constant)
- Primary actor: Owner. Requirements: REQ-F-05. Decision: ADR-0013.
- Main success scenario: after confirming the colour in UC1, the cube plays a sample beep at the
  current level; a tap cycles the four levels (each with its sample beep), a long stroke confirms
  and stores the level, read by the Sound gate from then on.
- Extensions: 1a. No gesture for 60 s: the previous level is kept.

## Domain: Social

### UC7 — Introduce a cube to the group
- Primary actor: Owner (carrying the cube into range). Requirements: REQ-F-10, REQ-NF-05.
- Main success scenario:
  1. The cube hears presence frames from neighbours and adds them to its table.
  2. Neighbours see the newcomer and greet (short look + beep); the newcomer greets back.
- Extensions: 1a. A neighbour falls silent for T_ttl (15 s): it is marked absent without ceremony.

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
  boots, shows the invitation, and the end-of-line check adopts it once (UC1) to prove the pad.
