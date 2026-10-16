# Verification plan

> The right side of the V: how every requirement and every component choice is checked, validated
> and approved. Calculations and simulations live in the repository and are reviewed in pull
> requests like code; approval happens at the gates in [gates.md](gates.md).
> Decision: ADR-0011.

## Methods and tools

| Method | Tool | Lives in | Used for |
|--------|------|----------|----------|
| Calculation | append-only log of C-NNNN entries with criticality tiers (V-SEA method) | `docs/hardware/calculations/log.md`, `hardware/electronics/sim/calc.py` | current budget, LDO, gate drive, interface windows, battery (V2) |
| Analog simulation | ngspice 42 benches with worst-case corners and negative controls (`sh run.sh`); values from the KiCad export once the schematic exists | `hardware/electronics/sim/*.cir`, `results/` tables and SVG plots | buzzer stage (C-0003/4), display reset RC (C-0006), I2C rise time (C-0007), strapping level (C-0008), battery divider (V2) |
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
| Buzzer driver, max volume, Sound gate (REQ-F-04, REQ-F-05, REQ-NF-03) | C-0003, C-0004, C-0005 | `buzzer-driver.cir` (64 corners, simulated) + phone dB meter; native tests of the gate | CDR |
| Display RESET filter | C-0006 (draft RC failed; ADR-0015 D2) | `oled-reset-rc.cir` (24 corners, simulated); bench: the display starts on 10 cold boots | CDR |
| Strapping pins (GPIO2, GPIO8, GPIO9) | C-0008 (ADR-0015 D1), C-0014 (R6 on GPIO8) | `strapping-gpio2.cir` + `crosscheck.py` (C-0015); bench: boot + download mode with every module fitted | CDR |
| I2C bus at 400 kHz | C-0007 | `i2c-bus.cir` (48 corners) + `crosscheck.py` (C-0015); bench: scope rise time | CDR |
| Interfaces: INMP441, TTP223, SSD1331 SPI | C-0009, C-0010, C-0011 | calculation (tier 3); bench | CDR |
| Current budget V1 (REQ-NF-04), LDO thermal, autonomy V2 | C-0001, C-0013 (XC6210, supersedes C-0002) | bench current and LDO temperature | CDR, then V2 |
| Expansion port J1 module budget (REQ-NF-12, ADR-0016) | C-0012, C-0013 | calculation (tier 1, provisional: passes at the nominal θJA, not at the 400 °C/W bound); bench: LDO temperature with a 50 mA dummy load on J1 is a CDR exit criterion | CDR |
| Expansion modules ignored unless known (REQ-F-20) | ADR-0016 D3 | native tests of the module registry at the module PDR; bench: boot with an unknown Qwiic board fitted | module PDR, CDR (bench) |
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
