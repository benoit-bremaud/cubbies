# ADR-0009: Licensing — MIT for code and docs, CC BY-SA 4.0 for the enclosure, private first

**Status:** Accepted — 2026-10-14

## Context

The repository will hold firmware, documentation and 3D-printable enclosure files. The owner's
policy is private-first; publication requires the public release checklist (secret scan on the
full history, placeholders only, CI secret detection). Existing eye libraries are GPL-3
(RoboEyes); the eyes will be a custom colour renderer, so no GPL code is linked. Many enclosure
models on MakerWorld carry restrictive remix licences and cannot be reused as-is.

## Options

| Option | Pros | Cons |
|---|---|---|
| MIT code + CC BY-SA 4.0 enclosure | Permissive firmware, share-alike models, common pairing | Two licences to explain |
| GPL-3 for everything | Compatible with RoboEyes if ever linked | Copyleft on firmware that colleagues may want to fork freely |
| No licence while private | Decide later | A public repo without a licence is unusable by others |

## Decision

**MIT** for firmware and documentation (root `LICENSE`), **CC BY-SA 4.0** for everything under
`hardware/case/` (`hardware/case/LICENSE.md`). Third-party models reused in the enclosure keep
their own licence, credited in a `CREDITS.md` next to the file. The repository stays **private**
until the public release checklist passes.

## Consequences

- Linking a GPL library into the firmware would require superseding this ADR.
- The enclosure is designed from scratch or from CC-compatible sources only.

> Note (2026-10-15, repo layout PR): `hardware/case/` became `mechanics/`; the licence split is unchanged.
