"""Tier-3 calculation lines for cubbies V1 (calculation log C-0001, C-0005, C-0009..C-0013; C-0012 thermal inputs from C-0013).

Plain arithmetic over worst-case corners; every input carries its source or ASSUMED.
Run: python3 calc.py   (prints every figure the log quotes)
"""

# C-0001 current budget on 3V3 (mA): (typical, peak)
BUDGET = {
    "ESP32-C3 (RX / TX 802.11b, datasheet v2.4 t.5-7)": (84, 335),
    "SSD1331 module (Waveshare wiki, black / white)": (15, 40),
    "INMP441 (datasheet, 3.3 V)": (1.4, 1.4),
    "GY-521 MPU6050 + module LDO + LED (ASSUMED 1 mA LED)": (5, 5),
    "GY-302 BH1750 (datasheet 0.19 mA max + LDO)": (0.3, 0.3),
    "TTP223 module incl. LED when touched (ASSUMED 1.3 mA)": (0.1, 1.4),
    "buzzer stage (buzzer-driver.cir, 82 ohm, peak 37.4 mA at 3.366 V, C-0013)": (0, 37.4),
}
typ = sum(v[0] for v in BUDGET.values())
pk = sum(v[1] for v in BUDGET.values())
assert pk <= 500, "C-0001 peak over 500 mA"
print(f"C-0001 budget: typical {typ:.1f} mA, peak {pk:.1f} mA (limit 500 mA, REQ-NF-04)")

# C-0013 (supersedes C-0002) XIAO 3V3 LDO = Torex XC6210B332MR, SOT-25 (Seeed XIAO ESP32C3 schematic),
# fed from VBUS through a 500 mA PTC fuse and a MSK4005 Schottky. XC6210 ETR0317_007: VOUT 3.234-3.366 V,
# dropout 75 mV max at 100 mA, Pd SOT-25 250 / 600 / 760 mW at 25 C (no board / 40x40 mm / JESD51-7).
# theta_JA derived as (125 - 25) / Pd, Pd derated to zero at 125 C (Torex convention, ESTIMATE): the
# 40x40 mm figure is nominal, the range bounds it until the bench measures Tj (owner decision C, 415/500).
VBUS = (4.75, 5.25)  # USB 2.0 VBUS at the source
VOUT = (3.234, 3.366)  # XC6210B332 output window
VF = (0.30, 0.55)  # MSK4005 forward drop, 100-500 mA (ASSUMED, datasheet not read)
R_PTC = 0.5  # PTC fuse resistance, ohm (ASSUMED)
TJ_MAX = 125
THETA = {"JESD51-7": (TJ_MAX - 25) / 0.760, "40x40 mm (nominal)": (TJ_MAX - 25) / 0.600, "no board": (TJ_MAX - 25) / 0.250}
dv_ldo = VBUS[1] - VF[0] - VOUT[0]  # worst-case drop across the LDO (the PTC drop is ignored: conservative)
for name, theta in THETA.items():
    print(f"C-0013 LDO theta_JA {theta:.0f} C/W ({name}): {dv_ldo * typ:.0f} mW at {typ:.0f} mA,"
          f" +{dv_ldo * typ * theta / 1000:.0f} C over ambient")
# headroom at the peak: cable 2 x 0.115 ohm (AWG28 ASSUMED) + PTC + Schottky; dropout scaled linearly
# from 75 mV at 100 mA, at 25 C (ESTIMATE: a PMOS dropout rises when hot; the headroom absorbs ~30 %)
vin_min = VBUS[0] - (0.23 + R_PTC) * pk / 1000 - VF[1]
vdo = 0.075 * pk / 100
assert vin_min - VOUT[1] - vdo > 0, "C-0013 no LDO headroom"
print(f"C-0013 LDO input min {vin_min:.2f} V at {pk:.0f} mA, headroom over {VOUT[1]} + {vdo:.2f} V dropout:"
      f" {vin_min - VOUT[1] - vdo:.2f} V")

# C-0005 AO3400 gate drive
voh_min = 0.8 * VOUT[0]  # XC6210B332 minimum rail (C-0013)
assert voh_min >= 2.5, "C-0005 gate below the RDS(on) spec point"
print(f"C-0005 gate drive VOH min {voh_min:.2f} V vs Vth max 1.45 V; Rds(on) spec at 2.5 V <= 52 mohm;"
      f" gate tau 100 ohm x 750 pF = {100 * 750e-12 * 1e9:.0f} ns vs 10 us (2 % of 2 kHz)")

# C-0009 INMP441 clocking and supply
fs, slot_bits = 16000, 32
sck = fs * 2 * slot_bits
assert 0.5e6 <= sck <= 3.3e6 and 7.8e3 <= fs <= 50e3 and VOUT[1] <= 3.63, "C-0009"
print(f"C-0009 INMP441 SCK {sck / 1e6:.3f} MHz (window 0.5-3.3 MHz), WS {fs / 1e3:.0f} kHz (7.8-50 kHz);"
      f" VDD max {VOUT[1]:.3f} V vs 3.63 V")

# C-0010 TTP223 output into GPIO4: VOH vs ESP32-C3 VIH 0.75 x VDD
print(f"C-0010 TTP223 VDD 2.0-5.5 V ok at {VOUT[0]}-{VOUT[1]} V; VIH {0.75 * VOUT[1]:.3f} V; push-pull output, VOH ~VDD - 0.2 V")

# C-0011 SPI 6 MHz to the SSD1331: t_cycle >= 150 ns
assert 1e9 / 6e6 >= 150 and 96 * 64 * 16 / 6e6 <= 0.040, "C-0011"
print(f"C-0011 SPI period at 6 MHz {1e9 / 6e6:.0f} ns >= 150 ns; frame 96x64x16 = {96 * 64 * 16 / 6e6 * 1e3:.1f} ms")

# C-0012 expansion port J1 (issue #18), thermal inputs from C-0013: current a Qwiic module may draw
# from the XIAO 3V3. Sustained: Tj = TA + dv_ldo x I x theta_JA <= 125 C at a 35 C desk (ASSUMED),
# with the tier-1 2x margin on the module's share. Peak: C-0001 peak + module peak <= 500 mA (REQ-NF-04).
# The budget asserts on the nominal theta; the no-board bound is printed and closed by the bench (CDR).
TA = 35
mod_cont, mod_peak = 50, 75  # 80 overshoots by 0.3 mA (peak 420.3 mA)
assert pk + mod_peak <= 500, "C-0012 module peak over 500 mA"
for name, theta in THETA.items():
    k = dv_ldo * theta / 1000  # C per mA
    i_sust_max = (TJ_MAX - TA) / k
    ok = typ + 2 * mod_cont <= i_sust_max
    if name.endswith("(nominal)"):
        assert ok, "C-0012 module continuous budget over the thermal limit"
    print(f"C-0012 J1 module, theta {theta:.0f} C/W ({name}): LDO {k:.2f} C/mA, sustained total max"
          f" {i_sust_max:.0f} mA; module {mod_cont} mA continuous gives Tj {TA + k * (typ + mod_cont):.0f} C,"
          f" 2x margin {'PASS' if ok else 'NOT MET (bench to close)'}")
print(f"C-0012 J1 module peak {mod_peak} mA: total {pk + mod_peak:.0f} mA")
