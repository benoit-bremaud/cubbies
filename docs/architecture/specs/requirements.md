# Requirements — cubbies V1

> **Method**: functional expression of the need (NF X50-151 / EN 16271 style): service functions
> and constraints first, each with assessment criteria and levels, then numbered requirements.
> Written to leave solutions open; the ADRs record the solutions chosen, and the
> [verification plan](../verification-plan.md) owns how each requirement is verified.
> **Status**: draft for owner validation (PDR input, see [gates.md](../gates.md)).
> **Scope**: V1 (USB-powered cube, tier A behaviours). V2/V3 requirements are listed as reserved
> identifiers so the V1 design does not block them.

## 1. Need

A small desk companion offered to each colleague of a team. Alone on a desk it must feel alive
(eyes, moods, reactions, sleep). When other companions are within radio range it must take part in
group behaviours without any pairing, app, server or configuration by the recipient. Recipients
are not makers: the cube works from the moment it is plugged in.

## 2. Service functions (what the product does for whom)

| Id | Service function | Beneficiary | Assessment criterion | Level | Flexibility |
|----|------------------|-------------|----------------------|-------|-------------|
| FS1 | Show a living, expressive face | owner | number of distinguishable expressions; idle motion never freezes | ≥ 6 expressions; eyes stay open 2–6 s between two blinks (measured from the reopening); never a static frame for more than 10 s while awake | F1 (negotiable above 4) |
| FS2 | React to the owner's attention | owner | touch gestures recognised; reaction latency | tap, double tap, long stroke; visible reaction within 150 ms | F0 (latency) |
| FS3 | React to the sound environment | owner | wake on noise; react to voice level | wakes on a clap or voice at 1 m; no reaction to office hum | F1 |
| FS4 | Be heard without disturbing | owner, neighbours | sound level at 1 m; silence when nobody is there; adjustable volume | ≤ 55 dB(A) at max, ≤ 40 dB(A) default; silent while asleep and (from S8) when the office is dark; volume constant in V1, 4 steps once a gesture exists | F1 (steps) |
| FS5 | Live a daily rhythm | owner | sleeps when inactive and (from S8) when the office goes dark; wakes on attention | dozes after 10 min, sleeps after 30 min; wakes on touch or sound | F1 |
| FS6 | Carry an identity | owner, team | colour chosen by the owner, kept across power cycles and updates | palette of 8–12 colours; choice survives OTA | F0 |
| FS7 | Notice and greet other companions | team | neighbours detected within range; greeting on arrival | detection within 5 s at 10 m indoors; greeting once per arrival | F1 (range) |
| FS8 | Share moods and react together | team | contagious mood; chain reaction; synchronised choir; collective sleep | mood converges within 60 s; an idle cube catches a neighbour's yawn within 15 s; chain hop ≤ 500 ms; choir drift ≤ 50 ms | F2 (choir is tier B) |
| FS9 | Be built by hand in a small series | maker | parts off the shelf; assembly time; no per-unit firmware | ≤ 45 min per cube after the first; one firmware image | F0 |

## 3. Constraints

| Id | Constraint | Criterion / level | Origin |
|----|-----------|-------------------|--------|
| C1 | Zero friction for the recipient | no app, no pairing, no Wi-Fi credentials, no flashing | ADR-0001 |
| C2 | USB-C desk power only in V1 | 5 V, ≤ 500 mA peak | ADR-0002 |
| C3 | Board: Seeed XIAO ESP32-C3; 11 GPIO hard budget | every peripheral fits the pin map; extras on I2C | ADR-0003, ADR-0007 |
| C4 | One firmware for every cube | identity in NVS, never in build flags | ADR-0008 |
| C5 | Radio: ESP-NOW broadcast, no router | frames versioned and validated | ADR-0005 |
| C6 | Cube form factor ≈ 40 mm, 3D printed | OLED window, touch pad on top, USB-C at the back | ADR-0006 |
| C7 | Privacy: the microphone never records or transmits audio | only a sound level and event flags leave the microphone boundary | security policy, PRIV |
| C8 | Nothing vulgar; flirting stays at cartoon level; rare gag behaviours stay rare | gag events ≤ 1 per ~16 h of awake time, never while asleep or in the dark | owner decision |
| C9 | Licences: MIT code, CC BY-SA 4.0 enclosure | no GPL-licensed code linked into the firmware | ADR-0009 |
| C10 | No time source in V1 (no RTC, no Wi-Fi): no clock-based behaviour | "evening", "Friday" and quiet hours derive from light and activity, or wait for V1.5; the firmware's clock is a monotonic uptime (ms since boot), never a time of day | ADR-0002, UML study 2026-10-14 |

## 4. Functional requirements

`Carried by`: FW = firmware, EL = electronics, ME = mechanics.

