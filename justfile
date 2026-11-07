# Local CI (#99, skill local-ci, ADR-0019): four tiers, one recipe each, no GitHub and no AI agent
# needed. GitHub Actions runs nothing: cubbies deploys nothing (the firmware is flashed by cable;
# V1.5 will distribute a release through the mother cube, by a local `just release`, roadmap T6).
# Tools come from mise.toml (`just setup`); run through `mise x -- just <tier>` when the shell does
# not activate mise (the git hooks do). PlatformIO, ruff and Semgrep run through `uvx`, pinned here.
#
#   L0 fast     pre-commit hook                        staged files only                       < 10 s
#   L1 check    pre-push hook, review-local --push     lint + test + build + security + secrets < 3 min
#   L2 gate     review-local § 2b, on the pushed head  check + SonarQube, then the attestation  5–15 min
#               before every merge
#   L3 nightly  systemd user timer                     check + TruffleHog + pin drift
#
# Then: `just merge <pr>` (refuses without the gate attestation on the head SHA).

set shell := ["sh", "-eu", "-c"]

pio := "uvx --from platformio==6.2.0 pio"
ruff := "uvx ruff@0.14.0"
semgrep := "uvx semgrep@1.183.0"

# PlatformIO packages of this project only: the shared ~/.platformio/packages holds one directory per
# package name, and another platform on this machine pins a different toolchain-riscv32-esp, so the
# two projects kept reinstalling each other's toolchain (ADR-0019). Platform, penv and cache stay shared.
export PLATFORMIO_PACKAGES_DIR := env("HOME") / ".platformio/packages-cubbies"

# L1 by default
default: check

# Once per clone: the pinned toolchain and the git hooks
setup:
    mise install
    mise x -- lefthook install

# L0 — the pre-commit checks on the staged files (< 10 s)
fast:
    lefthook run pre-commit

