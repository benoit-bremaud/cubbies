#!/usr/bin/env sh
# Measure Python coverage (sim scripts with the system python, the case script inside the
# FreeCAD AppImage python in check-only mode, so no export is rewritten), write coverage.xml,
# then run the local SonarQube scan. C/C++ is not analysed by the Community server.
# Like run.sh, schematic_values.py regenerates schematic.lib from the schematic.
# Usage: SONAR_TOKEN=$(cat ~/.config/sonarqube/analysis-token) sh tools/sonar-scan.sh
# Needs uv, python3, kicad-cli 9, freecadcmd (FreeCAD 1.1, python 3.11) and sonar-scanner.
set -eu
cd "$(dirname "$0")/.."
: "${SONAR_TOKEN:?set SONAR_TOKEN to the analysis token}"
token=$SONAR_TOKEN
unset SONAR_TOKEN  # only sonar-scanner gets it
COVERAGE_VERSION=7.17.1
FREECAD_PYTHON=3.11
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
CUBBIES_ROOT=$(pwd)
CUBBIES_COV_SITE="$work/site"
export COVERAGE_FILE="$CUBBIES_ROOT/.coverage" CUBBIES_ROOT CUBBIES_COV_SITE
cov() { uvx --from "coverage==$COVERAGE_VERSION" coverage "$@"; }

rm -f "$CUBBIES_ROOT/.coverage" "$CUBBIES_ROOT"/.coverage.*  # coverage erase keeps parallel files
for script in schematic_values.py test_schematic_values.py test_crosscheck.py calc.py crosscheck.py; do
  (cd hardware/electronics/sim && cov run -p --source=. "$script" > "$work/$script.log" 2>&1) \
    || { cat "$work/$script.log"; exit 1; }
done

uv pip install -q --python-version "$FREECAD_PYTHON" --target "$CUBBIES_COV_SITE" "coverage==$COVERAGE_VERSION"
# freecadcmd exits 0 on an uncaught exception, so the wrapper turns one into exit 1.
cat > "$work/case_cov.py" << 'PY'
import os, pathlib, sys, traceback
sys.path.insert(0, os.environ["CUBBIES_COV_SITE"])
import coverage
script = os.path.join(os.environ["CUBBIES_ROOT"], "hardware", "case", "cubbies_case.py")
cov = coverage.Coverage(data_file=os.environ["COVERAGE_FILE"], data_suffix=True, include=[script])
os.environ["CUBBIES_CHECK_ONLY"] = "1"
cov.start()
try:
    exec(compile(pathlib.Path(script).read_text(), script, "exec"), {"__name__": "cubbies_case", "__file__": script})
except Exception:
    traceback.print_exc()
    sys.exit(1)
finally:
    cov.stop()
    cov.save()
print("CASE-COV-OK")  # freecadcmd exits 0 on any exception, so success must be explicit
PY
freecadcmd "$work/case_cov.py" > "$work/case.log" 2>&1 || { cat "$work/case.log"; exit 1; }
if tr '\r\t' '\n\n' < "$work/case.log" | grep -q '^FAIL' || ! grep -q CASE-COV-OK "$work/case.log"; then cat "$work/case.log"; exit 1; fi

cov combine -q
cov xml -q -o coverage.xml
cov report --skip-empty
SONAR_TOKEN=$token sonar-scanner
