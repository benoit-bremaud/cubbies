# Traceability matrix — cubbies V1

Proves coverage: every requirement has a use case, a design view, a component and a
verification; nothing is orphaned. This file owns the REQ → design → verification mapping; the
use cases own UC → REQ. Updated at each PDR. Port and module names follow the component diagram
of the first UML study (`diagrams/cube-core/03-component.md`, merged in PR #5).

| Requirement | Use case(s) | Sequence / state | Component (port) | Verification |
|-------------|-------------|------------------|------------------|--------------|
| REQ-F-01 eyes | UC3, UC5 | [05a-state-mood-engine](diagrams/cube-core/05a-state-mood-engine.md) (reactions) | eye renderer → `IDisplay` (mood frame, reaction overlay, identity frames; `Timeline`) | golden images (28 states), blink period test, reaction timing tests (F1b) |
| REQ-F-02 moods | UC3, UC4, UC8 | [05a-state-mood-engine](diagrams/cube-core/05a-state-mood-engine.md) | mood engine (`MoodEngine`); stimulus router (`route`, `effects`) | native mood tests (05a rows), router tests, golden images (F2) |
| REQ-F-03 touch gestures | UC1, UC3, UC6 | [05c-state-gesture-classifier](diagrams/cube-core/05c-state-gesture-classifier.md) | gesture classifier (touch-down + classified gestures) ← `ITouchSource`; eye renderer `TouchDown` overlay held until release | native gesture tests, bench; touch-down next-tick test (F1b) |
| REQ-F-04 sounds | UC3, UC5, UC7 | [05e-state-sound-gate](diagrams/cube-core/05e-state-sound-gate.md) | note tables + `SoundGate` → `IAudio` (notes, level) | native gate tests (F2), bench, ngspice `buzzer-driver.cir` (C-0003..C-0005, ADR-0015 D3) |
| REQ-F-05 volume, silence | UC6 (V1.x ritual step), B1 | [05b-state-sleep-wake](diagrams/cube-core/05b-state-sleep-wake.md), [05e-state-sound-gate](diagrams/cube-core/05e-state-sound-gate.md) | `SoundGate` in `lib/` → `IAudio` (constant `kVolume`, muted Dozing/Asleep) | native gate tests, `RecordingAudio` (F2) |
| REQ-F-06 sleep / wake | UC5 | [05b-state-sleep-wake](diagrams/cube-core/05b-state-sleep-wake.md) | `SleepCycle` ← `IClock` (uptime), reads `MoodEngine` | native sleep tests with injected clock (05b rows, F2) |
| REQ-F-07 identity default + ritual | UC1 | [02a-first-boot](diagrams/identity/02a-sequence-first-boot.md), [02b-adoption](diagrams/identity/02b-sequence-adoption-ritual.md), [05a-invitation](diagrams/identity/05a-state-invitation-window.md), [05b-ritual](diagrams/identity/05b-state-adoption-ritual.md) | identity rules ← `IStorage`, MAC passed by value (`resolveIdentity`, `InvitationWindow`, `AdoptionRitual`, F3); eye renderer identity frames (invitation, ritual, adoption, error) | native tests (`test_native_identity`, `test_native_invitation`, `test_native_ritual`, F3), bench; identity-frame goldens (F1b) |
| REQ-F-08 identity persistence | UC1 | [02a-first-boot](diagrams/identity/02a-sequence-first-boot.md), [05b-ritual](diagrams/identity/05b-state-adoption-ritual.md) | `IStorage` (typed `readU8`/`writeU8`, NVS adapter `NvsStorage`, F5) | native tests (stored/missing/out-of-range, single write, fail closed), bench ([bench-f5](../hardware/bench-f5.md) #8) |
| REQ-F-09 sound reactions | UC4, UC5 | [05d-state-sound-classifier](diagrams/cube-core/05d-state-sound-classifier.md) | sound classifier ← `IMic` (rms + peak per tick) | bench, native threshold tests |
| REQ-F-10 presence, greeting | UC7 | [02-sequence-presence](diagrams/social-presence/02-sequence-presence.md), [06-data-flow](diagrams/social-presence/06-data-flow.md), [05a-neighbour-entry](diagrams/social-presence/05a-state-neighbour-entry.md) | Social protocol ← `IRadio` (pull), `SelfState` per tick (`PresenceCodec`, `SocialProtocol`, F4) | native tests (`test_native_codec` incl. fuzz, `test_native_social` via `LoopbackRadio`, F4), `EspNowRadio` (F5), bench ×2 ([bench-f5](../hardware/bench-f5.md) #9–10) |
| REQ-F-11 contagion | UC8 | **deferred**: `social-group` PDR (05a has only the hook) | mood engine + social protocol | native tests |
| REQ-F-12 chain reaction | UC9 | **deferred**: `social-group` PDR | social protocol (event relay) | native tests (no echo storms) |
| REQ-F-13 rare gag | B2 | **deferred**: `social-group` PDR (mood-engine extension) | mood engine + awake-time accumulator + rate limiter | native rate-limit tests |
| REQ-F-14 choir | UC10 | tier B | — | bench |
| REQ-F-15 motion reactions | B3 | tier B (S9) | motion classifier ← `IMotion` (port created at the S9 PDR) | native tests, bench |
| REQ-NF-01..11 | — | — | see [verification-plan.md](verification-plan.md) | per method |

PDR `cube-core` passed on 2026-10-14 (ADR-0013). Deferred to the `social-group` PDR: REQ-F-11,
F-12, F-13. Open until then: double tap cycling backwards in the ritual (with the palette size).
