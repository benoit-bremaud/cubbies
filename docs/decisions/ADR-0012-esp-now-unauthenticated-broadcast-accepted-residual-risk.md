# ADR-0012: Security — unauthenticated ESP-NOW broadcast accepted as a bounded residual risk in V1

**Status:** Accepted — 2026-10-14 — Amends ADR-0005 (security consequence only; the transport
decision stands)

## Context

ADR-0005 chose ESP-NOW broadcast for the social layer and stated that frames are versioned and
parsed by allowlist. The UML study of the `social-presence` feature
([02-sequence-presence.md](../architecture/diagrams/social-presence/02-sequence-presence.md),
[06-data-flow.md](../architecture/diagrams/social-presence/06-data-flow.md)) showed that
allowlist parsing protects against *malformed* frames but not against *hostile well-formed*
frames, for three reasons that ADR-0005 did not record:

- **Broadcast cannot be encrypted.** ESP-NOW encryption (CCMP with a per-peer LMK) applies only
  to a registered unicast peer; Espressif's ESP-NOW documentation states that encrypting
  multicast or broadcast action frames is not supported, so frames to `FF:FF:FF:FF:FF:FF` are
  always in the clear.
- **The source MAC is forgeable.** The sender identity comes from the 802.11 header through the
  receive callback, not from the payload, and any ESP32 can transmit with an arbitrary source
  MAC. The MAC is the social key (ADR-0008), so impersonating a cube costs nothing.
- **No provisioning path exists in V1.** There is no router, no app, no phone link and no mother
  cube (V1.5). A shared secret could only be baked into the firmware, which CLAUDE.md forbids
  ("No secrets in the firmware"), or typed on a device that has one touch pad and no keyboard.

What an attacker in radio range (10–30 m indoors with stock hardware, more with a directional
antenna; any 802.11 frame injector will do, not only an ESP32) can do with well-formed frames:

1. **Table flooding.** Send 16 distinct forged MACs, each re-sent within `T_ttl`, so every slot
   of the neighbour table stays present and real newcomers are dropped.
2. **Impersonation.** Forge a known cube's MAC to overwrite its mood byte and refresh its
   `lastSeen`. A nuisance in V1; mood injection once G2 contagion ships.
3. **Greeting nuisance.** Appear as a new MAC and trigger one greeting (eyes glance, hello beep)
   per cooldown while the cube is Awake.

The fleet is a desk toy given to colleagues. Nothing on a cube is persisted from the radio,
nothing is actuated beyond the eyes and the buzzer, and the social layer "degrades to nothing"
when alone (ADR-0001). The question is whether this threat justifies a key scheme in V1.

## Options and scoring

Criteria: V1 scope and KISS (3), zero-pairing adoption ritual preserved (3), actual harm
prevented on a desk toy (2), path to the V1.5/V2 key scheme (2), energy and code cost (1).

| Criterion (weight) | Accept bounded risk in V1 | Fleet key in NVS, provisioned at adoption | Per-peer keys on unicast | BLE advertising instead |
|---|---|---|---|---|
| V1 scope, KISS (3) | 3 | 1 | 0 | 1 |
| Zero pairing kept (3) | 3 | 1 | 0 | 3 |
| Harm prevented (2) | 1 | 3 | 3 | 0 |
| Path to V1.5/V2 (2) | 2 | 3 | 2 | 1 |
| Energy and code cost (1) | 3 | 2 | 1 | 2 |
| **Total /33** | **27** | 20 | 11 | 16 |

Notes on the losing options:

- **Fleet key in NVS.** The right V2 answer, but V1 has no channel to deliver the key: the
  adoption ritual is a tap sequence on the cube alone (ADR-0008), with no second device to carry a
  secret. Authenticating broadcast would also mean a message authentication code (an HMAC over
  the frame) computed in software, since ESP-NOW does not authenticate broadcast for us; fine,
  but pointless without provisioning.
- **Per-peer keys on unicast.** Breaks zero pairing: every cube would have to register every
  other cube's MAC and key, which is exactly the pairing step ADR-0001 refuses. ESP-NOW on the
  C3 also allows at most 20 registered peers, 17 of them encrypted
  (`CONFIG_ESP_WIFI_ESPNOW_MAX_ENCRYPT_NUM`, default 7), so the scheme hard-caps the fleet.
- **BLE advertising.** Legacy advertising is equally unauthenticated and spoofable, and loses the
  relayed-OTA path that made ESP-NOW win in ADR-0005. It changes the transport without closing
  the hole.

## Decision

**The unauthenticated ESP-NOW broadcast is accepted in V1 as a residual risk**, bounded by the
following rules, which the `social-presence` implementation must satisfy and the native tests
must assert:

- **Eviction touches absent entries only.** The neighbour table has 16 fixed slots. A newcomer
  may evict only an absent entry (age above `T_ttl`, proposal 15 s = 3 × `T_presence`), the one
  with the oldest `lastSeen`; when every slot is present the newcomer is dropped. A burst of
  forged MACs cannot push a real, present neighbour out.
- **Greeting cooldown and global budget.** Greetings are decided on the tick, at most one per
  `T_greet_cooldown` (proposal 10 s), re-greet only after `T_regreet` (proposal 10 min), and a
  global budget caps them at 6 per 10 min (tunable in the Social protocol). An attacker can make
  the cube glance and beep, not chatter.
- **Allowlist on every field, rate bounded.** Length check before any copy, 8-slot RX queue
  dropping newest, at most 4 frames decoded per tick, every field checked against the values
  the current firmware understands. A flood cannot starve gestures, eyes or display.
- **RAM only, eyes and beep only.** Nothing received over the radio is written to NVS, relayed,
  or logged with the MAC (debug output prints the palette index and at most a 16-bit hash prefix
  of the MAC, per the data-flow diagram). The only effects reachable by a forged frame are a mood byte in RAM, a
  glance of the eyes and a quiet beep through the single audio adapter.

The transport decision of ADR-0005 is unchanged. ADR-0005's implicit claim that allowlist
parsing makes the social layer safe is amended: it makes it robust, not authenticated.

## Consequences

- The threats above are documented facts of V1, not bugs. A report of "a laptop made my cube
  say hello" is answered by this ADR, not by a hotfix.
- G2 mood contagion inherits the impersonation risk. Its design must not let a received mood
  byte do more than nudge the local mood engine, so a forged frame cannot force a state.
- The V2 ADR that introduces fleet authentication is already scoped: a **fleet key stored in
  NVS, never in source or build flags**, provisioned at adoption by a channel that exists by then
  (mother cube or phone), used to authenticate broadcast frames with a software MAC and to
  encrypt unicast where needed. A shared fleet key authenticates the fleet, not an individual
  cube: impersonation between adopted cubes stays possible, and the key is only as secret as the
  flash, so NVS encryption and flash encryption are prerequisites of that ADR. Provisioning, key
  rotation and re-adoption are that ADR's problems, not V1's.
- The V1.5 relayed-OTA ADR must require a **signed image** (public key in firmware, no secret
  on the cube; ESP-IDF Secure Boot V2 app-image signatures, or an application-level
  Ed25519/ECDSA check), not the SHA-256 hash alone that ADR-0005 mentions: a hash defends
  against corruption, a signature against a hostile sender on the same unauthenticated
  transport.
- The adoption-day briefing states, with the MAC privacy note from the data-flow diagram, that a
  cube in range can be imitated and that nothing worse than a wrong face or a beep can follow.
