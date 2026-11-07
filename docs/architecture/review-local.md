# Local review — cubbies (local-first CI, issue #99)

- Date: 2026-11-07
- Commit: 3268bd5 (fix commits on 30f7e4f; checked SHA for the pre-push run)
- Scope: branch diff `origin/main...HEAD` (`ci/99-local-ci-no-workflows`), 27 files: justfile, mise, lefthook, typos, scripts, deleted workflows, ADR-0019, docs
- Checks run: adversarial (Opus, 2 rounds) · design · docs · tests · security · correctness · `just check` from a clean environment
- Verdict: ship — every Must fixed in a415905 and 3268bd5 (round 3 Should in the report commit) · Must 0 open · Should 5 deferred · Nice 10 deferred

## Adversarial review (#236)
- Round 1 on `30f7e4f`: Must 3 · Should 10 · Nice 6 · unverified 2 · Push gate: closed — 3 Must open
- Round 2 on `a415905`: 3 Must fixed · Should 7 fixed, 3 accepted exceptions · new Must 1 (`just ruff` pasted file names unquoted) · Push gate: closed — 1 Must open
- Round 3 on `3268bd5`: round 2 Must fixed · Should 3 fixed · new Should 2 (`just flash` trust step, log count 73), fixed in the report commit · **Push gate: open — 0 Must open**
- Hosted reviewers: none (owner policy: local review only, no Copilot/Codex on this repository)

## Pre-merge gate (head SHA 3268bd5)
| Step | Result |
|---|---|
| 0 CI mirror | n/a — no PR workflow left (ADR-0019); `just check` is the CI |
| 1 Tests | ✅ 373 native cases (golden images compared under `CI=1`) · ✅ 70 wiring cases · ✅ 23 attestation cases · ✅ both board builds |
| 2 Coverage | ⏸ Python coverage measured by `tools/sonar-scan.sh` in `just gate` (pre-merge run) · docs: n/a (no public API change) |
| 3 SonarQube | ⏸ deferred to the pre-merge run (`just gate` on the pushed head) |
| 4 Secrets | ✅ gitleaks clean (`origin/main..HEAD` and the full history, 61 commits) · no tracked `.env` · diff read: a public ed25519 key and an email already in the history (`.github/allowed_signers`), nothing else |
| 5 E2E/smoke | n/a — firmware flashed by cable, nothing deploys; bench validation per `docs/architecture/verification-plan.md` |
- Gate: `--push`: **pre-push green** — `mise x -- just check` passed from a clean environment (`env -i`, 40–87 s warm, 174 s cold); SonarQube and the attestation deferred to the pre-merge run

## 🔴 Must Have
_None open._ Fixed in a415905:
- `justfile` test recipe `[adversarial]` · the golden guard of `Golden.h` relied on `CI`, which no tier set · `CI=1 pio test -e native`; `just golden` regenerates; wiring test asserts it
- `.gitleaks.toml:10` `[adversarial|docs]` · pin comment named the deleted workflow · now `mise.toml`
- `ADR-0019:102` `[adversarial|docs]` · "README § Local quality scan" renamed · `README § Local CI`
- `ADR-0019:11` vs `PROJECT_LOG.md` `[docs|adversarial]` · two different minutes figures · one figure, jobs API (589 runs, 812 min), table on #99
- `justfile`, `lefthook.yml`, `CLAUDE.md`, `README.md` `[design]` · ruff / PlatformIO pins copied in five places · `just ruff` recipe serves lint and the hook, docs name recipes, wiring test refuses a copied pin
- `scripts/test-attest.sh` `[tests]` · the bad-signature branch could be deleted unnoticed; the fake `gh` ignored state/context and the jq filter · cases "trusted key, other payload", "wrong namespace", "status failure"; fake `gh` records and filters
- `scripts/test-local-ci.sh` `[tests]` · `-` cases accepted any cause; `sonar: no token` inherited the shell token; recipe greps missed the wiring scripts · causes asserted, `env -u SONAR_TOKEN`, token-file case, `--error`, `gitleaks git`, scripts in `test`, failing `check` stops the gate, injection arguments to `merge`, 100/101-character subjects

## 🟡 Should Have
Fixed in the report commit: trust step first in `just flash`, `xargs -r`, log count 73.

