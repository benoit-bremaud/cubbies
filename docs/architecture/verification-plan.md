# Verification plan

> The right side of the V: how every requirement and every component choice is checked, validated
> and approved. Calculations and simulations live in the repository and are reviewed in pull
> requests like code; approval happens at the gates in [gates.md](gates.md).
> Decision: ADR-0011.

## Methods and tools

| Method | Tool | Lives in | Used for |
|--------|------|----------|----------|
| Calculation | spreadsheet-free Markdown tables with formulas and values | `docs/hardware/calculations.md` | current budget, buzzer drive, RC values, battery (V2) |
| Analog simulation | KiCad 9 simulator / ngspice 42, batch via `kicad-cli sch export netlist --format spice` then `ngspice -b` | `hardware/electronics/sim/*.cir` + exported plots | only where a real question exists: buzzer driver (flyback spike, MOSFET dissipation), battery divider (V2) |
| Native unit tests | PlatformIO `env:native`, Unity | `firmware/test/` | moods, sleep, identity, gestures, protocol, rate limits |
| Golden images | native display adapter rendering the 96×64 framebuffer to PNG, compared to references | `firmware/test/golden/` | every expression and transition (REQ-F-01, REQ-F-02, REQ-NF-08) |
| Fuzzing | native test feeding random and malformed frames to the parser | `firmware/test/` | REQ-NF-06 |
| ERC | `kicad-cli sch erc` | CI (later) and local | GPIO contract, net sanity |
| Bench measurement | multimeter, phone sound meter, two prototypes, camera for latency | results recorded in `docs/hardware/measurements.md` | current, dB, range, latency, frame rate |
| Print trial | the printed enclosure on prototype hardware | photos in `docs/hardware/assembly.md` | REQ-NF-10, REQ-NF-11 |
| Exploration only | Wokwi (XIAO ESP32-C3, buzzer, MPU6050, SSD1306 as a stand-in) | not versioned | prototyping gestures without hardware; no SSD1331, no I2S mic |

## Matrix: decision or requirement → control → validation → approval

| Decision / requirement | Control (desk) | Validation (evidence) | Approval |
|------------------------|----------------|-----------------------|----------|
| Buzzer driver, max volume, Sound gate (REQ-F-04, REQ-F-05, REQ-NF-03) | calculation | ngspice transient + phone dB meter; native tests of the gate | CDR |
| Display RESET filter | calculation (≈ 500× margin on the reset time) | bench: the display starts on 10 cold boots | CDR |
| Current budget V1 (REQ-NF-04), autonomy V2 | calculation table | bench current | CDR, then V2 |
| GPIO allocation (C3, ADR-0007) | review against ADR-0007 | `kicad-cli sch erc` | CDR (signed contract) |
| Eyes and expressions (REQ-F-01, 02, NF-08) | native PNG render | golden images in CI | PDR (owner review of the images) |
| Moods, sleep, identity, protocol (REQ-F-05..13) | Unity tests | replayed scenarios with injected clock | PDR |
| Radio range (REQ-NF-05) | estimate | measured with two prototypes | V1 |
| Latency, frame rate (REQ-NF-01, 02) | design budget | bench | V1 |
| Enclosure (REQ-NF-10, 11) | FreeCAD script, dimensions | print and fit on prototype | mechanical CDR |
| Privacy of audio (REQ-NF-07, C7) | architecture rule | code review of the audio adapter | every PR |

## Rules

- A measurement without a recorded value and date is not a verification.
- A simulation is committed with its netlist, its command and its exported plot; the PR explains
  what the plot proves.
- A golden image changes only in a PR whose description shows the before/after.
