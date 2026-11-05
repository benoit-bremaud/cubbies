#!/usr/bin/env sh
# Measure Python coverage (sim scripts with the system python, the case script inside the
# FreeCAD AppImage python in check-only mode, so no export is rewritten), write coverage.xml,
# then run the local SonarQube scan. C/C++ is not analysed by the Community server.
# Usage: SONAR_TOKEN=$(cat ~/.config/sonarqube/analysis-token) sh tools/sonar-scan.sh
# Needs uv, python3, freecadcmd (FreeCAD 1.1, python 3.11), kicad-cli, ngspice and sonar-scanner.
set -eu
cd "$(dirname "$0")/.."
: "${SONAR_TOKEN:?set SONAR_TOKEN to the analysis token}"
COVERAGE_VERSION=7.10.7
FREECAD_PYTHON=3.11
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
root=$(pwd)
export COVERAGE_FILE="$root/.coverage"
cov() { uvx --from "coverage==$COVERAGE_VERSION" coverage "$@"; }

cov erase
for script in schematic_values.py test_schematic_values.py test_crosscheck.py calc.py crosscheck.py; do
  (cd hardware/electronics/sim && cov run -p --source=. "$script" > /dev/null)
done

uv pip install -q --python-version "$FREECAD_PYTHON" --target "$work/site" "coverage==$COVERAGE_VERSION"
cat > "$work/case_cov.py" << PY
import os, sys
sys.path.insert(0, "$work/site")
import coverage
script = "$root/hardware/case/cubbies_case.py"
cov = coverage.Coverage(data_file="$root/.coverage", data_suffix=True, include=[script])
os.environ["CUBBIES_CHECK_ONLY"] = "1"
cov.start()
try:
    exec(compile(open(script).read(), script, "exec"), {"__name__": "cubbies_case", "__file__": script})
finally:
    cov.stop()
    cov.save()
PY
freecadcmd "$work/case_cov.py" > "$work/case.log" 2>&1 || { cat "$work/case.log"; exit 1; }
if tr '\r\t' '\n\n' < "$work/case.log" | grep -q '^FAIL'; then cat "$work/case.log"; exit 1; fi

cov combine -q
cov xml -q -o coverage.xml
cov report --skip-empty
sonar-scanner  # reads SONAR_TOKEN from the environment
