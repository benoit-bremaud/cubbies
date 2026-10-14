# ADR-0005: Radio — ESP-NOW broadcast for the social layer

**Status:** Accepted — 2026-10-14

## Context

The social layer needs cubes to detect each other with no pairing, no router and no app
(ADR-0001). Two connectionless candidates on the ESP32-C3: BLE advertising (31-byte legacy
payload, scanned by neighbours, very low power) and ESP-NOW broadcast (250-byte frames over
Wi-Fi, no peer registration needed for broadcast). The owner's V1.5 "mother cube" idea (one cube
with Wi-Fi fetches a release and pushes it to the others) has an existing building block only on
ESP-NOW (Espressif's ESP-NOW OTA component: a root pushes a SHA-256-verified image to a list of
MACs); no BLE peer-relay OTA library was found. ESP-NOW and a Wi-Fi station must share the same
channel, which constrains the mother cube only. Indoor range of both is in the 10–30 m class;
vendor figures are not trusted.

## Options and scoring

Criteria: zero pairing (3), OTA relayed by the mother cube (3), V2 battery energy (2), latency
for choir and chain reactions (2), code simplicity (2).

| Criterion (weight) | BLE advertising | ESP-NOW broadcast | Both in V1 |
|---|---|---|---|
| Zero pairing (3) | 3 | 3 | 3 |
| Relayed OTA (3) | 1 | 3 | 3 |
| Energy V2 (2) | 3 | 2 | 1 |
| Latency (2) | 2 | 3 | 3 |
| Simplicity (2) | 2 | 3 | 1 |
| **Total /36** | 26 | **34** | 29 |

## Decision

**ESP-NOW broadcast** carries presence, mood and group events in V1. Frames are versioned,
parsed by allowlist, unknown or malformed frames dropped. BLE is not used in V1; a phone-facing
BLE service remains possible later without changing the social layer.

## Consequences

- The protocol state machine (neighbour table, greetings, event propagation) is pure logic under
  `firmware/lib/`, with the ESP-NOW send/receive as a thin adapter, so it is testable natively.
- V2 power management must account for ESP-NOW's Wi-Fi radio duty cycle (listen windows).
- The mother cube (V1.5) reuses the same transport for OTA.
