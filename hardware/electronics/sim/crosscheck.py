"""Second method for the Tier 1 benches (C-0015, issue #30): closed-form and piecewise-exact models.

Each Tier 1 ngspice result is recomputed here without SPICE, from the circuit equations, and the
two engines must agree on every corner row of results/*.txt. A disagreement prints
`FAIL C-0015` (run.sh fails on it). The models are idealised on purpose, so each check states its
tolerance and the effect it ignores.

    python3 crosscheck.py   (from this folder, after the benches; run.sh calls it)
"""

from __future__ import annotations

import math
import pathlib
import re

RESULTS = pathlib.Path(__file__).resolve().parent / "results"
SI = {"n": 1e-9, "p": 1e-12, "u": 1e-6, "m": 1e-3, "k": 1e3}
failures = 0


def num(text: str) -> float:
    """'0.6m' -> 0.0006, '100p' -> 1e-10, '9E-08' -> 9e-08 (ngspice table cells)."""
    m = re.fullmatch(r"([-+\d.eE]+)([npumk]?)", text)
    if not m:
        raise ValueError(f"unreadable cell: {text!r}")
    return float(m.group(1)) * SI.get(m.group(2), 1.0)


def rows(name: str, width: int) -> list[list[float]]:
    """Numeric rows of a bench table: the cells of '<inputs> | <out> | <out>' lines, header skipped."""
    out = []
    for line in (RESULTS / name).read_text().splitlines():
        cells = line.replace("|", " ").split()
        if "|" in line and len(cells) == width and re.match(r"[\d.]", cells[0]):
            out.append([num(c) for c in cells])
    if not out:
        raise SystemExit(f"no rows in {name}: run the benches first")
    return out


def agree(label: str, spice: float, ref: float, rel: float, abs_: float = 0.0) -> None:
    global failures
    if abs(spice - ref) > max(rel * abs(ref), abs_):
        failures += 1
        print(f"FAIL C-0015 {label}: ngspice {spice:.4g} vs closed form {ref:.4g}")


# C-0007 I2C: single-pole RC rise, t(30->70 %) = Rp Cb ln(7/3); sink at VOL 0.4 V = (Vp - 0.4) / Rp.
# Ignores the driver's off time (1 ns) and the timestep: 2 % tolerance.
i2c = rows("i2c-bus.txt", 5)
for rp, vp, cb, tr, isink in i2c:
    agree(f"C-0007 tr {rp:.0f} {cb:g}", tr * 1e-9, rp * cb * math.log(7 / 3), 0.02)
    agree(f"C-0007 Isink {rp:.0f} {vp}", isink * 1e-3, (vp - 0.4) / rp, 0.02)
print(f"C-0015 C-0007 I2C: {len(i2c)} corners agree")

# C-0006 RES# RC under a linear 3V3 ramp to 3.3 V in tr. While the rail ramps, RES# lags it and the
# diode stays off, so both options follow v(t) = S (t - tau (1 - e^(-t/tau))), S = 3.3 / tr.
# After tr: exponential towards 3.3 V; low time = until v reaches 0.2 x 3.3 V (negative when it
# never was low). Ignores the diode leakage: 2 % on the level, 5 us on the time.
rc = rows("oled-reset-rc.txt", 7)
for _opt, tr, r, c, v_st, low_ms, _v_rep in rc:
    tau = r * c
    v_t = 3.3 / tr * (tr - tau * (1 - math.exp(-tr / tau)))
    agree(f"C-0006 V(RES) tr {tr:g} R {r:g} C {c:g}", v_st, v_t, 0.02, 2e-3)
    t_low = -tau * math.log((3.3 - 0.66) / (3.3 - v_t)) if v_t < 0.66 else None
    if t_low is not None:
        agree(
            f"C-0006 low time tr {tr:g} R {r:g} C {c:g}",
            low_ms * 1e-3,
            t_low,
            0.02,
            5e-6,
        )
print(f"C-0015 C-0006 reset RC: {len(rc)} corners agree")

# C-0003 buzzer coil peak current: piecewise-exact periodic steady state of the RL loop.
# On: VDD across Rs + Rcoil + Rds(on); off: the coil recirculates through Rcoil and the 1N4148
# (constant 0.65 V drop). Ignores the diode's exponential knee and the gate edges: 6 %.
RDS, VD, F = 0.05, 0.65, 2000.0
bz = rows("buzzer-driver.txt", 9)
for vdd, rs, rcoil, lc, duty, ipk, *_ in bz:
    r_on, t_on, t_off = rs + rcoil + RDS, duty / F, (1 - duty) / F
    i_inf, a_on, a_off = (
        vdd / r_on,
        math.exp(-r_on * t_on / lc),
        math.exp(-rcoil * t_off / lc),
    )
    i0 = 0.0
    for _ in range(2000):  # iterate cycles to the periodic steady state
        i1 = i_inf + (i0 - i_inf) * a_on
        # off: di/dt = -(Rcoil i + VD) / L until the current reaches zero
        i0 = max(0.0, (i1 + VD / rcoil) * a_off - VD / rcoil)
    agree(
        f"C-0003 Ipk {vdd} {rs:.1f} {rcoil:g} {lc:g} {duty}",
        ipk,
        i_inf + (i0 - i_inf) * a_on,
        0.06,
    )
print(f"C-0015 C-0003 buzzer: {len(bz)} corners agree")

# C-0008 / C-0014 strapping: DC divider, V = Vrail - R x I(leak). Exact: 0.1 %.
strap = {
    m.group(1): float(m.group(2))
    for m in re.finditer(
        r"^(\w+):\s+V\(GPIO\d\) = ([\d.eE+-]+)",
        (RESULTS / "strapping-gpio2.txt").read_text(),
        re.M,
    )
}
r5, r6 = 47e3, 10e3
agree("C-0008 B0", strap["B0"], 3.234 - r5 * 1.05 * 50e-9, 1e-3)
agree("C-0008 B", strap["B"], 3.2 - 1.05 / (1 / r5 + 2 / 4.7e3) * 50e-9, 1e-3)
agree("C-0014 G8", strap["G8"], 3.234 - r6 * 1.05 * 1.05e-6, 1e-3)
print("C-0015 C-0008/C-0014 strapping: 3 cases agree")

print(
    "C-0015 second method: all agree"
    if failures == 0
    else f"C-0015 second method: {failures} FAIL"
)
raise SystemExit(1 if failures else 0)
