# Local review — cubbies (branch diff)

- Date: 2026-10-14
- Commit: e17cddd (pre-fix), findings applied in the same branch before push
- Scope: branch diff `origin/main...docs/adr-0012-esp-now-residual-risk` — four markdown files
  (ADR-0012 new, ADR-0005 status line, ADR index, PROJECT_LOG entry)
- Checks run: docs · security · correctness (design and tests not applicable: no code in the diff)
- Verdict: ship as-is after the Should fixes (all applied) · Must 0 · Should 8 · Nice 4

## 🔴 Must Have
_None._

## 🟡 Should Have
- `docs/decisions/ADR-0012-…md:39` `[docs]` · criteria names differ between prose and table ("battery" vs "Energy") · aligned to "energy and code cost" — applied
- `docs/decisions/ADR-0012-…md:113` `[docs|correctness]` · eviction rule less precise than 02-sequence (oldest `lastSeen` among absent entries) · stated as in the diagram — applied
- `docs/decisions/ADR-0012-…md:116` `[docs]` · `T_ttl` not given although flooding and eviction depend on it · added 15 s = 3 × `T_presence` — applied
- `docs/decisions/ADR-0012-…md:123` `[docs|security]` · "never logged with the MAC" stricter than the data-flow diagram (16-bit hash prefix allowed in debug) · aligned — applied
- `docs/decisions/ADR-0012-…md:53` `[correctness]` · "caps encrypted peers at a small number" vague; ESP-IDF: 20 peers, 17 encrypted, default 7 · numbers cited — applied
- `docs/decisions/ADR-0012-…md:93` `[security]` · V2 fleet-key hook overclaims: a shared key authenticates the fleet, not a cube, and NVS is only secret with flash encryption · caveat added — applied
- `docs/decisions/ADR-0012-…md:55` `[docs]` · "MAC/HMAC" collides with MAC = media access control · reworded "message authentication code (HMAC)" — applied
- `docs/decisions/ADR-0012-…md:24` `[security]` · attacker range understated, not limited to an ESP32 · widened — applied

## 🔵 Nice to Have
- `docs/decisions/ADR-0012-…md:16` `[correctness]` · cite Espressif's "encrypting multicast action frames is not supported" · added
- `docs/decisions/ADR-0012-…md:98` `[security]` · name the signing mechanism for V1.5 (Secure Boot V2 or app-level Ed25519/ECDSA) · added
- `docs/decisions/ADR-0012-…md:10` `[docs]` · relative links to the social-presence diagrams dangle until `docs/uml-study` merges · left as is, merge order
- `docs/architecture/diagrams/social-presence/06-data-flow.md:104` (branch `docs/uml-study`) `[docs]` · note promises a "superseding" ADR and lists per-peer keys as a V2 hook; ADR-0012 amends and scores per-peer keys out · follow-up on that branch after merge

## ⚪ Disagree / Intentional
- `docs/decisions/README.md:18` `[docs]` · index jumps 0009 → 0012 · ADR-0010/0011 live on the unmerged `docs/design-dossier` branch, numbering agreed with the sibling session
- `docs/decisions/ADR-0005-…md:3` `[docs]` · "Amended by" rather than "Superseded by" · the transport decision stands, only the security consequence changes; ADR-0012 says so explicitly

## Per-dimension summary
- Design: n/a (no code) · Docs: Nygard sections complete, scoring arithmetic verified, wording aligned to the diagrams · Tests: n/a · Security: claims verified against ESP-IDF ESP-NOW docs via Context7, V2 hook caveated · Correctness: peer limits and OTA hash-vs-signature claims correct
