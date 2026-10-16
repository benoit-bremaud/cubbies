# ADR-0015: Hardware validation fixes before the CDR (strapping pin, display reset, buzzer current)

**Status:** Accepted — 2026-10-16

## Context

The first calculation and simulation pass (V-SEA method; [calculation log](../hardware/calculations/log.md)
C-0001 to C-0011) found three drafted choices that fail their datasheet requirement at worst-case
corners:

1. **GPIO2** must read 1 at reset (ESP32-C3 boot-mode table). The buzzer gate's 100 kΩ pull-down
   holds it at 0.005 V (C-0008).
2. The **SSD1331 RES# RC** of 10 kΩ / 100 nF leaves RES# above VIL once VDD is stable whenever
   the 3V3 ramp lasts 0.6 ms or more (C-0006).
3. The **buzzer series resistor** of 68 Ω lets the coil reach 43.7 mA against a 40 mA rating
   (C-0003).

Each option below was scored with an MCDA matrix (0–5 scores, weights out of 100, totals out of
500). The owner validated the top-scored option of each matrix.

## Decision

- **D1 — swap buzzer and I2C SDA** (430/500).
  - Rejected: a pull-up on GPIO2 (410), which turns the MOSFET on at reset and clicks the buzzer
    at every boot; keeping the draft with a bench check (340).
  - XIAO ESP32-C3: buzzer on GPIO6 (D4), SDA on GPIO2 (D0), held high by the module pull-ups.
  - C3 Super Mini: buzzer on GPIO0, SDA on GPIO8.
  - This changes the GPIO allocation, not the 11-GPIO budget of ADR-0007.
- **D2 — display RES# RC: 10 kΩ / 1 µF X7R plus a 1N4148 discharge diode** (470/500).
  - Rejected: 100 kΩ / 100 nF (395), which a module's own pull-up defeats; keeping the draft
    (270).
- **D3 — buzzer series resistor: 82 Ω** (440/500).
  - Rejected: keeping 68 Ω on a coil-power argument (380); 75 Ω (340), which still gives
    40.2 mA.
  - 82 Ω gives 37.2 mA at the worst corner.

## Consequences

- `firmware/include/pins.h`, the electronics-spec GPIO contract and `bom.md` change in the same PR.
  No firmware logic changes, since the adapters take their pins from `pins.h`.
- The volume falls by about 1.7 dB at a given duty, which is compatible with REQ-NF-03. The dB
  mapping is still measured on the bench.
- The display leaves reset about 16 ms after power-up.
- Open before the CDR:
  - a second engine (LTspice) on the Tier 1 entries;
  - a bench boot and download check with every module fitted;
  - a measurement of the I2C rise time.
