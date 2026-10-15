# Calculations — cubbies V1

Every value here is reviewed in a pull request and approved at the CDR. Formulas are shown so a
reviewer can recompute. Status per block: `draft` → `simulated` → `measured` → `approved`.

## 1. Current budget on USB-C (REQ-NF-04) — status: draft

| Consumer | Typical | Peak | Source |
|----------|---------|------|--------|
| ESP32-C3, Wi-Fi radio for ESP-NOW | 80 mA (RX 84 mA) | 335 mA (TX 802.11b 1 Mbps, the ESP-NOW default rate) | ESP32-C3 datasheet v2.4 table 5-7 |
| SSD1331 OLED, eyes on black | 15 mA (estimate; Waveshare measures ~4 mA black, ~38 mA white) | 40 mA (full white) | Waveshare 0.95" RGB OLED wiki |
| INMP441 | 2.2 mA (at 3.3 V; 1.4 mA is the 1.8 V figure) | 2.5 mA | datasheet |
| MPU6050 | 4 mA | 4 mA | datasheet |
| BH1750 | 0.2 mA | 0.2 mA | datasheet |
| TTP223 | 0.1 mA | 0.1 mA | datasheet |
| Buzzer stage | 0 | 39 mA (instantaneous while sounding, §2) | calculation §2 |
| **Total** | **≈ 100 mA** | **≈ 420 mA** | within 500 mA |

LDO: the XIAO v1.3 schematic uses a TI TLV75733 (SOT-23-5, annotated 700 mA; the wiki table says
500 mA). Every milliamp above passes through it from 5 V: 100 mA → 0.17 W, 300 mA sustained →
0.51 W, 420 mA burst → 0.71 W. With θJA ≈ 150–200 °C/W (to confirm in the TLV757P datasheet) a
sustained 300 mA means +80–100 °C: the REQ-NF-04 "typical" ceiling is a thermal limit, not a
rating. CDR check: measure the LDO temperature at the real typical current.

## 2. Buzzer drive stage (REQ-F-04, REQ-NF-03) — status: draft

- Passive buzzer, nominal 3 V, coil 16 ± 4 Ω, **rated current 40 mA** (TRU Components class part;
  the exact SKU is chosen at the CDR). 3 / 16 ≈ 190 mA if driven hard: far above the rating and
  above any GPIO (ESP32-C3: 40 mA source / 28 mA sink only at maximum drive strength; the default
  is 20 mA, and 10 mA on GPIO2, the buzzer pin).
- Target ≤ 55 dB(A) at 1 m at maximum volume: series resistor R_s with the coil, N-channel MOSFET
  **AO3400** low side (VGS(th) 0.65–1.45 V, RDS(on) < 52 mΩ at 2.5 V: fully on from a 3.3 V
  GPIO; the 2N7002 is not specified below 4.5 V gate drive and is rejected), flyback diode 1N4148
  across the buzzer, 100 Ω gate resistor, 100 kΩ gate pull-down.
- With R_s = 68 Ω: I ≈ 3.3 / (68 + 16) ≈ 39 mA peak, within the 40 mA rating at any duty; the
  average at 50 % duty ≈ 20 mA.
- Volume levels: PWM duty 2 %, 8 %, 20 %, 50 % (V1 uses one constant, default 8 %; the four
  steps wait for a gesture); the dB mapping is measured, not computed.
- To simulate: `electronics/sim/buzzer-driver.cir`, transient 2 kHz, 4 duty values;
  checks: peak current, MOSFET dissipation, flyback spike ≤ 10 V.

## 3. Display RESET RC (ADR-0007 pin budget) — status: draft

- Goal: hold RESET low ≥ 1 ms after 3V3 rises (SSD1331 needs a reset pulse ≥ 3 µs; 1 ms gives
  margin against the LDO rise time), then high.
- R = 10 kΩ, C = 100 nF → τ = 1 ms; RESET crosses the VIH threshold (~0.8 × 3.3 V) at ≈ 1.6 τ ≈
  1.6 ms.
- VIL max = 0.2 × VDD is left at ≈ 0.22 τ ≈ 0.22 ms: the guaranteed low window is ≈ 0.22 ms,
  ≈ 70× the 3 µs requirement. Hand calculation is enough, no simulation; verified on the bench by
  10 cold boots (verification plan). Caveat: the datasheet wants RES# low after VDD is stable,
  which an RC tracking the LDO ramp satisfies by construction.

## 4. Touch pad through the enclosure — status: draft

- TTP223 sensitivity with the 1.6 mm PLA wall of the mechanical spec: capacitor C_s on the module (0–50 pF) lowers the
  sensitivity; with no C_s the module is at its most sensitive (datasheet); the usable wall thickness is not
  specified and is measured on the printed prototype. Start with no C_s and a
  copper tape pad of 15 × 15 mm glued under the top face; verify on the printed prototype.

## 5. Reserved: V2 battery — status: not started

- Autonomy = capacity / average current; with 100 mA awake and 1000 mAh: ≈ 10 h awake; sleep
  strategy needed. Battery divider for gauge: 200 kΩ / 200 kΩ into an ADC pin requires freeing a
  GPIO or using the XIAO's BAT path (to study).
