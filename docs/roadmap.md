# Roadmap and prioritised backlog

Versions are incremental: the simplest working cube first, then behaviours added in order of
implementation difficulty. Each item becomes an issue with acceptance criteria and its UML views
before it is coded (conception gate).

## Versions

| Version | Theme | Hardware delta | Gate |
|---------|-------|----------------|------|
| V0 | Prototypes | 2–3 cubes, OLED vs LCD comparison, breadboard | eyes animate, touch and buzzer work |
| V1 | Living cube + social layer | P0–P3 parts, USB-C, printed enclosure | adoption day with the first batch |
| V1.5 | Mother cube and updates | none (Wi-Fi credentials in NVS on one cube) | a release reaches every cube without a cable |
| V2 | Nomad cube | LiPo + switch, enclosure revision | a cube survives a day unplugged with sleep |
| V3 | Wireless charging | Qi receiver in the base | charges on the office desk pads |
| V4 (idea) | Voice | XIAO S3 + I2S amp + speaker | only if V1–V3 prove the demand |

## Backlog, ordered from simplest to hardest

Priority tiers: **A** = V1 must-have, **B** = V1 if time allows, **C** = after V1.
Within a tier, the order is the implementation order.

### Solo behaviours

| # | Behaviour | Tier | Depends on |
|---|-----------|------|------------|
| S1 | Procedural eyes in the cube's colour: idle, blink, look around | A | P0 |
| S2 | Mood engine: happy, curious, tired, bored, annoyed, with smooth transitions | A | S1 |
| S3 | Touch reactions: tap (surprise), double tap (joy), long stroke (purr, eyes close) | A | S2 |
| S4 | Expressive beeps per mood, wake-up melody, volume setting with quiet hours | A | S2 |
| S5 | Sleep / wake: dozes after inactivity, yawns, wakes on touch | A | S2 |
| S6 | Identity: MAC-derived default colour, adoption ritual, secret reset gesture | A | S1 |
| S7 | Sound reactions: wakes on noise, reacts to voice level, clap | A | P1, S5 |
| S8 | Day rhythm: yawns in the evening, lullaby when the office goes dark, Friday mood | B | S5, P3 |
| S9 | Eyes follow tilt; shake = annoyed; upside down = sleeps | B | P2 |
| S10 | Sulks when ignored for two days | B | S2, persisted counters |
| S11 | Dream animation while asleep; "reads" when the owner types for long | C | S5, S7 |

### Social behaviours (ESP-NOW broadcast)

| # | Behaviour | Tier | Depends on |
|---|-----------|------|------------|
| G1 | Presence: broadcast identity + mood, keep a neighbour table, greet a newcomer | A | S6 |
| G2 | Contagious mood: neighbours' moods pull mine | A | G1, S2 |
| G3 | Chain reaction: one cube triggers, the others react one after another | A | G1 |
| G4 | Synchronised choir: shared clock, multi-voice melody | B | G1, S4 |
| G5 | Collective sleep: the first yawn spreads | B | G2, S5 |
| G6 | Jealousy: the owner pets another cube in range, mine sulks | B | G1, S3 |
| G7 | Pass the ball: a tap sends a virtual object to a neighbour | C | G1, S3 |
| G8 | Team poll: one cube asks, each answers by tap, all display the result | C | G7 |
| G9 | Chinese whispers: each cube distorts the melody before passing it on | C | G3, S4 |
| G10 | Flirting (cartoon level): hearts, blush, wink, serenade, bump = kiss, slow dance of blinks | C | G1, P2 |
| G11 | Birthday / team counter / shared events | C | V1.5 mother cube |

### Platform

| # | Item | Tier |
|---|------|------|
| T1 | PlatformIO skeleton, two board envs, native tests, CI build + test | A |
| T2 | Pin map and hardware abstraction (display, audio, touch, radio, sensors as adapters) | A |
| T3 | Versioned ESP-NOW frame format with allowlist parsing | A |
| T4 | Security CI baseline (gitleaks, dependency review) | A |
| T5 | Enclosure v1: 40 mm cube, snap-fit, OLED window, touch pad on top, USB-C at the back | A |
| T6 | OTA relayed by the mother cube (ESP-NOW OTA) | V1.5 |
| T7 | Power management: light sleep, screen off, battery gauge | V2 |
