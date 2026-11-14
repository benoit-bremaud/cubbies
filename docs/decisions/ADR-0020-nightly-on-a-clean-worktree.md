# ADR-0020: The nightly runs on a clean origin/main worktree and fails on secrets only

**Status:** Accepted — 2026-11-15 — issue #104 (supersedes the L3 part of ADR-0019)

## Context

ADR-0019 put the L3 tier, `just nightly`, on a systemd user timer whose units lived only on the
owner's machine and ran in the main checkout as it stood, and made any step fail it: `check`,
TruffleHog, `pio pkg outdated` and the `mise outdated` drift report. In the owner's sweep of
2026-11-13 across the projects with a nightly, 9 runs failed: 2 on real dependency
vulnerabilities, 3 on tool drift and 4 on a dirty or feature-branch checkout, not on `main`.

The owner decided on 2026-11-13, for every project (agents-skills#427, skill `local-ci` rule 9
and § Apply to a project, step 4), that the nightly stays on the workstation (`Persistent=true`) but runs in a
dedicated worktree of `origin/main`, and that only a vulnerability or a secret fails it.

## Options and scoring

Where the nightly runs: the owner's MCDA of 2026-11-13 on agents-skills#427, applied to every
project; the issue records the totals and the rejection reason only, not the criteria and weights. Option A, the workstation with
`Persistent=true` catch-up and a dedicated clean worktree of `origin/main`, scored **400/500**
against 385 and 375 for the two alternatives; moving the nightly to the homelab was rejected
because its setup and the copy of secrets to another host outweigh faster alerts. Running in the
working checkout as it stands (ADR-0019) is what produced the 4 false failures above.

What fails the run is the owner's scope decision on the same issue, not a scored choice: only
vulnerabilities and secrets, because tool drift is fixed by a later bump and gates nothing that
night.

## Decision

1. **Clean worktree.** The service fetches `origin/main` (failure tolerated: a catch-up run
   before the network uses the last fetch), creates `~/.cache/nightly/cubbies` as a detached
   worktree if missing (pruning a stale registration first), resets it hard to `origin/main`,
   cleans untracked files, trusts its `mise.toml`, installs the PlatformIO packages of every env
   (`just deps`, failure tolerated) and runs `mise x -- just nightly` there. Never the working
   checkout.
2. **Versioned units.** `cubbies-nightly.service`, `cubbies-nightly.timer` and
   `cubbies-nightly-failed.service` live in `scripts/systemd/` and are copied to
   `~/.config/systemd/user/` after a merge that changes them (`systemctl --user daemon-reload`; on a new machine also
   `systemctl --user enable --now cubbies-nightly.timer`).
3. **What fails.** Secrets only: gitleaks on the history (`just secrets`) and TruffleHog. cubbies
   has no dependency audit or container image to scan. A red `just check` (already a push and
   merge gate), a failing `pio pkg outdated` or `mise outdated`, and mise.toml drift print
   `WARNING nightly: …` and leave the run green. Outdated PlatformIO pins are only printed (no
   WARNING line: `pio pkg outdated` exits 0 whatever it finds), so read the full journal for them.

## Consequences

- The `OnFailure=` desktop notification now means a secret, never a dirty checkout or a new
  library release; drift is read in `journalctl --user -u cubbies-nightly.service`. A follow-up may make outdated
  PlatformIO pins print a WARNING line too.
- A red `check` on `main` no longer notifies; it is caught by the next push or gate.
- The worktree shares refs with the main checkout, so the history scans also see local branches:
  a secret in an unpushed branch still fails the nightly, which is intended.
- ADR-0019 stays in force for L0 to L2; its L3 description and the consequence "the systemd
  units … run `just nightly` in the main checkout" are superseded.
