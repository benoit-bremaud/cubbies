# ADR-0011: Design dossier structure, verification plan and gates

**Status:** Accepted — 2026-10-14

## Context

The owner requires a formal design dossier before any code: Clean architecture and SOLID
throughout, UML-first, decisions traceable, component choices controlled, validated and approved
with calculators and simulators. The product mixes firmware, electronics and a printed enclosure,
is built by hand in a series of more than ten units, and will be published. Models examined:
arc42 (12 optional sections), C4 (zoom levels), 4+1 (large systems), ISO/IEC/IEEE 42010
(conformance, no template), Cockburn textual use cases, NF X50-151 / EN 16271 functional
specification, OSHWA documentation practices (native design files, interchange exports, BOM with
designators, assembly instructions, licences), PDR/CDR review gates. Verification tools
available: KiCad 9 simulator with ngspice 42 and `kicad-cli` batch export, KiCad PCB calculator,
PlatformIO native tests, a host display adapter for golden images; Wokwi lacks the SSD1331 and
the I2S microphone; Espressif QEMU is ESP-IDF-only.

## Options and scoring (dossier)

Criteria: proportionality (3), coverage of firmware + electronics + mechanics + making (3),
traceability and gates (2), recognised models (2), maintenance (2).

| Criterion (weight) | A. arc42-lite + C4 + Cockburn + matrix | E. A + hardware specs | F. E + functional requirements + OSHWA + gates | G. F + ISO 42010 viewpoints |
|---|---|---|---|---|
| Proportionality (3) | 3 | 2 | 2 | 1 |
| Coverage (3) | 1 | 2 | 3 | 3 |
| Traceability, gates (2) | 2 | 3 | 3 | 3 |
| Recognised models (2) | 3 | 3 | 3 | 3 |
| Maintenance (2) | 3 | 2 | 2 | 1 |
| **Total /36** | 28 | 28 | **31** | 26 |

(Option letters follow the Q&A session: B, C and D were intermediate variants, discarded.)

## Options and scoring (verification tooling)

Criteria: proportionality (3), traceability (3), validates the core before hardware (3), setup
cost (2), value for V2/V3 (2).

| Criterion (weight) | A. calculations + targeted ngspice + bench | B. A + Wokwi in CI | C. bench only | D. A + native display simulator (golden images) | E. D + Wokwi exploration |
|---|---|---|---|---|---|
| Proportionality (3) | 3 | 2 | 3 | 3 | 2 |
| Traceability (3) | 3 | 3 | 1 | 3 | 3 |
| Core before hardware (3) | 1 | 2 | 1 | 3 | 3 |
| Setup cost (2) | 2 | 1 | 3 | 2 | 1 |
| V2/V3 value (2) | 3 | 3 | 1 | 3 | 3 |
| **Total /39** | 31 | 29 | 23 | **37** | 32 |

## Decision

**Dossier F** and **verification D**:

- `docs/architecture/README.md` becomes an arc42-lite overview (sections 1, 2, 3, 4, 5, 8, 10,
  11, 12 only), with C4 context and container views.
- `docs/architecture/specs/requirements.md`: service functions and constraints with criteria and
  levels, then numbered REQ-F / REQ-NF with a "carried by" column.
- `docs/architecture/specs/use-cases.md`: Cockburn textual use cases by domain, each citing its
  requirements; `diagrams/<feature>/` holds the UML views whose question applies.
- `docs/architecture/traceability-matrix.md`, `verification-plan.md`, `gates.md` (PDR before
  code per feature, CDR before ordering the series).
- `docs/hardware/`: electronics spec with the signed GPIO contract, calculations, mechanical
  spec, assembly guide (OSHWA), measurements.
- `docs/research/state-of-the-art.md`.
- Verification: calculations and ngspice netlists committed with plots; native tests; a native
  display adapter producing PNG golden images for every expression; bench measurements
  recorded; Wokwi for manual exploration only.

## Consequences

- No feature starts before its PDR checklist passes; no series order before the CDR.
- The first UML study (eight diagrams, drafted before this ADR) is kept as the first draft and
  re-validated against `requirements.md` at the `cube-core` PDR.
- A `Display` port with a real SSD1331 adapter and a PNG adapter is the justified seam; it is
  not speculative.
