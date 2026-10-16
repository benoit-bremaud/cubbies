#!/usr/bin/env sh
# Rerun every cubbies bench and the tier-3 calculation lines; full outputs land in results/.
# Fails when a bench prints FAIL (a limit broken; negative controls print EXPECTED-FAIL instead),
# when ngspice reports an error or exits non-zero, or when an output is empty.
# Usage: sh run.sh   (from anywhere; needs ngspice 42 and python3)
set -u
cd "$(dirname "$0")"
status=0
for bench in buzzer-driver oled-reset-rc i2c-bus strapping-gpio2; do
  out="results/$bench.txt"
  if ! ngspice -b "$bench.cir" > "$out.raw" 2>&1; then
    echo "ngspice failed on $bench"; status=1
  fi
  grep -vE '^(Circuit|Doing analysis|Using |Reset re-loads|Note:|No\. of Data|Reference value|Initial Transient|-+$|Node +Voltage|----)' "$out.raw" \
    | grep -E '\||FAIL|^[AB]|nodiode|rror|atal|^option|^corner|^Rpull' > "$out"
  rm -f "$out.raw"
  if [ ! -s "$out" ]; then echo "empty output: $bench"; status=1; fi
  if grep -iE '(^|[^-])FAIL C-|error|fatal' "$out"; then status=1; fi
done
python3 calc.py > results/calc.txt || status=1
if [ "$status" -eq 0 ]; then echo "all benches PASS"; else echo "benches FAILED"; fi
exit "$status"
