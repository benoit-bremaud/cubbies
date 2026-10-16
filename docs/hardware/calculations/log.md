# Calculation log — cubbies V1

This log is append only (see the [README](README.md)). Entries are in the order they were
recorded, newest at the bottom.

How to read it:
- "Bench" means `hardware/electronics/sim/`.
- Results tables live in `results/`.
- Every **Reproduce** command runs from that folder.

## C-0001 — 2026-10-16 — Current budget on 3V3 (REQ-NF-04)

- **Purpose**: Tier 3. Check that the cube stays within the USB 500 mA peak.
- **Method**: Sum, per consumer, of the typical current and of the peak current.
- **Inputs**:
  - ESP32-C3: 84 mA in RX, 335 mA in TX 802.11b (datasheet v2.4, table 5-7).
  - SSD1331 module: 15 mA typical, 40 mA with a full-white screen (Waveshare wiki).
  - INMP441: 1.4 mA.
  - GY-521 module: 5 mA, including its LED (ASSUMED).
  - GY-302 module: 0.3 mA.
  - TTP223 module: 1.4 mA touched, including its LED (ASSUMED).
  - Buzzer stage: 37.2 mA peak (C-0003).
- **Result**:
  - Typical 106 mA, peak 420 mA, within 500 mA.
  - The peak adds bursts that never coincide for long: an ESP-NOW TX lasts under 1 ms.
- **Used in**: electronics-spec §Power; REQ-NF-04.
- **Reproduce**: `python3 calc.py`
- **Based on**: main at `dc773b1`

## C-0002 — 2026-10-16 — XIAO 3V3 LDO (TLV75733): dissipation and input headroom

- **Purpose**: Tier 1 (thermal margin under 2× at a sustained 300 mA). Bound the LDO temperature
  and confirm that USB VBUS leaves enough dropout headroom.
- **Method**:
  - Dissipation: P = (VBUS max − 3.3) × I. Temperature rise: ΔT = P × θJA.
  - Headroom: VIN = VBUS min − cable resistance × I peak.
- **Inputs**:
  - VBUS 4.75–5.25 V (USB 2.0).
  - Cable: 1 m, 2 × 0.115 Ω (AWG28, ASSUMED).
  - θJA 200 °C/W (SOT-23-5, ASSUMED).
  - Dropout 0.43 V at 500 mA (ASSUMED).
  - The TLV757P datasheet must be read for both of these last two values.
- **Result**:
  - At 106 mA typical: 206 mW, +41 °C over ambient, which is acceptable.
  - At the 420 mA peak: +164 °C if sustained, but it is a burst (ESP-NOW TX under 1 ms), so
    the junction does not follow it.
  - At a sustained 300 mA: +117 °C. A sustained 300 mA is thermally unacceptable, so the
    REQ-NF-04 "typical" ceiling is a thermal limit, as already stated.
  - LDO input at least 4.65 V, which leaves 0.92 V over 3.3 V plus dropout.
  - **OPEN**: θJA and dropout to be read at the source; LDO temperature to be measured at the
    bench (CDR).
- **Used in**: electronics-spec §Power.
- **Reproduce**: `python3 calc.py`
- **Based on**: main at `dc773b1`

## C-0003 — 2026-10-16 — Buzzer coil current and series resistor

- **Purpose**: Tier 1 (the rating was exceeded at the first run). Keep the coil current within the
  buzzer's 40 mA rating at every corner.
- **Method**: Bench `buzzer-driver.cir`, transient at 2 kHz, 64 corners:
  - 3V3 at 3.25 and 3.35 V;
  - Rs at ±5 %;
  - coil resistance 12 and 20 Ω, coil inductance 1 and 4 mH (ASSUMED, not published);
  - duty 2, 8, 20 and 50 %;
  - worst-case AO3400.
- **Inputs**:
  - Coil 16 ± 4 Ω, rated 40 mA (bom.md).
  - Gate high level 2.60 V (ESP32-C3 VOH min, 0.8 × 3.25 V).
