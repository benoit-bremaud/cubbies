# Component diagram — expansion port

> **Frame**: `cmp — Expansion port`
> **Feature**: `expansion-port` — issue #18
> **Related ADRs**: ADR-0016, ADR-0007, ADR-0008, ADR-0014 (pure lib, thin adapters)
> **Base view**: [cube-core/03-component.md](../cube-core/03-component.md)

## Context

This diagram answers "where does module support live, and what does it depend on?". The
dependency points inward: the lib never includes Arduino.

## Diagram

```mermaid
flowchart LR
  subgraph HW[Hardware]
    J1["J1 Qwiic JST-SH<br/>GND 3V3 SDA SCL"]
    MOD["Expansion module<br/>≤ 50 mA, ≤ 75 mA peak"]
    BUS[("I2C bus 400 kHz")]
    SENS["IMU, light sensor"]
  end
  subgraph SRC[firmware/src adapters]
    W[WireBus]
    MAIN[main.cpp]
  end
  subgraph LIB[firmware/lib pure logic]
    P{{II2cBus port}}
    REG["Module registry<br/>allowlist + identity check"]
    BEH["Module behaviours<br/>one per known module"]
    ME[Mood engine]
  end
  MOD --- J1 --- BUS
  SENS --- BUS
  BUS --- W
  W -. implements .-> P
  REG --> P
  BEH --> P
  BEH --> ME
  MAIN --> REG
  MAIN --> BEH
```

## Notes

- `II2cBus` is the only new port; it is created at the first module's PDR, together with the
  first behaviour and its native tests.
- The registry holds a compile-time allowlist; the same table is in every cube (ADR-0008).
