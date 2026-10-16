"""Second method for the Tier 1 benches (C-0015, issue #30): closed-form and piecewise-exact models.

Each Tier 1 ngspice result is recomputed here without SPICE, from the circuit equations, and the
two engines must agree on every corner row of results/*.txt. A disagreement prints
`FAIL C-0015` (run.sh fails on it). Checked columns: C-0007 rise time and sink current; C-0006
RES# level at VDD stable and low time (the replug column is not checked); C-0003 peak coil
current; C-0008 / C-0014 strapping levels. Tolerances are set at about 5x the deviation observed
on the adopted corners, so a modelling error of a few percent fails; each states what it ignores.

    python3 crosscheck.py   (from this folder, after the benches; run.sh calls it)
"""

from __future__ import annotations

import math
import pathlib
import re

RESULTS = pathlib.Path(__file__).resolve().parent / "results"
SI = {"n": 1e-9, "p": 1e-12, "u": 1e-6, "m": 1e-3, "k": 1e3}


def num(text: str) -> float:
    """'0.6m' -> 0.0006, '100p' -> 1e-10, '9E-08' -> 9e-08 (ngspice table cells)."""
    scale = SI.get(text[-1:], 1.0)
    body = text[:-1] if text[-1:] in SI else text
    try:
        value = float(body)
    except ValueError:
        raise ValueError(f"unreadable cell: {text!r}") from None
    if not math.isfinite(value):
        raise ValueError(f"unreadable cell: {text!r}")
    return value * scale


def rows(text: str, width: int, expect: int) -> list[list[float]]:
    """Numeric rows of a bench table ('<inputs> | <out> | <out>' lines, header skipped).

    Raises ValueError when the count differs from `expect`, so a row lost by the run.sh filter cannot pass.
    """
    out = []
    for line in text.splitlines():
        cells = line.replace("|", " ").split()
        if "|" in line and len(cells) == width and re.match(r"[-\d.]", cells[0]):
            out.append([num(c) for c in cells])
    if len(out) != expect:
        raise ValueError(f"expected {expect} rows, found {len(out)}: rerun the benches")
    return out


class Checker:
    """Collects the disagreements between ngspice and the closed form."""

    def __init__(self) -> None:
        self.failures = 0

    def agree(
        self, label: str, spice: float, ref: float, rel: float, abs_: float = 0.0
    ) -> None:
        """FAIL when |spice - ref| > max(rel x |ref|, abs_); abs_ is the floor for refs near 0."""
        if abs(spice - ref) > max(rel * abs(ref), abs_):
            self.failures += 1
            print(f"FAIL C-0015 {label}: ngspice {spice:.4g} vs closed form {ref:.4g}")


def check_i2c(c: Checker, table: str, expect: int = 48) -> int:
    """C-0007: single-pole RC rise t(30->70 %) = Rp Cb ln(7/3); sink (Vp - 0.4) / Rp.

    Ignores the driver's 1 ns off time and the timestep: 0.1 % (observed 0.004 %).
    """
    data = rows(table, 5, expect)
    for rp, vp, cb, tr, isink in data:
        c.agree(
            f"C-0007 tr {rp:.0f} {cb:g}", tr * 1e-9, rp * cb * math.log(7 / 3), 1e-3
        )
        c.agree(f"C-0007 Isink {rp:.0f} {vp}", isink * 1e-3, (vp - 0.4) / rp, 1e-3)
    return len(data)