| Id | Requirement | FS | Carried by |
|----|-------------|----|------------|
| REQ-F-01 | The cube renders procedurally animated eyes in its identity colour, with idle motion and blinking. | FS1 | FW |
| REQ-F-02 | The cube has at least five moods (happy, curious, tired, bored, annoyed) with smooth transitions. | FS1 | FW |
| REQ-F-03 | The cube recognises tap, double tap and long stroke on its touch surface and reacts to each. | FS2 | FW, EL |
| REQ-F-04 | The cube emits expressive sounds per mood and a wake-up melody. | FS4 | FW, EL |
| REQ-F-05 | Every sound the cube emits respects the current volume (a constant in V1; four stored steps chosen in a second step of the adoption ritual from V1.x, ADR-0013), and the cube is silent while asleep and, from S8, in the dark. | FS4, C10 | FW |
| REQ-F-06 | The cube dozes after inactivity, sleeps, yawns, and wakes on touch or sound. | FS5 | FW |
| REQ-F-07 | On first boot the cube derives a colour from its MAC; the adoption ritual overrides it; holding the pad during the post-boot invitation re-runs it (ADR-0013). | FS6 | FW |
| REQ-F-08 | The identity survives power cycles and OTA updates. | FS6 | FW |
| REQ-F-09 | The cube wakes on a loud sound and reacts to voice level. | FS3 | FW, EL |
| REQ-F-10 | The cube announces its identity and mood to companions in range, knows which are present, and greets a newcomer. | FS7 | FW |
| REQ-F-11 | Neighbours' moods pull the cube's mood (contagion). | FS8 | FW |
| REQ-F-12 | A cube can trigger a chain reaction that neighbours relay once each. | FS8 | FW |
| REQ-F-13 | Rare gag behaviour: a discreet "toot" with embarrassed eyes, neighbours react; rate-limited on awake time (≈ once per 16 h awake), never while asleep or in the dark. | C8, C10 | FW |
| REQ-F-14 | Synchronised choir across cubes (tier B). | FS8 | FW |
| REQ-F-20 | Collective sleep: an idle (Bored) cube catches the yawn of a drowsy neighbour (Tired, dozing or asleep) and turns Tired; a cube in use never does (tier B, G5). | FS8, FS5 | FW |
| REQ-F-15 | Eyes follow the cube's tilt; shaking annoys it; upside down it sleeps (tier B, S9). The motion sensor is fitted in V1 so the series needs no rework. | FS1, FS2 | FW, EL |
| REQ-F-20 | A delivered cube accepts one extra I2C module through an internal expansion port: the module works without a per-cube firmware, an unknown or absent module changes nothing, and the cube still boots (ADR-0016, issue #18). | FS9, C3, C4 | FW, EL, ME |

## 5. Non-functional requirements

| Id | Requirement | Criterion / level | Carried by |
|----|-------------|-------------------|------------|
| REQ-NF-01 | Reaction latency to touch | ≤ 150 ms | FW |
| REQ-NF-02 | Smoothness of the eyes | ≥ 25 fps on the chosen display | FW, EL |
| REQ-NF-03 | Sound level | ≤ 55 dB(A) at 1 m at max, ≤ 40 default | EL, FW |
| REQ-NF-04 | Current budget on USB-C | ≤ 300 mA typical, ≤ 500 mA peak | EL |
| REQ-NF-05 | Radio range indoors | ≥ 10 m through one wall | EL, FW |
| REQ-NF-06 | Radio robustness | unknown, malformed or newer-version frames dropped silently; no crash on fuzzed input | FW |
| REQ-NF-07 | Privacy | no audio samples leave the microphone boundary; only level and events | FW |
| REQ-NF-08 | OLED longevity | eyes drift ≥ 1 px every 60 s; screen dimmed in sleep | FW |
| REQ-NF-09 | Build reproducibility | one `pio run` per board env, pinned platform and libraries | FW |
| REQ-NF-10 | Assembly | ≤ 45 min per cube, no soldering on the XIAO pads beyond headers if possible | ME, EL |
| REQ-NF-11 | Enclosure | snap-fit, no screws needed, prints without supports | ME |
| REQ-NF-12 | Expansion module power | one module on J1 draws ≤ 50 mA continuous and ≤ 75 mA peak (< 1 s) from 3V3 (C-0012) | EL, FW |

## 6. Reserved for later versions

| Id | Version | Topic |
|----|---------|-------|
| REQ-V2-xx | V2 | battery autonomy, charging, light sleep, battery gauge |
| REQ-V3-xx | V3 | Qi receiver integration, thermal behaviour on the pad |
| REQ-F-16..19 | V1.5 | mother cube: Wi-Fi provisioning, release fetch, ESP-NOW OTA relay |
