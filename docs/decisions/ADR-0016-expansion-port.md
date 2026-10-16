# ADR-0016: Expansion port J1 — power budget, internal module bay, plug-and-play by address scan

**Status:** Accepted — 2026-10-16

## Context

Issue #18: a delivered cube should keep growing, by updates and by plugging a small board (for
example a vibration driver or an extra sensor). After the CDR the hardware is frozen
(ADR-0011), so the port must be settled now.

Already merged in PR #34:

- J1 is a JST-SH 4-pin Qwiic/STEMMA QT connector (GND, 3V3, SDA, SCL), on the shared I2C bus.
- J1 has no interrupt line: no GPIO is free (ADR-0007).
- C-0007 counts the pull-ups of one extra module.

Still open: the power a module may take, where the module sits in the enclosure, and how one
firmware handles cubes with and without a module (ADR-0008). Each choice was scored with an
MCDA matrix (0–5 scores, weights out of 100, totals out of 500); the owner validated the top
option.

## Decision

- **D1 — Module power budget: ≤ 50 mA continuous, ≤ 75 mA peak (< 1 s)** from the XIAO 3V3
  ([C-0012](../hardware/calculations/log.md)).
  - The owner validated 50 / 80 mA; the calculation trims the peak to 75 mA so the cube stays
    within 500 mA (C-0001's peak is 420.3 mA).
  - Rejected: 100 mA continuous, which drops the LDO thermal margin under 2×.
  - Modules above the budget need their own supply and are out of V1. A vibration motor runs in
    bursts only.
- **D2 — Internal module bay behind a clipped rear cover** (415/500).
  - Rejected: a slot with the module outside the cube (365), which hurts the shape (C6) and
    exposes the connector; a module fitted by the maker with no later access (335), which
    breaks the "add a capability" goal for a non-maker (C1).
  - Bay 20 × 20 × 6 mm, J1 reachable inside, cover held by two snap hooks, no screws
    (REQ-NF-11). Dimensions: `docs/hardware/mechanical-spec.md`.
- **D3 — Plug-and-play principle in firmware** (design rule, implemented at the first module's
  PDR):
  - At boot, after the built-in sensors, the firmware probes a fixed allowlist of I2C addresses
    of known modules. A device is accepted only when its identity check passes (for example a
    chip-ID register), never on an address alone.
  - Anything not on the allowlist, or failing its identity check, is ignored. No scan result is
    trusted as input beyond "present / absent" (security policy INPUT rules).
  - The addresses of the fitted sensors (MPU6050 0x68/0x69, BH1750 0x23/0x5C) are reserved; a
    module on one of them is out of specification.
  - One firmware for every cube (ADR-0008, C4): presence is detected at run time, never set by
    a build flag or per-cube NVS key. A new module's behaviour ships in a firmware update (V1.5
    OTA, roadmap T6).
  - Probe once at boot; no hot-plug in V1.
  - Probing and the identity check are pure logic in `firmware/lib/` behind an I2C port, tested
    natively; the Arduino `Wire` adapter stays thin in `firmware/src/`.

## Consequences

- REQ-F-20 and REQ-NF-12 are added; UC12 describes the owner's goal; views under
  `docs/architecture/diagrams/expansion-port/`.
- The enclosure script (not written yet, ADR-0010) must include the bay and the cover; the
  envelope may grow by the bay depth if it does not fit beside the modules.
- Bench at the CDR: LDO temperature with a 50 mA dummy load on J1, and a boot with an unknown
  Qwiic board fitted.
- No change to the schematic, the GPIO budget or the benches.
