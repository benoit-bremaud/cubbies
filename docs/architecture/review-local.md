# Local review — cubbies (feat/hardware-validation)

- Date: 2026-10-16
- Commit: ff13210
- Scope: branch diff main...HEAD
- Checks run:
  - design and tests: architecture-guardian (bench self-validation reviewed as test quality);
  - docs: doc-auditor;
  - correctness and security: a general-purpose review that reran the benches.
- Verdict: fix the Must Have items first. Must 3 · Should 8 · Nice 4.

## 🔴 Must Have

- **M1** `hardware/electronics/sim/run.sh:7-10`, every `*.cir`, `calc.py` `[tests|design|correctness]`
  - **Problem**: the benches never check their own PASS rules.
    - A regression, for example Rs back to 68 Ω, would print a number and the run would stay
      green.
    - `sh` has no pipefail, and `grep` matches only `Error` in that exact case.
  - **Fix**:
    - Each bench prints `FAIL <C-id>` when it breaks its limit; a negative control prints
      `EXPECTED-FAIL`.
    - `calc.py` asserts its limits.
    - `run.sh` keeps the full output, fails on `FAIL`, `error` or `fatal` and on an empty
      result, and checks the exit status of ngspice.
- **M2** `strapping-gpio2.cir:7-8`, `firmware/include/pins.h:13-14`, ADR-0015 D1 `[design|correctness]`
  - **Problem**: GPIO2 is held high only by the pull-ups on the I2C modules. ADR-0007 lets the
    P2 and P3 tiers be absent, so a cube built only to P0 or P1 leaves GPIO2 floating at reset.
    The Super Mini has the same gap on its GPIO2 (mic BCLK).
  - **Fix**: a pull-up on the cube itself, or making the IMU mandatory. **Owner decision.**
- **M3** `docs/hardware/assembly.md:18-19` `[docs]`
  - **Problem**: the parts list still gives 68 Ω for the buzzer and 10 kΩ / 100 nF for the reset
    RC with no diode, so a build from it reproduces the rejected circuit.
  - **Fix**: list 82 Ω, and 10 kΩ / 1 µF X7R with a 1N4148 (D2).

## 🟡 Should Have

- **S1** `buzzer-driver.cir:44` `[correctness]`
  - **Problem**: `pcoil` uses `i(vs)`, so it misses the current that recirculates through the
    diode, and the "≤ 11 mW" in C-0003 is understated.
  - **Fix**: sense the current in series with Lc and correct the figure in a new log entry.
- **S2** `buzzer-driver.cir:55` `[correctness|design]`
  - **Problem**: the comment says the negative control uses a 20 Ω coil, but the code sets 12 Ω.
  - **Fix**: correct the comment to 12 Ω.
- **S3** `buzzer-driver.cir:21` `[correctness]`
  - **Problem**: the bench drives the gate at 2.64 V, while `calc.py` uses 2.60 V.
  - **Fix**: use the worst case, 2.60 V, in both.
- **S4** `i2c-bus.cir:30-33` `[correctness|design]`
  - **Problem**: the 30 % and 70 % thresholds and `isink` are fixed at 3.3 V.
  - **Fix**: take them from `vp`, sweep `vp` at 3.2 and 3.3 V, and add a sink-current limit of
    3 mA.
- **S5** Super Mini GPIO8 SDA and its LED `[correctness]`
  - **Problem**: the onboard LED adds sink current on SDA.
  - **Fix**: write it up in C-0007 as a limit that applies to the prototype only.
- **S6** `oled-reset-rc.cir:44-45` `[correctness]`
  - **Problem**: pass or fail has to be read off by eye.
  - **Fix**: print an explicit PASS/FAIL (covered by M1).
- **S7** `docs/hardware/electronics-spec.md:47` `[docs]`
  - **Problem**: it still says "see calculations §1", a document that no longer exists.
  - **Fix**: point to C-0001 and C-0002.
- **S8** `strapping-gpio2.cir:6` `[docs]`
  - **Problem**: option A is still labelled "today".
  - **Fix**: label A as the rejected draft and B as adopted.

## 🔵 Nice to Have

- **N1** `oled-reset-rc.cir:16` `[correctness]`: the ideal source pulls RES# down when the
  supply is off. State that assumption in C-0006.
- **N2** `oled-reset-rc.cir` `[design]`: the `vrep` measure is never used. Remove it.
- **N3** `electronics-spec.md:70` `[correctness]`: download boot on the XIAO relies on the board's
  own pull-up on GPIO8. Cite that in C-0008 once the XIAO schematic is checked.
- **N4** C-0002 `[correctness]`: the LDO result assumes the 420 mA peak is a short burst. State
  that duty-cycle assumption in a new entry.

## ⚪ Disagree / Intentional

- `models.lib` uses generic VDMOS models, not vendor models. They are fitted to the datasheet
  limits and marked ASSUMED. The second engine is listed as open (ADR-0015).
- `calc.py` and `run.sh` are kept minimal, with no framework, as the proportionality rule asks.
- ADR-0015 does not silently reopen ADR-0007: it changes which pin does what, not the 11-pin
  budget.
- The history references to 68 Ω and 100 nF in ADR-0015 and in the log are intentional.

## Per-dimension summary

- **Design**: proportionate, and the ADR process is respected. Gaps: the benches do not check
  their own PASS rules (M1), and the boot level depends on the modules fitted (M2).
- **Docs**: complete. Freshness gaps in `assembly.md` and `electronics-spec.md`.
- **Tests**: the benches are reproducible but not self-validating (M1). The native tests are
  unaffected: 220/220 pass and both board envs build.
- **Security**: no findings. No secrets, and the shell script runs only fixed bench names.
- **Correctness**: the log figures match the results tables. Minor modelling errors: S1, S3, S4.
