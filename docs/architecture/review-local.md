# Local review — cubbies (branch `docs/uml-study`)

- Date: 2026-10-14
- Commit: e894c8a (findings applied in the follow-up commit on the same branch)
- Scope: branch diff `main...HEAD` — 8 new UML diagrams, `docs/architecture/README.md`, `PROJECT_LOG.md`. Docs only, no code.
- Checks run: design (architecture-guardian, three passes) · docs (doc-auditor) · tests (not applicable, no code in scope) · security (security-policy threat-model pass on the frame and identity designs) · correctness (Mermaid render of every block with `mmdc`, 8/8 OK, and a relative-link resolution script, all links resolve)
- Verdict: ship after the listed fixes, all applied before push  ·  Must 4 · Should 6 · Nice 5

Every Must and Should below is **applied** in the working tree; the lines record what the gate
found and what changed, so the PR reviewer can check the fix rather than rediscover the issue.

## 🔴 Must Have

- `docs/architecture/diagrams/social-presence/06-data-flow.md:30` `[security]` · The design bounded frame *content* but not frame *rate*: "bounded RX queue" with no overflow policy and no per-tick drain budget, so a replay flood could starve the tick or silently drop legitimate frames · Added the receive-path budget table: drop newest on full (8 slots), 4 frames decoded per tick, counters by reason, native flood test.
- `docs/architecture/diagrams/social-presence/06-data-flow.md:57` `[security]` · Length check was drawn after the queue, contradicting "dropped before any field is read" and forcing 250-byte slots · Length compare moved into the callback, fixed 8-byte slots, diagram and note updated.
- `docs/architecture/diagrams/social-presence/06-data-flow.md:88` `[security][design]` · Spoofing consequence overstated ("at most one entry and one greeting"): a sustained stream of 16 forged MACs hides real newcomers, and a forged known MAC overwrites mood and `lastSeen` · Rewritten as an honest accepted residual risk with a global greeting budget, submitted to the owner for an ADR-0005 amendment with the V2 key hook.
- `docs/architecture/diagrams/cube-core/05a-state-mood-engine.md:73` `[docs][design]` · Stale cross-reference after the use-case renumbering (UC3 instead of UC2) · Fixed.

## 🟡 Should Have

- `docs/architecture/diagrams/cube-core/03-component.md:46` `[security][correctness]` · `nowMs` wrap-around (49.7 days) unspecified for an always-on device · Clock contract added (uint32, modular subtraction only, wrap test).
- `docs/architecture/diagrams/social-presence/06-data-flow.md:98` `[security]` · "never logged outside debug builds" is a PRIV hole, prototypes sit on the same desks · Debug output limited to palette index and a 16-bit MAC hash prefix, counters by reason only.
- `docs/architecture/diagrams/social-presence/06-data-flow.md:103` `[security]` · GDPR framing incomplete (sender-side leak, inference of presence at work, lawful basis, retention) · Privacy paragraph added.
- `docs/architecture/diagrams/identity/02a-sequence-first-boot.md:29` and `02b:47` `[security]` · NVS value shape unchecked (Preferences default looks like index 0) and no failed-write branch (fail closed) · One-byte shape check in 02a, `put` failure branch in 02b.
- `docs/architecture/diagrams/cube-core/01-use-case.md:65` `[design]` · UC5 "greet once" promised more than the Awake-only policy delivers · Guarantee reworded to include the Awake condition, in 01 and 02.
- `docs/architecture/diagrams/cube-core/05b-state-sleep-wake.md:4` and `06-data-flow.md:3` `[docs]` · Header blocks not aligned with the other six files · Roadmap link and explicit "Frame: none" added.

## 🔵 Nice to Have

- `docs/architecture/diagrams/social-presence/06-data-flow.md:19` `[security]` · "rejects other ESP-NOW users" overstated for a magic number · Reworded to "filters unrelated traffic". Applied.
- `docs/architecture/diagrams/social-presence/06-data-flow.md` `[security]` · OTA "SHA-256-verified" defends against corruption, not a hostile image · Note added that the V1.5 ADR must require a signed image. Applied.
- `docs/architecture/diagrams/cube-core/03-component.md:80` `[security]` · Queue primitive unnamed · SPSC ring or FreeRTOS queue stated. Applied.
- `docs/architecture/diagrams/cube-core/03-component.md:103` `[docs]` · Four runtime paragraphs between diagram and notes were hard to scan · `### Runtime rules` sub-heading added. Applied.
- Header blocks `[docs]` · ADRs named in plain text rather than linked · Not applied: the template allows it and the ADR folder is stable. Can be done in the PR if the reviewer asks.

## ⚪ Disagree / Intentional

- `docs/architecture/diagrams/cube-core/05a-state-mood-engine.md:97` `[docs]` · "the beeps of S4" flagged as possibly stale · Correct as written: roadmap S4 is "expressive beeps per mood, wake-up melody, volume setting".
- `PROJECT_LOG.md:10` `[docs]` · Entry written before a PR exists · Intentional draft entry; it is rewritten with the merge SHA at post-merge cleanup per `project-log-discipline`.
- Extra sections beyond the template (Cockburn brief, transition tables, frame layout, mood encoding) `[docs]` · They are the source of the firmware constants and the frame contract, not decoration.
- CLAUDE.md "volume is a stored setting with quiet hours" not edited although the study drops quiet hours from V1 `[design]` · A project rule change is the owner's decision; it is flagged in `01-use-case.md` and in the Q&A submitted with the study.

## Per-dimension summary

- Design: three adversarial passes, 10 then 4 Must items found and fixed, third pass clean; remaining owner decisions are listed in the study's Q&A, not hidden in the diagrams.
- Docs: template followed by 8/8 files, every link resolves, no doc-noise, one stale reference fixed.
- Tests: not applicable (no code); every diagram names the native test that will cover it.
- Security: no secrets; input validation complete once the receive-path budget, NVS shape check and clock wrap were added; spoofing stated as an accepted residual risk pending ADR amendment.
- Correctness: all Mermaid blocks render, no stale UC numbers remain (`grep` clean).
