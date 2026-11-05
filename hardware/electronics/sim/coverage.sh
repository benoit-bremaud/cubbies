#!/usr/bin/env sh
# Measure the Python coverage of the bench scripts and write coverage.xml for SonarQube (issue #53).
# Runs the two test scripts, then the real scripts the benches use: calc.py, crosscheck.py (reads
# the results/ that run.sh writes) and schematic_values.py (needs kicad-cli 9, rewrites schematic.lib).
# Usage: sh run.sh && sh coverage.sh   (needs uv; coverage is fetched by uvx, pinned)
set -eu
cd "$(dirname "$0")"
cov="uvx coverage@7.17.1"
log="$(mktemp)"
trap 'rm -f "$log"' EXIT
rm -f .coverage coverage.xml  # a failed run must not leave a stale report for the scanner
for script in test_schematic_values.py test_crosscheck.py calc.py crosscheck.py schematic_values.py; do
  $cov run -a --branch "$script" > "$log" || {
    cat "$log"; echo "$script failed"; exit 1
  }
done
$cov report
$cov xml -o coverage.xml
