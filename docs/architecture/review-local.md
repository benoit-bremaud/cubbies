# Local review — cubbies (branch diff)

- Date: 2026-10-14
- Commit: 5e7cca1 (findings applied in the amended commit that replaces it)
- Scope: branch diff `main...HEAD` on `chore/security-ci-baseline` (8 files: five workflows,
  Dependabot config, `.gitleaks.toml`, PROJECT_LOG entry)
- Checks run: design/docs (doc-auditor) · security (security-policy + security-ci-baseline pass) ·
  correctness (build/run verification of the workflows and actions) · tests: not applicable (no
  test code in the diff)
- Verdict: fix Must Have first, then ship · Must 1 · Should 5 · Nice 5 — **all Must and Should
  applied before push**

## 🔴 Must Have

- `.github/workflows/codeql.yml:22`, `dependency-review.yml:15`, `scorecard.yml:17` `[correctness|security|docs]` ·
  Code scanning upload, dependency review and the Scorecard job (whose template permissions lack
  `contents: read` for a private checkout) fail on every run while the repository is private, and a
  personal private repository cannot enable Advanced Security. Raised by all three reviews, merged
  here. · Fix applied: job-level `if: ${{ !github.event.repository.private }}` on the three jobs,
  explained in each header, to be removed in the public-release PR; recorded in PROJECT_LOG.

## 🟡 Should Have

- `.github/workflows/ci.yml:13` `[security]` · `cancel-in-progress: true` also cancelled runs on
  `main`, leaving a merged commit without a CI result. · Applied: cancel only on `pull_request`.
- `.github/workflows/gitleaks.yml:1` `[correctness|docs]` · Header claimed "full git history" on
  every run; the action scans the event's commit range on push/PR and the full history only on the
  weekly schedule. · Applied: header reworded; `GITLEAKS_CONFIG` made explicit.
- `.gitleaks.toml:10` `[docs]` · Comment said "keep this allowlist empty" above a non-empty
  allowlist. · Applied: comment states why the config file itself is listed.
- `PROJECT_LOG.md:10` `[docs]` · "mirrored from retro-shmup" was an unverifiable cross-repo claim,
  and the entry did not record the private-repo guards. · Applied: reworded, guards recorded.
- `CLAUDE.md:64` `[docs]` · Toolchain section described the gate as local only. · Applied: one
  sentence on the CI re-run.

## 🔵 Nice to Have

- `.github/workflows/ci.yml:37` `[correctness]` · Cache key omitted the PlatformIO core version.
  · Applied: `6.2.0` in the key with a comment.
- `.github/dependabot.yml:3` `[docs]` · The `pip install platformio==6.2.0` pins are not covered
  by Dependabot. · Applied: documented in the header; manual bump accepted for a single pin.
- `.github/workflows/ci.yml:39` `[security]` · `pip install` is version-pinned, not hash-pinned
  (SUPPLY-1 gap). · Deferred: proportionate for a hobby firmware repository; revisit if a
  `requirements-ci.txt` appears.
- `firmware/platformio.ini:23` `[security]` · The pioarduino platform archive has no checksum
  (PlatformIO offers none for URL platforms). · Deferred, recorded as a known supply-chain gap.
- `ci.yml:8`, `gitleaks.yml:8` `[correctness]` · PR triggers are not restricted to `main` as in
  codeql/dependency-review. · Deferred: only `main` exists; align if release branches appear.

## ⚪ Disagree / Intentional

- `.github/workflows/gitleaks.yml:27` `[security]` · No SARIF upload of secrets to the Security
  tab, although the `security-ci-baseline` skill lists it. · Intentional: the gitleaks-action README
  advises against it (a secret removed later shows as fixed while still in history); now stated in
  the header.
- Workflow count for a fresh repository `[design]` · Five workflows on a scaffold could look like
  over-engineering. · Intentional: it is the owner's documented baseline and the public-release
  checklist prerequisite; Gitleaks is the required item, the rest is standard.

## Per-dimension summary

- Design: no over-engineering beyond the documented baseline; comments say why, not what.
- Docs: four accuracy fixes applied (gitleaks scope, allowlist comment, log wording, CLAUDE.md).
- Tests: not applicable to this diff; the native test and both board builds were re-run from the
  committed tree by the correctness review (1/1 passed, both builds succeeded).
- Security: all eight action pins verified against upstream tags, least privilege confirmed,
  no injection surface, cache poisoning low risk; one operational Should Have applied.
- Correctness: three private-repo failure modes confirmed against the actions' sources and the
  repository state, fixed by job guards; cache/URL-platform interaction verified as working.