Fixed in 3268bd5: `just ruff` positional arguments (new Must of round 2, wiring case with a stub `uvx`) · `just flash` so a manual flash keeps the isolated packages directory (`assembly.md`) · `git ls-files -z | xargs -0` for shellcheck and Semgrep · ADR figure sentence · log case counts.

Fixed in a415905: `PLATFORMIO_PACKAGES_DIR` exported by the justfile (bare `just` too) · shellcheck and Semgrep on `git ls-files` · roadmap T1/T4 · `hardware/case/README.md` · CLAUDE.md layout · README L0/L1 rows · ADR release plan (`attest.sh release` needs a staging note) · CodeQL wording · test-attest global git config isolated.

Deferred (recorded as accepted exceptions in ADR-0019 § Consequences or filed):
- `justfile` security `[adversarial]` · `p/python` fetched from the registry at each run · vendor a pinned copy if it ever flakes
- `~/.config/systemd/user/cubbies-nightly.*` `[adversarial]` · units not versioned, run the main checkout as it stands · commit under `tools/systemd/` with an install step
- `README.md:73` `[adversarial]` · Community Build has no branch analysis, a gate on a PR head publishes under `main` · accepted until the squash replaces it
- `justfile` merge `[security]` · a head stale against `main` after the gate still merges · add an ancestor check and the PR base check in `merge`
- `scripts/pio-trust-local-ca.sh:146` `[security|design]` · a CA identified by its second line, CRLF or an empty body wrongly detected · compare SHA-256 fingerprints, refuse an empty body
- `lefthook.yml` pre-push `[security]` · checks the working tree, not the pushed ref · read the pushed SHA from stdin

## 🔵 Nice to Have
- `scripts/check-commit-msg.sh:19` `[adversarial|security]` · `${#subject}` counts bytes under dash · `wc -m` under a UTF-8 locale (upstream script, fix in agents-skills)
- `scripts/attest.sh:90` `[security]` · a tag starting with `-` reaches `verify-tag` as an option, fails safely · `--`
- `scripts/attest.sh` `[security]` · no revocation list for `allowed_signers` · acceptable for one signer, noted
- `justfile` gate `[security]` · `attest.sh write` re-reads HEAD after the HEAD-moved check · pass the SHA
- `justfile` comments `[design|docs]` · two comment blocks per recipe, the long WHY could live in the ADR
- `justfile` nightly `[design]` · `\`-continued lines where the other multi-step recipes use a shebang block
- `scripts/test-local-ci.sh` `[tests]` · L0 ruff job never exercised (no `.py` staged, avoids a download); `nightly` body untested
- `scripts/test-attest.sh` `[tests]` · `write` refusals (no signing key, push rejected) and a tag signed by an untrusted key untested
- `ADR-0019:80` `[adversarial]` · "no maintained C++ ruleset" now worded; source not cited
- `justfile` comments `[adversarial]` · duplicated doc comment blocks above `security`, L2, L3 (style)

## ⚪ Disagree / Intentional
- `justfile` nightly fails on any pin drift `[adversarial]` · intended, drift is a review item (ADR-0019)
- First merge through `gh pr merge --squash --match-head-commit` `[adversarial|security]` · bootstrap: `allowed_signers` is not on `origin/main` yet (ADR-0019 § Consequences), owner's explicit yes required
- Clean-tree check in three places, pushed check in two `[design]` · one line each with its own message; `attest.sh` is a copied script
- `gate` calls `check` from its body `[design]` · guarantees the checked commit is the attested one
- `attest.sh` keeps its unused `release` / `deploy/staging` paths `[design]` · copied word for word so the merge hook verifies with `main`'s copy

## Per-dimension summary
- Adversarial: 3 Must (golden guard, two stale doc references), all fixed; Should on network, systemd, Sonar branch recorded in the ADR · Design: pin duplication removed with `just ruff`; scope drift fixed with `git ls-files` · Docs: stale references fixed, figures unified · Tests: false-confidence assertions replaced, 54→70 and 20→23 cases · Security: no Must, no secret in the diff, shellcheck clean · Correctness: `just check` green from a clean environment on both 30f7e4f and the fixed tree.