- **Result**:
  - With the drafted 68 Ω: peak **43.7 mA, which fails**.
  - With **82 Ω**: peak 37.2 mA, mean coil power at most 11.4 mW (coil current sensed in series
    with the inductance, so the off-time recirculation through D1 is counted).
  - The owner chose 82 Ω (MCDA 440/500, against 380 for keeping 68 Ω on a coil-power criterion
    and 340 for 75 Ω; ADR-0015).
  - Expected cost: about −1.7 dB, which is consistent with the "never loud" constraint.
  - Plot: `results/buzzer-driver.svg`.
  - **OPEN**: second engine.
- **Used in**: ADR-0015 D3; bom.md (82 Ω); electronics-spec.
- **Reproduce**: `sh run.sh` (fails on any `FAIL C-0003`; table in `results/buzzer-driver.txt`).
  Sensitivity checked: the 68 Ω corners print 10 `FAIL C-0003` lines.
- **Based on**: main at `dc773b1`

## C-0004 — 2026-10-16 — Flyback clamp and AO3400 stress

- **Purpose**: Tier 2. The drain spike must stay ≤ 10 V, well below the AO3400's 30 V VDS, and
  the MOSFET dissipation must be negligible.
- **Method**: The same bench and the same 64 corners as C-0003. Negative control: the 1N4148
  removed, at 4 mH and 50 % duty.
- **Inputs**: 1N4148 across the coil, using the public model in `models.lib`.
- **Result**:
  - With the diode: drain at most 4.12 V.
  - **Without the diode: 30.7 V**, the MOSFET in avalanche (printed `EXPECTED-FAIL`). The diode is
    therefore required.
  - Mean MOSFET power at most 0.04 mW, against 350 mW for a SOT-23.
- **Used in**: bom.md (1N4148, AO3400).
- **Reproduce**: `ngspice -b buzzer-driver.cir` (line `vdmax_nodiode`)
- **Based on**: main at `dc773b1`

## C-0005 — 2026-10-16 — AO3400 gate drive from the GPIO

- **Purpose**: Tier 3. Confirm the MOSFET is fully on from a 3.3 V GPIO and switches fast
  enough for the shortest pulse.
- **Method**:
  - Compare the GPIO high level with the gate threshold.
  - Gate time constant: Rg × Ciss.
- **Inputs**:
  - VOH min 0.8 × 3.25 V.
  - AO3400: Vth at most 1.45 V; RDS(on) ≤ 52 mΩ at VGS 2.5 V; Ciss about 750 pF upper bound.
  - Rg 100 Ω; pull-down 100 kΩ.
- **Result**:
  - Gate driven at 2.60 V, above the 2.5 V at which RDS(on) is specified.
  - τ = 75 ns, against a 10 µs pulse (2 % duty at 2 kHz).
  - Pull-down leakage 33 µA while the buzzer sounds.
  - The 2N7002 stays rejected: its RDS(on) is not specified below 4.5 V of gate drive.
- **Used in**: bom.md.
- **Reproduce**: `python3 calc.py`
- **Based on**: main at `dc773b1`

## C-0006 — 2026-10-16 — SSD1331 RES# held by an RC from 3V3

- **Purpose**: Tier 1 (reset correctness; the draft failed). Requirement from the SSD1331
  power-on sequence: RES# below VIL (0.2 × VDD) for at least 3 µs after VDD is stable.
- **Method**: Bench `oled-reset-rc.cir`, 24 corners:
  - 3V3 ramp of 0.1, 0.6 or 2 ms (TLV75733 soft-start plus USB plug-in, ASSUMED range);
  - R and C at ±10 %.
  - A replug is simulated after 20 ms off.
- **Inputs**:
  - Option A (draft): 10 kΩ / 100 nF.
  - Option B: 10 kΩ / 1 µF X7R with a 1N4148 from RES# (anode) to 3V3 (cathode).
