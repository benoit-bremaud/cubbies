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
  Views: [diagrams/social-group/](../diagrams/social-group/) (PDR G2 validated 2026-10-22, issue #52).
- Preconditions: the cube is Awake and hears at least one present, awake neighbour (UC7).
- Minimal guarantee: alone, or with only sleeping or tired neighbours, the cube behaves as solo;
  the owner's attention always wins over the group.
- Success guarantee: the cube takes the mood most of its neighbours show, at most once a minute.
- Main success scenario:
  1. The owner pets cube A; A turns Happy and its presence frames carry the mood.
  2. Cube B sees Happy as its neighbours' most common mood for 6 s, without being touched or
     spoken to meanwhile.
  3. B turns Happy, with the usual eyes cross-fade; its own frames now carry Happy.
  4. Both cubes settle back on their own timers; neither is pulled again for a minute.
- Extensions:
  - 2a. The owner of B pets or calls it during the hold: the pull waits for 6 s of calm.
  - 2b. The most common mood changes during the hold: the hold restarts.
  - 2c. A tie: Happy beats Annoyed beats Curious beats Bored (a sulking cube dampens a calm group).
  - 2d. B was pulled less than a minute ago: nothing happens.
  - 3a. The pulled mood is Annoyed: B grumbles once (grumpy beep through the Sound gate).

### UC9 — Start a chain reaction
- Primary actor: Owner (double-taps their cube during a group moment). Requirements: REQ-F-12.
  Views: [diagrams/social-group/](../diagrams/social-group/) 02b, 05b (PDR G3 validated 2026-10-22, issue #55).
- Preconditions: the cube hears at least one present neighbour (UC7).
- Minimal guarantee: alone, a double tap is the usual solo Joy; no cube reacts twice to one event
  and the radio carries at most one frame per cube per event.
- Success guarantee: every cube in range (and up to three relays beyond) reacts once, one after
  another, at most 300 ms apart.
- Main success scenario:
  1. The owner double-taps cube A; A shows its Joy and broadcasts a chain event.
  2. Each neighbour waits 300 ms times its place in the group (MAC order), then shows Joy with
     its Joy sound and relays the event once.
  3. Copies of the event heard again are ignored; the wave dies out.
- Extensions:
  - 1a. A is alone: solo Joy, nothing is sent.
  - 1b. A started a chain less than 10 s ago: solo Joy, nothing is sent.
  - 2a. The same event id is heard again: ignored.
  - 2b. A neighbour is dozing or asleep: it does not react but still relays.
  - 2c. A neighbour reacted to a chain less than 5 s ago: it ignores the new one.
  - 2d. The event has already been relayed three times: the last cube reacts but does not relay.

### UC10 — Gather the cubes for a choir (tier B)
- Primary actor: Team (one colleague strokes their cube while the cubes are gathered).
  Requirements: REQ-F-14. Views: [diagrams/social-group/](../diagrams/social-group/) 02c, 05c
  (issue #57).
- Preconditions: the cubes are in radio range of the stroked cube and present to it (UC7).
- Minimal guarantee: alone, a stroke is the usual solo Purr; a cube never sings louder than the
  Sound gate's volume, never while asleep, and at most once per minute.
- Success guarantee: every awake cube in range of the conductor sings one voice of the same
  melody, all starting within 50 ms of each other (FS8).
- Main success scenario:
  1. The colleague strokes cube A; A purrs and broadcasts a choir start, "in 500 ms".
  2. Each awake neighbour takes a voice from its place in the group (MAC order, three voices).
  3. After 500 ms every cube, A included, sings its voice; the choir ends with the melody.
- Extensions:
  - 1a. A is alone: solo Purr, nothing is sent.
  - 1b. A took part in a choir less than 60 s ago: solo Purr, nothing is sent.
  - 2a. A neighbour is dozing or asleep: it stays silent.
  - 2b. A neighbour sang in a choir less than 60 s ago, or is already singing: it stays silent.
  - 2c. A neighbour handles the start too late (after its start time): it stays silent.
  - 2d. A cube is out of A's range: it does not sing (the start is not relayed).
  - 3a. A cube falls asleep while singing: its voice stops.

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

### UC12 — Add a capability by plugging a module
- Primary actor: Owner (or Maker on their behalf). Requirements: REQ-F-20, REQ-NF-12.
  Decisions: ADR-0016. Views: [diagrams/expansion-port/](../diagrams/expansion-port/).
- Preconditions: the cube is unplugged from USB; the owner has a Qwiic/STEMMA QT module whose
  behaviour the cube's firmware already knows (shipped with it or received by update, V1.5).
- Minimal guarantee: the cube boots and behaves as before, whatever is plugged in.
- Success guarantee: after the next boot the cube uses the module (e.g. it vibrates when
  petted).
- Main success scenario:
  1. The owner unclips the rear cover; the module bay and J1 are in view.
  2. The owner plugs the module's Qwiic cable into J1, lays the module in the bay and clips the
     cover back.
  3. The owner plugs the cube in; at boot the cube scans the I2C bus, recognises the module's
     address and identity, and enables the matching behaviour.
  4. The cube greets its new capability once (a short reaction), then lives as usual.
- Extensions:
  - 3a. The module is unknown to this firmware: the cube ignores it and boots as usual; the
    behaviour arrives with a later update (V1.5 OTA), no reflash by the owner.
  - 3b. The module answers on the address of a fitted sensor (IMU 0x68/0x69, light 0x23/0x5C):
    the conflict breaks a built-in sensor; the cube keeps running with what still answers, and
    the owner removes the module (address list: ADR-0016; behaviour verified on the bench at
    the module PDR).
  - 3c. The module draws more than the budget (REQ-NF-12): out of specification; the module
    list in the docs only names modules within the budget.
  - 2a. The cube was not unplugged: hot-plugging is not supported; the module is only seen at
    the next boot.