# ruff check + format on the whole repository, or on the files the pre-commit hook passes. Positional
# arguments: a staged file name is passed as one quoted argument, never pasted into the command line.
[positional-arguments]
ruff *files:
    if [ $# -eq 0 ]; then set -- .; fi; {{ruff}} check "$@" && {{ruff}} format --check "$@"

# ruff, typos, shellcheck on every tracked shell script, the hook config
lint: ruff
    typos
    git ls-files -z '*.sh' | xargs -0 -r shellcheck --severity=warning
    lefthook validate

# CI=1: the golden images are compared, never regenerated, whatever the shell exports (Golden.h).
# Native unit tests of the behaviour logic (golden images included), then the local CI wiring tests
test:
    cd firmware && CI=1 {{pio}} test -e native
    sh scripts/test-local-ci.sh
    sh scripts/test-attest.sh

# Regenerate the golden images (ADR-0014): review the sheet of firmware/tools/golden_sheet.py in the PR
golden:
    cd firmware && CUBBIES_UPDATE_GOLDEN=1 {{pio}} test -e native

# Flash a board by USB (docs/hardware/assembly.md); the packages directory of this project applies here too
flash env="xiao_c3":
    sh scripts/pio-trust-local-ca.sh
    cd firmware && {{pio}} run -e {{quote(env)}} -t upload

# The penv certifi bundle must trust the machine's local CAs first (scripts/pio-trust-local-ca.sh).
# Firmware build for every board env
build:
    sh scripts/pio-trust-local-ca.sh
    cd firmware && {{pio}} run -e xiao_c3 -e c3_supermini

# SAST on the Python tooling (Semgrep Community Edition, named ruleset: `--config auto` would send the
# project URL; metrics off). The C++ behaviour logic has no free Semgrep ruleset: it is covered by
# the native tests and the review (ADR-0019). EIO_BACKEND=posix: semgrep-core otherwise opens
# io_uring rings charged to RLIMIT_MEMLOCK and dies intermittently (skill local-ci § Gotchas).
# SAST (Semgrep CE, p/python) on every tracked Python file
security:
    git ls-files -z '*.py' | EIO_BACKEND=posix xargs -0 -r {{semgrep}} scan --config p/python --error --metrics=off --quiet

# Secrets in the whole git history
secrets:
    gitleaks git --redact --no-banner .

# L1 — pre-push (< 3 min)
check: lint test build security secrets

# L2 — pre-merge, called by review-local § 2b (base: the branch to integrate into), 5–15 min.
# Ends with the gate attestation (agents-skills ADR-0003): commit status `gate` + signed note on
# HEAD, which must already be pushed and up to date with `base`, so it runs on the pushed head only;
# the clean-tree and pushed checks run first, not after 10 minutes in `attest.sh write`.
# L2 — pre-merge on the pushed head: check + SonarQube, then the attestation (5–15 min)
gate base="origin/main":
    #!/bin/sh
    set -eu
    base={{quote(base)}}
    [ -z "$(git status --porcelain)" ] || { echo "gate: commit or set aside local changes first" >&2; exit 1; }
    case "$base" in origin/*) git fetch --quiet origin "${base#origin/}" ;; esac
    git merge-base --is-ancestor "$base" HEAD || { echo "gate: HEAD is behind $base, merge or rebase it first" >&2; exit 1; }
    [ -n "$(git branch -r --contains HEAD)" ] || { echo "gate: push HEAD first (the attestation is written on the pushed head)" >&2; exit 1; }
    # `check` runs from the body, not as a dependency, so the HEAD it checked is the one attested.
    start=$(git rev-parse HEAD)
    just check
    just sonar
    [ "$(git rev-parse HEAD)" = "$start" ] || { echo "gate: HEAD moved during the gate ($start checked), run it again" >&2; exit 1; }
    sh scripts/attest.sh write gate

# SonarQube analysis on the shared self-hosted instance with the machine's global analysis token
# (never a per-project one; skill infra-accounts), waiting for the quality gate. tools/sonar-scan.sh
# measures the Python coverage first (sim scripts, case script inside FreeCAD), then scans.
# SonarQube analysis with quality-gate wait (step of gate; global analysis token)
sonar:
    #!/bin/sh
    set -eu
    command -v sonar-scanner > /dev/null || { echo "sonar: sonar-scanner not on PATH (system tool, skill local-ci rule 2)" >&2; exit 1; }
    SONAR_TOKEN="${SONAR_TOKEN:-$(cat "$HOME/.config/sonarqube/analysis-token" 2>/dev/null || true)}"
    [ -n "$SONAR_TOKEN" ] || { echo "sonar: no token in SONAR_TOKEN or ~/.config/sonarqube/analysis-token" >&2; exit 1; }
    SONAR_TOKEN="$SONAR_TOKEN" sh tools/sonar-scan.sh -Dsonar.qualitygate.wait=true

# Squash, like every merge of this repository so far.
# Merge a PR only when its exact head SHA carries the gate attestation (agents-skills ADR-0003)
merge pr:
    #!/bin/sh
    set -eu
    pr={{quote(pr)}}
    case "$pr" in ''|*[!0-9]*) echo "merge: usage just merge <pr number>" >&2; exit 1 ;; esac
    sha=$(gh pr view "$pr" --json headRefOid -q .headRefOid)
    git fetch --quiet origin "$sha"
    sh scripts/attest.sh verify gate "$sha"
    gh pr merge "$pr" --squash --match-head-commit "$sha"

# L3 — nightly, systemd user timer (no time budget): deep secret history, then pin drift. PlatformIO
# packages (platform, libraries) are pinned exactly by hand in firmware/platformio.ini (no Dependabot
# ecosystem), so `pio pkg outdated` is the only watch on them: its table is printed for the journal
# (an exact pin never has a "Wanted" newer than "Current", so a newer "Latest" is a review item, not
# a failure). `mise outdated` exits 0 whatever it finds and lists the global ~/.tool-versions under
# --no-global, so the recipe keeps the mise.toml rows and fails on a non-empty report (`--bump`:
# without it exact pins report nothing).
# L3 — nightly (systemd timer): check + TruffleHog + PlatformIO and toolchain pin drift
nightly: check
    trufflehog git "file://$PWD" --no-verification --fail --no-update
    cd firmware && {{pio}} pkg outdated
    out=$(mise outdated --bump --no-global --no-header) || { echo "nightly: mise outdated failed" >&2; exit 1; }; \
    drift=$(printf '%s\n' "$out" | grep -F mise.toml || true); \
    [ -z "$drift" ] || { echo "nightly: toolchain drift, bump mise.toml:"; echo "$drift"; exit 1; }
