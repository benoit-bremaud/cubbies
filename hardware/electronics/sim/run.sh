#!/usr/bin/env sh
# Rerun every cubbies bench, its closed-form cross-check and the tier-3 calculation lines; full outputs land in results/.
# Fails when a bench prints FAIL (a limit broken; negative controls print EXPECTED-FAIL instead),
# when ngspice reports an error or exits non-zero, or when an output is empty.
# Component values come from the schematic: schematic.lib is regenerated from ../cubbies-v1.kicad_sch
# when kicad-cli is installed (the committed copy is used otherwise).
# Usage: sh run.sh   (from anywhere; needs ngspice 42 and python3; kicad-cli 9 to refresh the values)
set -u
cd "$(dirname "$0")"
status=0
if command -v kicad-cli > /dev/null 2>&1; then
  # keep going on failure: every bench still reports
  python3 schematic_values.py || { echo "schematic value extraction failed"; status=1; }
else
  echo "kicad-cli not found: using the committed schematic.lib"
fi
python3 test_schematic_values.py || status=1
for bench in buzzer-driver oled-reset-rc i2c-bus strapping-gpio2; do
  out="results/$bench.txt"
  if ! ngspice -b "$bench.cir" > "$out.raw" 2>&1; then
    echo "ngspice failed on $bench"; status=1
  fi
  grep -vE '^(Circuit|Doing analysis|Using |Reset re-loads|Note:|No\. of Data|Reference value|Initial Transient|-+$|Node +Voltage|----)' "$out.raw" \
    | grep -E '\||FAIL|^[AB]|^G8|nodiode|rror|atal|^option|^corner|^Rpull' > "$out"
  rm -f "$out.raw"
  if [ ! -s "$out" ]; then echo "empty output: $bench"; status=1; fi
  if grep -iE '(^|[^-])FAIL C-|error|fatal' "$out"; then status=1; fi
done
python3 calc.py > results/calc.txt || status=1
# second method for the Tier 1 benches (C-0015): must agree with every ngspice corner
python3 crosscheck.py > results/crosscheck.txt || { grep 'FAIL' results/crosscheck.txt; status=1; }
if [ "$status" -eq 0 ]; then echo "all benches PASS"; else echo "benches FAILED"; fi
exit "$status"
