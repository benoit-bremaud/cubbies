"""Tier-3 calculation lines for cubbies V1 (calculation log C-0001, C-0002, C-0005, C-0009..C-0012).

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
    "buzzer stage (buzzer-driver.cir, 82 ohm, peak 37.2 mA)": (0, 37.2),
}
typ = sum(v[0] for v in BUDGET.values())
pk = sum(v[1] for v in BUDGET.values())
assert pk <= 500, "C-0001 peak over 500 mA"
print(f"C-0001 budget: typical {typ:.1f} mA, peak {pk:.1f} mA (limit 500 mA, REQ-NF-04)")

# C-0002 LDO TLV75733 dissipation (the 420 mA peak is a burst: ESP-NOW TX < 1 ms, not sustained) and junction rise; theta_JA ASSUMED 200 C/W (SOT-23-5, to read)
VBUS = (4.75, 5.25)  # USB 2.0 VBUS at the source
for i in (typ, 300, pk):
    p = (VBUS[1] - 3.3) * i / 1000
    print(f"C-0002 LDO at {i:.0f} mA: {p * 1000:.0f} mW, +{p * 200:.0f} C over ambient")
# headroom: 1 m cable, 2 x 0.115 ohm (AWG28 ASSUMED), dropout 0.43 V max at 500 mA (ASSUMED, to read)
vin_min = VBUS[0] - 0.23 * pk / 1000
assert vin_min - 3.73 > 0, "C-0002 no LDO headroom"
print(f"C-0002 LDO input min {vin_min:.2f} V, headroom over 3.3 + 0.43 V: {vin_min - 3.73:.2f} V")

# C-0005 AO3400 gate drive
voh_min = 0.8 * 3.25
assert voh_min >= 2.5, "C-0005 gate below the RDS(on) spec point"
print(f"C-0005 gate drive VOH min {voh_min:.2f} V vs Vth max 1.45 V; Rds(on) spec at 2.5 V <= 52 mohm;"
      f" gate tau 100 ohm x 750 pF = {100 * 750e-12 * 1e9:.0f} ns vs 10 us (2 % of 2 kHz)")

# C-0009 INMP441 clocking and supply
fs, slot_bits = 16000, 32
sck = fs * 2 * slot_bits
assert 0.5e6 <= sck <= 3.3e6 and 7.8e3 <= fs <= 50e3 and 3.3 * 1.015 <= 3.63, "C-0009"
print(f"C-0009 INMP441 SCK {sck / 1e6:.3f} MHz (window 0.5-3.3 MHz), WS {fs / 1e3:.0f} kHz (7.8-50 kHz);"
      f" VDD max {3.3 * 1.015:.3f} V vs 3.63 V")

# C-0010 TTP223 output into GPIO4: VOH vs ESP32-C3 VIH 0.75 x VDD
print(f"C-0010 TTP223 VDD 2.0-5.5 V ok at 3.25-3.35 V; VIH {0.75 * 3.35:.2f} V; push-pull output, VOH ~VDD - 0.2 V")

# C-0011 SPI 6 MHz to the SSD1331: t_cycle >= 150 ns
assert 1e9 / 6e6 >= 150 and 96 * 64 * 16 / 6e6 <= 0.040, "C-0011"
print(f"C-0011 SPI period at 6 MHz {1e9 / 6e6:.0f} ns >= 150 ns; frame 96x64x16 = {96 * 64 * 16 / 6e6 * 1e3:.1f} ms")

# C-0012 expansion port J1 (issue #18): current a Qwiic module may draw from the XIAO 3V3
# Sustained: LDO junction rise (VBUS max - 3.3) x I x theta_JA must keep Tj <= 125 C at a 35 C desk
# (ambient ASSUMED; Tj max TLV757P recommended operating, to read), with the tier-1 2x margin on the
# module's share. Peak: C-0001 peak + module peak <= 500 mA (REQ-NF-04).
TA, TJ_MAX, THETA = 35, 125, 200
k = (VBUS[1] - 3.3) * THETA / 1000  # C per mA
i_sust_max = (TJ_MAX - TA) / k
mod_cont, mod_peak = 50, 75  # 80 overshoots by 0.3 mA (peak 420.3 mA)
assert typ + 2 * mod_cont <= i_sust_max, "C-0012 module continuous budget over the thermal limit"
assert pk + mod_peak <= 500, "C-0012 module peak over 500 mA"
print(f"C-0012 J1 module: LDO {k:.2f} C/mA, sustained total max {i_sust_max:.0f} mA, cube {typ:.0f} mA;"
      f" module <= {mod_cont} mA continuous (Tj {TA + k * (typ + mod_cont):.0f} C), <= {mod_peak} mA peak"
      f" (total {pk + mod_peak:.0f} mA)")
