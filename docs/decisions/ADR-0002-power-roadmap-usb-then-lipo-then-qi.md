# ADR-0002: Power roadmap — USB-C in V1, LiPo in V2, Qi wireless charging in V3

**Status:** Accepted — 2026-10-14

## Context

Current budget of an awake cube (ESP32-C3 with radio 27–75 mA, SSD1331 OLED 20–40 mA, INMP441
~1.5 mA, buzzer 20–30 mA while sounding): roughly 80–150 mA. A 1000 mAh LiPo therefore lasts
7–12 h awake; a cube that is "alive" all day cannot run on battery without sleeping most of the
time. Super Mini boards have no charger (an external TP4056 would be needed); XIAO boards have
one onboard. Office desks are equipped with Qi induction chargers. Power banks cut off below
~50–100 mA, so they are not a fallback. The owner wants progressive versions.

## Options and scoring

Criteria: colleague friction (3), cube awake and alive (3), KISS (2), cost ×N (2), gift safety (2).

| Criterion (weight) | 1. USB-C permanent | 2. LiPo + charger | 3. Both, battery optional | 4. USB-C + small reserve |
|---|---|---|---|---|
| Friction (3) | 3 | 1 | 2 | 3 |
| Awake / alive (3) | 3 | 1 | 2 | 3 |
| KISS (2) | 3 | 1 | 1 | 1 |
| Cost ×N (2) | 3 | 1 | 2 | 2 |
| Safety (2) | 3 | 2 | 2 | 3 |
| **Total /39** | **39** | 17 | 23 | 31 |

## Decision

- **V1: USB-C permanent.** The cube is plugged on the desk like a lamp; a USB power strip powers
  the adoption day. No battery, no power management.
- **V2: LiPo with protection**, charged by the board's onboard charger, with light sleep and
  screen-off; a cube that is taken to a colleague's desk survives the trip.
- **V3: Qi receiver in the enclosure base** feeding the board's 5 V input, so the cube charges
  on the office desk pads.

## Consequences

- The board must carry a LiPo charger from V1 so that V2 and V3 change the enclosure only
  (ADR-0003 chooses the XIAO ESP32-C3 for that reason).
- The enclosure v1 reserves no battery volume; the v2 revision adds it. YAGNI applied.
- Power management (T7 in the roadmap) is deferred to V2 entirely.