def check_reset_rc(c: Checker, table: str) -> int:
    """C-0006: RES# RC under a linear 3V3 ramp to 3.3 V in tr.

    While the rail ramps RES# lags it and the diode stays off, so both options follow
    v(t) = S (t - tau (1 - e^(-t/tau))), S = 3.3 / tr; after tr, an exponential towards 3.3 V.
    Low time = until v reaches 0.2 x 3.3 V, and ngspice reports it negative when RES# was never
    low. Ignores the diode leakage: 0.5 % on the level (observed 0.06 %), 0.5 % or 2 us on the time.
    """
    data = rows(table, 7, 24)
    for _opt, tr, r, cap, v_st, low_ms, _v_rep in data:
        tau = r * cap
        v_t = 3.3 / tr * (tr - tau * (1 - math.exp(-tr / tau)))
        label = f"tr {tr:g} R {r:g} C {cap:g}"
        c.agree(f"C-0006 V(RES) {label}", v_st, v_t, 5e-3, 1e-3)
        if v_t < 0.66:
            t_low = -tau * math.log((3.3 - 0.66) / (3.3 - v_t))
            c.agree(f"C-0006 low time {label}", low_ms * 1e-3, t_low, 5e-3, 2e-6)
        elif low_ms > 0:
            c.agree(f"C-0006 never low {label}", low_ms, 0.0, 0.0)
    return len(data)


def check_buzzer(c: Checker, table: str) -> int:
    """C-0003 peak coil current: piecewise-exact periodic steady state of the RL loop.

    On: VDD across Rs + Rcoil + Rds(on) 50 mohm; off: the coil recirculates through Rcoil and the
    1N4148 at a constant 0.65 V. Ignores the diode's knee and the gate edges: 1 % (observed 0.18 %).
    """
    rds, vd, freq = 0.05, 0.65, 2000.0
    data = rows(table, 9, 64)
    for vdd, rs, rcoil, lc, duty, ipk, *_ in data:
        r_on, t_on, t_off = rs + rcoil + rds, duty / freq, (1 - duty) / freq
        i_inf = vdd / r_on
        a_on, a_off = math.exp(-r_on * t_on / lc), math.exp(-rcoil * t_off / lc)
        i0 = 0.0
        for _ in range(2000):  # iterate cycles to the periodic steady state
            i1 = i_inf + (i0 - i_inf) * a_on
            # off: di/dt = -(Rcoil i + VD) / L until the current reaches zero
            i0 = max(0.0, (i1 + vd / rcoil) * a_off - vd / rcoil)
        ref = i_inf + (i0 - i_inf) * a_on
        c.agree(f"C-0003 Ipk {vdd} {rs:.1f} {rcoil:g} {lc:g} {duty}", ipk, ref, 0.01)
    return len(data)


def check_strapping(c: Checker, table: str) -> int:
    """C-0008 / C-0014: DC divider V = Vrail - R x I(leak), 0.1 %.

    Same equation as the bench, so it guards the netlist wiring, not the model. The constants
    mirror strapping-gpio2.cir and schematic.lib (R5 47 k, R6 10 k): update both together.
    """
    level = {
        m.group(1): float(m.group(2))
        for m in re.finditer(r"^(\w+):\s+V\(GPIO\d\) = ([\d.eE+-]+)", table, re.M)
    }
    r5, r6 = 47e3, 10e3
    c.agree("C-0008 B0", level["B0"], 3.234 - r5 * 1.05 * 50e-9, 1e-3)
    c.agree("C-0008 B", level["B"], 3.2 - 1.05 / (1 / r5 + 2 / 4.7e3) * 50e-9, 1e-3)
    c.agree("C-0014 G8", level["G8"], 3.234 - r6 * 1.05 * 1.05e-6, 1e-3)
    return 3


def main() -> int:
    c = Checker()
    read = lambda name: (RESULTS / name).read_text()  # noqa: E731
    print(f"C-0015 C-0007 I2C: {check_i2c(c, read('i2c-bus.txt'))} corners checked")
    print(
        f"C-0015 C-0006 reset RC: {check_reset_rc(c, read('oled-reset-rc.txt'))} corners checked"
    )
    print(
        f"C-0015 C-0003 buzzer: {check_buzzer(c, read('buzzer-driver.txt'))} corners checked"
    )
    print(
        f"C-0015 C-0008/C-0014 strapping: {check_strapping(c, read('strapping-gpio2.txt'))} cases checked"
    )
    print(
        "C-0015 second method: all agree"
        if c.failures == 0
        else f"C-0015 second method: {c.failures} FAIL"
    )
    return 1 if c.failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
