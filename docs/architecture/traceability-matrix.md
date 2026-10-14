# Traceability matrix — cubbies V1

Proves coverage: every requirement has a use case, a design view, a component and a
verification; nothing is orphaned. This file owns the REQ → design → verification mapping; the
use cases own UC → REQ. Updated at each PDR. Port and module names follow the component diagram
of the first UML study (`diagrams/cube-core/03-component.md`, merged in PR #5).

| Requirement | Use case(s) | Sequence / state | Component (port) | Verification |
|-------------|-------------|------------------|------------------|--------------|
| REQ-F-01 eyes | UC3, UC5 | — | eye renderer → `IDisplay` | golden images, blink period test |
| REQ-F-02 moods | UC3, UC4, UC8 | [05a-state-mood-engine](diagrams/cube-core/05a-state-mood-engine.md) | mood engine | native state tests, golden images |
| REQ-F-03 touch gestures | UC1, UC3, UC6 | — | gesture classifier (touch-down + classified gestures) ← `ITouchSource` | native gesture tests, bench |
| REQ-F-04 sounds | UC3, UC5, UC7 | — | sound bank → Sound gate → `IAudio` | bench, ngspice (driver) |
| REQ-F-05 volume, silence | UC6 (V1.x ritual step), B1 | [05b-state-sleep-wake](diagrams/cube-core/05b-state-sleep-wake.md) | Sound gate in `lib/` → `IAudio` | native tests |
| REQ-F-06 sleep / wake | UC5 | [05b-state-sleep-wake](diagrams/cube-core/05b-state-sleep-wake.md) | Sleep cycle ← `IClock` (uptime) | native tests with injected clock |
| REQ-F-07 identity default + ritual | UC1 | [02a-first-boot](diagrams/identity/02a-sequence-first-boot.md), [02b-adoption](diagrams/identity/02b-sequence-adoption-ritual.md) | identity rules ← `IStorage`, MAC passed by value | native tests, bench |
| REQ-F-08 identity persistence | UC1 | [02a-first-boot](diagrams/identity/02a-sequence-first-boot.md) | `IStorage` (NVS adapter) | bench |
| REQ-F-09 sound reactions | UC4, UC5 | — | sound classifier ← `IMic` | bench, native threshold tests |
| REQ-F-10 presence, greeting | UC7 | [02-sequence-presence](diagrams/social-presence/02-sequence-presence.md), [06-data-flow](diagrams/social-presence/06-data-flow.md) | presence ← `IRadio` | native protocol tests, bench ×2 |
| REQ-F-11 contagion | UC8 | **deferred**: `social-group` PDR (05a has only the hook) | mood engine + social protocol | native tests |
| REQ-F-12 chain reaction | UC9 | **deferred**: `social-group` PDR | social protocol (event relay) | native tests (no echo storms) |
| REQ-F-13 rare gag | B2 | **deferred**: `social-group` PDR (mood-engine extension) | mood engine + awake-time accumulator + rate limiter | native rate-limit tests |
| REQ-F-14 choir | UC10 | tier B | — | bench |
| REQ-F-15 motion reactions | B3 | tier B (S9) | motion classifier ← `IMotion` (port created at the S9 PDR) | native tests, bench |
| REQ-NF-01..11 | — | — | see [verification-plan.md](verification-plan.md) | per method |

PDR `cube-core` passed on 2026-10-14 (ADR-0013). Deferred to the `social-group` PDR: REQ-F-11,
F-12, F-13. Open until then: double tap cycling backwards in the ritual (with the palette size).
