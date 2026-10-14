# ADR-0008: Identity — MAC-derived default colour, overridden by an adoption ritual stored in NVS

**Status:** Accepted — 2026-10-14

## Context

Every cube runs the same firmware (no per-cube builds, no flashing by recipients). A cube needs a
stable identity for the social layer (its MAC address) and a human-facing one (its colour, shown
by the eyes and by the enclosure). A name cannot be typed on a device without a keyboard; in V1
the identity is colour + MAC. The adoption day is a ritual: the colleague choosing the colour of
their cube's eyes, cube in hand, is the moment of adoption. Spare cubes must be re-adoptable
without a cable.

## Options and scoring

Criteria: one firmware for all (3), friction and ritual (3), robustness (2), KISS (2).

| Criterion (weight) | MAC default + adoption ritual (NVS) | MAC only | Build flag per cube | Assigned by the mother cube |
|---|---|---|---|---|
| One firmware (3) | 3 | 3 | 1 | 3 |
| Friction + ritual (3) | 3 | 2 | 3 | 2 |
| Robustness (2) | 3 | 1 | 3 | 2 |
| KISS (2) | 2 | 3 | 2 | 1 |
| **Total /30** | **28** | 24 | 22 | 23 |

## Decision

- On first boot the colour is **derived from the MAC address** (never an empty identity).
- The **adoption ritual** overrides it: tap cycles through a palette of 8–12 colours that render
  well on the OLED, a long stroke confirms, the choice is stored in NVS and survives OTA updates.
- A **secret gesture** (touch held at boot) re-runs the adoption.
- Two cubes may share a colour; the social layer keys on the MAC, humans on the colour.

## Consequences

- Identity rules (palette, MAC hashing, ritual state machine) are pure logic, tested natively.
- Names come later (mother cube or phone), as an optional NVS field the V1 firmware ignores.
