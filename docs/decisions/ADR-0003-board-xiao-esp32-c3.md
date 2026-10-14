# ADR-0003: Board — Seeed XIAO ESP32-C3 for the series, ESP32-C3 Super Mini for prototypes

**Status:** Accepted — 2026-10-14

## Context

Verified facts: XIAO C3, C6 and S3 expose 11 GPIO on their headers, the C3 Super Mini about as
many usable ones; the ESP32-C3 and C6 have no capacitive-touch peripheral (an external TTP223 is
needed), the S3 has one. The XIAO C3 charges a LiPo at ~370 mA, the XIAO C6 at 100 mA only. The
ESP32-C6 requires Arduino core 3.x, which the official PlatformIO platform does not provide (a
community fork, pioarduino, is required). BLE 5.3, Zigbee and Thread of the C6 serve no identified
need. The Super Mini has no charger, which the power roadmap (ADR-0002) needs from V1. The XIAO
S3 adds PSRAM, a dual core and native touch at ~9 € and higher consumption, with no V1 use.

## Options and scoring

Criteria: V2/V3 roadmap without a board change (3), official toolchain (2), price ×N (2), GPIO
(2), Mochi-clone community (2).

| Criterion (weight) | XIAO C3 | XIAO C6 | XIAO S3 | C3 Super Mini |
|---|---|---|---|---|
| Roadmap V2/V3 (3) | 3 | 2 | 3 | 1 |
| Official toolchain (2) | 3 | 1 | 3 | 3 |
| Price ×N (2) | 2 | 2 | 1 | 3 |
| GPIO (2) | 2 | 2 | 2 | 2 |
| Community (2) | 2 | 2 | 2 | 3 |
| **Total /33** | **27** | 20 | 25 | 23 |

## Decision

**Seeed XIAO ESP32-C3** for the series. The 2–3 prototypes may use the **ESP32-C3 Super Mini**
(same chip, same firmware, one PlatformIO env per board with its own pin map). The S3 is kept as
the candidate for a hypothetical V4 "voice" version.

## Consequences

- `platformio.ini` carries two board envs; `firmware/include/pins.h` has one block per env.
- Touch is an external TTP223 module (1 GPIO).
- Every sensor beyond the P0/P1 set goes on I2C (ADR-0007).
