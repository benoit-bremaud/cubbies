#!/usr/bin/env sh
# Rerun every cubbies bench and the tier-3 calculation lines; result tables land in results/.
# Usage: sh run.sh   (from hardware/electronics/sim; needs ngspice 42 and python3)
set -eu
cd "$(dirname "$0")"
for bench in buzzer-driver oled-reset-rc i2c-bus strapping-gpio2; do
  ngspice -b "$bench.cir" 2>&1 | grep -E '^[0-9AB]|^option|^corner|^Rpull|nodiode|Error' > "results/$bench.txt"
done
python3 calc.py > results/calc.txt
grep -l Error results/*.txt && exit 1 || echo "all benches ran"