- **Result**:
  - Option A fails for every ramp of 0.6 ms or more: RES# is 0.70–2.08 V once VDD is stable,
    against a 0.66 V limit.
  - **Option B passes at every corner**: RES# is at most 0.38 V once VDD is stable and stays low
    for at least 0.83 ms more. It is at most 0.40 V on a replug.
  - The display leaves reset about 16 ms after power-up, long before the firmware initialises
    SPI.
  - The owner chose option B (MCDA 470/500, against 395 for 100 kΩ / 100 nF, which a module's
    own pull-up on RES# would defeat, and 270 for keeping the draft; ADR-0015).
  - Plots: `results/oled-reset-rc-a.svg` and `results/oled-reset-rc-b.svg`.
  - **OPEN**:
    - second engine;
    - bench check of 10 cold boots;
    - check whether the chosen module already has a pull-up on RES#.
  - Assumption: power-off is an ideal source falling to 0 V, which sinks current, so the replug
    figure is optimistic; the cold-boot bench check covers a real rail.
- **Used in**: ADR-0015 D2; electronics-spec; bom.md.
- **Reproduce**: `ngspice -b oled-reset-rc.cir`
- **Based on**: main at `dc773b1`
- **Supersedes**: the RC in the former calculations.md §3, which was never logged.

## C-0007 — 2026-10-16 — I2C pull-ups and rise time at 400 kHz

- **Purpose**: Tier 1 (1.34× margin at 100 pF). Requirements from UM10204 Fast-mode: tr (30 to
  70 %) ≤ 300 ns, and sink current ≤ 3 mA at VOL 0.4 V.
- **Method**: Bench `i2c-bus.cir`.
  - Pull-up networks, each at −5 % and +5 %, all with the 47 kΩ board pull-up (C-0008):
    - 47 k alone (no module, P0/P1: the bus is unused);
    - 4.7 k ∥ 4.7 k (GY-521 and GY-302);
    - 2.2 k ∥ 4.7 k (GY-521 clones);
    - 2.2 k ∥ 4.7 k ∥ 4.7 k (plus an expansion module, issue #18).
  - Bus capacitance 50, 100 and 200 pF (ASSUMED); thresholds at 30 % and 70 % of the rail.
  - Rail 3.2 and 3.3 V (module LDOs, ASSUMED).
- **Result** (the bench prints `FAIL C-0007` on a broken limit up to 100 pF):
  - At 100 pF: rise time 87–200 ns, which passes (1.5× margin).
  - At 200 pF, for information: 174–399 ns; 4.7 k ∥ 4.7 k would fail.
  - Sink current 1.23–2.82 mA, which passes in every case, including three modules.
  - The Super Mini onboard LED on GPIO8 adds sink current on SDA; it is not counted: prototypes
    only.
  - Rule: keep the bus wiring ≤ 20 cm. If a bench measurement finds tr > 300 ns, fall back to
    100 kHz.
  - **OPEN**: second engine; bench measurement of the rise time.
- **Used in**: electronics-spec (I2C 400 kHz).
- **Reproduce**: `ngspice -b i2c-bus.cir`
- **Based on**: main at `dc773b1`

## C-0008 — 2026-10-16 — Strapping pin GPIO2 at reset with the buzzer stage

- **Purpose**: Tier 1 (boot correctness). Requirement from the ESP32-C3 datasheet boot-mode
  table: GPIO2 = 1 in both SPI boot and download boot, with no internal pull at reset;
  VIH = 0.75 × VDD = 2.51 V.
- **Method**: Bench `strapping-gpio2.cir`, a DC operating point with 50 nA of pin leakage
  (ASSUMED).
- **Inputs**:
  - Option A (draft): buzzer gate pull-down of 100 kΩ on GPIO2.
  - Option B0: SDA on GPIO2 with no module fitted, only the 47 kΩ (+5 %) board pull-up to 3.25 V.
  - Option B: SDA on GPIO2 with the modules, 2.36 kΩ (+5 %) to 3.2 V.
- **Result**:
  - Option A gives **0.005 V, which fails.**
  - Option B0 gives 3.25 V and option B 3.20 V, which both pass. The Super Mini's GPIO2 (mic
    BCLK) has the same 47 kΩ pull-up: case B0.
  - The 47 kΩ board pull-up came from the pre-push review (M2): without it a P0/P1 cube leaves
    GPIO2 floating. Owner: 485/500, against 10 kΩ (410) and making the IMU mandatory (285).
  - The owner chose to swap the buzzer and SDA (MCDA 430/500, against 410 for a pull-up on
    GPIO2, which turns the MOSFET on at reset and clicks the buzzer, and 340 for keeping the
    draft and testing on the bench; ADR-0015):
    - XIAO: buzzer GPIO6 (D4), SDA GPIO2 (D0);
    - Super Mini: buzzer GPIO0, SDA GPIO8, where download boot needs 1 and the onboard LED also
      pulls up.
  - **OPEN**: second engine; bench boot and download mode with no module and with every module
    fitted (bench-f5 #14); confirm the XIAO's own pull-up on GPIO8 for download boot.
- **Used in**: ADR-0015 D1; `firmware/include/pins.h`; electronics-spec GPIO contract.
- **Reproduce**: `ngspice -b strapping-gpio2.cir`
- **Based on**: main at `dc773b1`

## C-0009 — 2026-10-16 — INMP441 clock and supply window

- **Purpose**: Tier 3. Check the I2S settings and the supply against the INMP441 limits.
- **Method**: SCK = fs × 2 slots × 32 bits.
- **Inputs**:
  - fs 16 kHz (electronics-spec).
  - INMP441 datasheet: SCK 0.5–3.3 MHz, WS 7.8–50 kHz, VDD 1.62–3.63 V.
  - 3V3 at most 3.35 V.
- **Result**:
  - SCK 1.024 MHz and WS 16 kHz, both inside their windows.
  - VDD 3.35 V, below 3.63 V.
  - Note: the datasheet suggests a 100 kΩ pull-down on SD, for the half-frame when the
    microphone tri-states. It is not needed with a single left-channel microphone read in mono.
    Check whether the module fits one.
- **Used in**: electronics-spec §Interfaces.
- **Reproduce**: `python3 calc.py`
- **Based on**: main at `dc773b1`

## C-0010 — 2026-10-16 — TTP223 output into GPIO4

- **Purpose**: Tier 3. Check the touch module's supply and output level against the GPIO input.
- **Method**: Compare the levels.
- **Inputs**:
  - TTP223 VDD 2.0–5.5 V, push-pull output.
  - ESP32-C3 VIH 0.75 × VDD = 2.51 V.
- **Result**:
  - The output high level is about VDD − 0.2 V, above 2.51 V, so the level passes.
  - The module calibrates at power-up for about 0.5 s: the pad must not be touched then.
  - The sensitivity through the 1.6 mm PLA wall is measured on the printed prototype.
- **Used in**: electronics-spec.
- **Reproduce**: `python3 calc.py`
- **Based on**: main at `dc773b1`

## C-0011 — 2026-10-16 — SSD1331 SPI clock and frame time

- **Purpose**: Tier 3. Check the SPI clock against the SSD1331 limit and the frame-rate need.
- **Method**: Period = 1 / f; frame time = bits per frame / f.
- **Inputs**: SPI clock 6 MHz; SSD1331 minimum cycle 150 ns; 96 × 64 pixels at 16 bit.
- **Result**:
  - Period 167 ns, above the 150 ns minimum.
  - A raw frame takes 16.4 ms, which allows 25 fps (REQ-NF-02).
- **Used in**: electronics-spec §Interfaces.
- **Reproduce**: `python3 calc.py`
- **Based on**: main at `dc773b1`

## C-0012 — 2026-10-16 — Expansion port J1: current budget for one module (issue #18)

- **Purpose**: Tier 1 (2× margin on the module's share: cube + 2 × module ≤ the thermal
  limit; 231 / 156 = 1.48× on the total). Give the maximum current a
  Qwiic/STEMMA QT module plugged into J1 may draw from the XIAO 3V3 (TLV75733).
- **Method**:
  - Sustained: LDO junction rise ΔT = (VBUS max − 3.3) × I × θJA (C-0002) must keep
    Tj ≤ 125 °C on a 35 °C desk; the cube's typical draw (C-0001) plus twice the module's
    continuous draw must stay under that total.
  - Peak: the C-0001 peak plus the module's peak must stay ≤ 500 mA (REQ-NF-04).
- **Inputs**:
  - C-0001: typical 106 mA, peak 420.3 mA.
  - C-0002: VBUS max 5.25 V, θJA 200 °C/W (ASSUMED, still OPEN in C-0002).
  - Desk ambient 35 °C (ASSUMED); Tj max 125 °C (TLV757P recommended operating range, ASSUMED,
    to read with θJA).
- **Result**:
  - The LDO rises by 0.39 °C per mA; the cube may draw at most 231 mA sustained in total.
  - **Module budget: ≤ 50 mA continuous, ≤ 75 mA peak** (bursts under 1 s). At 50 mA
    continuous Tj reaches 96 °C; at the peak the cube draws 495 mA.
  - The owner validated 50 / 80 mA (MCDA, 2026-10-16); the calculation lowers the peak to 75 mA
    because C-0001's peak is 420.3 mA, so 80 mA would give 500.3 mA.
  - Consequence: a vibration module (ERM or LRA at 60–90 mA) fits only as short bursts, under
    the firmware's control. A module above the budget needs its own supply and is out of V1.
  - **OPEN**: the same θJA and Tj max reading as C-0002; LDO temperature measured at the bench
    with a 50 mA dummy load on J1 (CDR).
- **Used in**: ADR-0016; electronics-spec §Power; REQ-NF-12.
- **Reproduce**: `python3 calc.py`
- **Based on**: main at `fc6539b`

## C-0013 — 2026-10-16 — XIAO 3V3 LDO is a Torex XC6210B332MR: dissipation, headroom, J1 budget (issue #32)

- **Purpose**: Tier 1 (thermal margin). Close the θJA and dropout readings left open in C-0002
  and C-0012.
- **Method**:
  - Read the Seeed XIAO ESP32C3 schematic (2022-05-23): U2 is a **Torex XC6210B332MR (SOT-25)**,
    not a TI TLV75733. VUSB reaches it through F1 (6 V / 500 mA PTC) and D1 (MSK4005 Schottky).
  - Dissipation: P = (VBUS max − Vf min − VOUT min) × I; Tj = TA + P × θJA.
  - Headroom at the peak: VIN = VBUS min − (cable + PTC) × I − Vf max, against VOUT max + dropout.
- **Inputs**:
  - XC6210 datasheet ETR0317_007: VOUT 3.234–3.366 V; dropout 75 mV max at 100 mA (Ta 25 °C);
    IOUT max ≥ 700 mA; Pd (SOT-25, Ta 25 °C) 250 mW with no board, 600 mW on a 40 × 40 mm board,
    760 mW on a JESD51-7 board; Topr up to 85 °C.
  - No θJA is printed. Derived θJA = (125 − 25) / Pd, with Pd derated to zero at 125 °C (Torex
    convention, ESTIMATE): 132 / **167 (nominal)** / 400 °C/W.
  - Schottky Vf 0.30–0.55 V (ASSUMED, datasheet not read); PTC 0.5 Ω (ASSUMED); cable as C-0002.
  - Dropout at 420 mA scaled linearly from 75 mV at 100 mA: 0.32 V (ESTIMATE).
  - For the record, the TLV757P values C-0002 assumed (datasheet SBVS322C): θJA 231.1 °C/W
    (JEDEC, DBV) and dropout ≤ 475 mV at 1 A. They do not apply to this board.
- **Result**:
  - LDO drop 1.72 V worst case; at 106 mA typical: 182 mW, +24 / +30 / +73 °C over ambient.
  - Headroom at the 420 mA peak: LDO input ≥ 3.89 V, 0.21 V over 3.366 V plus dropout. Passes.
  - C-0012 recomputed: the cube may draw 315 mA sustained in total at the nominal θJA (399 at
    132 °C/W, 131 at 400 °C/W). A 50 mA module gives Tj 80 °C nominal, so the 50 / 75 mA budget
    and its 2× margin **hold at the nominal θJA but not at the 400 °C/W bound** (Tj 142 °C).
  - The owner chose to keep the budget provisional and close it on the bench (MCDA 415/500,
    against 370 for adopting 167 °C/W outright and 315 for the 400 °C/W bound with a ~15 mA
    module budget superseding ADR-0016).
  - The 500 mA PTC sits next to the 495 mA peak of C-0012; the bursts last under 1 ms, far
    shorter than a PTC trip time, so it is noted, not a limit.
  - **OPEN** (CDR exit criteria): LDO case temperature on the bench with a 50 mA dummy load on J1
    (θJA from the measurement, then the budget is confirmed or a superseding ADR lowers it);
    Schottky Vf from its datasheet.
- **Used in**: electronics-spec §Power; ADR-0016 budget (unchanged, now provisional); REQ-NF-12.
- **Reproduce**: `python3 calc.py` (lines C-0013 and C-0012)
- **Based on**: main at `bd2ee79`
- **Supersedes**: C-0002 (wrong part: the TLV75733 is not on the XIAO); the thermal inputs of
  C-0012.

## C-0014 — 2026-10-16 — GPIO8 has no pull-up on the XIAO: 10 kΩ board pull-up R6 (issue #31)

- **Purpose**: Tier 1 (boot correctness). GPIO8 must read 1 in download boot (GPIO9 = 0); on the
  XIAO it carries the OLED SCK.
- **Method**:
  - Read the Seeed XIAO ESP32C3 schematic: ESP32-C3 pin 14 (GPIO8) goes straight to D8. The only
    pull-ups are on EN and GPIO9 (BOOT, 10 kΩ). There is **no pull-up on GPIO8**.
  - The SSD1331 SCK input is high impedance, so GPIO8 floats at reset: download boot is not
    guaranteed.
  - Bench `strapping-gpio2.cir`, new case G8 and a negative control without R6.
- **Inputs**:
  - R6 10 kΩ +5 % to 3.234 V (XC6210 minimum, C-0013).
  - Leakage: pin 50 nA plus the SSD1331 SCK input 1 µA (ASSUMED), 1.05 µA in total.
  - VIH = 0.75 × 3.366 V = 2.52 V (the B0 and B cases of C-0008 are rerun at the same rail and
    threshold).
- **Result**:
  - G8 with R6: 3.22 V, which passes. Without R6 the bench prints `EXPECTED-FAIL`.
  - B0 3.23 V and B 3.20 V still pass at the new rail.
  - Load while SCK is driven low: 0.33 mA, negligible against the GPIO drive; SPI edges are
    push-pull, so the pull-up does not slow them.
  - The owner chose the pull-up (MCDA 435/500, against 380 for relying on the USB-Serial-JTAG
    forced download and 360 for leaving it to the bench).
  - **OPEN**: bench boot and download mode with every module fitted (bench-f5 #14), unchanged.
- **Used in**: schematic R6; bom.md; assembly.md; electronics-spec strapping notes.
- **Reproduce**: `ngspice -b strapping-gpio2.cir`
- **Based on**: main at `bd2ee79`
- **Supersedes**: the C-0008 open item "confirm the XIAO's own pull-up on GPIO8".

## C-0015 — 2026-10-16 — Second method for the Tier 1 benches (issue #30)

- **Purpose**: Tier 1 needs a second, independent method. SPICE alternatives were
  weighed: LTspice is not installed and Xyce or Qucs-S are not packaged here.
- **Method**: `crosscheck.py` recomputes every corner row of the ngspice result tables from the
  circuit equations, with no SPICE:
  - C-0007 I2C: t(30→70 %) = Rp × Cb × ln(7/3); sink current (Vp − 0.4) / Rp. Tolerance 2 %.
  - C-0006 reset RC: RC response to a linear ramp, then the exponential to 3.3 V; RES# at VDD
    stable and the time it stays under 0.2 × VDD. Tolerance 2 % (5 µs on the time).
  - C-0003 buzzer: piecewise-exact periodic steady state of the RL loop (on through Rs + Rcoil +
    Rds(on) 50 mΩ, off through Rcoil and a constant 0.65 V diode). Tolerance 6 %.
  - C-0008 / C-0014 strapping: DC divider. Tolerance 0.1 %.
- **Inputs**: the bench tables in `results/`; the same nominal values and corners.
- **Result**:
  - **All 139 rows agree**: 48 I2C, 24 reset RC, 64 buzzer corners and 3 strapping cases.
  - Sensitivity checked: a wrong diode drop (0 V) and a wrong rise-time constant (ln 2.5) print
    60 `FAIL C-0015` lines.
  - The owner chose this route (MCDA 410/500, against 360 for Xyce from a vendor binary, 360 for
    waiving the second engine and 310 for LTspice under Wine).
  - C-0004 (flyback clamp) stays Tier 2 and is not cross-checked.
- **Used in**: verification-plan; calculations README status.
- **Reproduce**: `sh run.sh` (fails on any `FAIL C-0015`; table in `results/crosscheck.txt`)
- **Based on**: main at `bd2ee79`
- **Supersedes**: the "second engine" open items of C-0003, C-0006, C-0007 and C-0008.
