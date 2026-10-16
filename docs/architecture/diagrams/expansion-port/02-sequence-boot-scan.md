# Sequence diagram — expansion port — boot finds the plugged module

> **Frame**: `sd — Boot probes the expansion port`
> **Feature**: `expansion-port` — issue #18
> **Related ADRs**: ADR-0016 (budget, bay, plug-and-play), ADR-0008 (one firmware), ADR-0007 (no free GPIO, no interrupt)
> **Use case**: UC12 in [specs/use-cases.md](../../specs/use-cases.md)
> **Modules and ports**: [03-component.md](03-component.md)

## Context

This diagram answers "how does one firmware use a module that only some cubes have?". It is the
design principle of ADR-0016 D3; the code comes with the first module's PDR. The use-case view
is UC12 in [cube-core/01-use-case.md](../cube-core/01-use-case.md).

## Diagram

```mermaid
sequenceDiagram
  participant M as main.cpp
  participant R as Module registry (lib)
  participant B as WireBus (II2cBus)
  participant D as Module behaviour (lib)

  Note over M: built-in sensors are set up first; their addresses are reserved
  M->>R: probe(bus)
  Note over R: the allowlist excludes the reserved sensor addresses (native test)
  loop each entry of the allowlist (address, identity check)
    R->>B: readByte(address, idRegister, timeout)
    alt no ACK, timeout or stuck bus
      B-->>R: absent (bus recovered, boot goes on)
    else ACK
      B-->>R: id value
      alt id matches the entry
        Note over R: module present
      else id mismatch
        Note over R: ignored (unknown device)
      end
    end
  end
  R-->>M: list of present known modules
  opt a known module is present
    M->>D: enable(bus)
    Note over D: reads mood engine output each tick
  end
  Note over M,D: nothing else happens on the bus; no hot-plug in V1
```

## Rules carried by the diagram

| Case | Outcome |
|------|---------|
| No device on J1 | registry empty, cube unchanged (UC12 minimal guarantee) |
| Known address, identity OK | behaviour enabled |
| Known address, identity wrong | ignored |
| Unknown address | never probed, so ignored |
| Bus stuck or timeout | that probe counts as absent; bus recovered; boot never blocks |
| Address of a fitted sensor | reserved, out of specification (UC12 3b) |
