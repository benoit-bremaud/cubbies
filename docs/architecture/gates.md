# Design gates

Two lightweight review gates, borrowed from the PDR / CDR practice and scaled to a hobby
series. Each gate is a checklist the owner validates explicitly; nothing after a gate starts
before it is passed. Decision: ADR-0011.

## PDR — preliminary design review (conception validated before any code)

Applies per feature that adds a requirement, a port, a radio frame field or a state. Entry: the
feature has an issue with acceptance criteria. A change inside an existing behaviour (a new
expression, a tuned threshold) needs only: requirement cited, tests or golden images updated,
owner validation in the PR.

- [ ] Requirements it realises are listed in `specs/requirements.md` and cited by its use cases.
- [ ] Textual use cases written (Cockburn), with extensions.
- [ ] UML views whose question applies exist under `diagrams/<feature>/` and cite the ADRs.
- [ ] Four review passes done: requirements and traceability; Clean dependency direction and
      SOLID; KISS/YAGNI/DRY; pattern fit and anti-patterns.
- [ ] Traceability matrix updated (REQ ↔ UC ↔ diagrams ↔ planned tests).
- [ ] For visual features: reference images rendered and accepted by the owner.
- [ ] Owner validation recorded in the PR.

## CDR — critical design review (hardware frozen before ordering the series)

Applies once, before the parts for the series are ordered; a mechanical CDR applies before the
series print.

- [ ] GPIO allocation signed: `docs/hardware/electronics-spec.md` table matches
      `firmware/include/pins.h` and passes ERC.
- [ ] Strapping pins (GPIO2, GPIO8, GPIO9 on the ESP32-C3): the level each attached circuit
      forces at reset checked against the datasheet boot-mode table.
- [ ] Every calculation and simulation of `verification-plan.md` marked "approved" with evidence.
- [ ] Bench measurements on at least two prototypes recorded: current, dB, range, latency.
- [ ] BOM confirmed with suppliers and prices; reference designators consistent with the schematic.
- [ ] Assembly procedure tried once end to end on a prototype, time recorded.
- [ ] Enclosure printed, fitted, snap-fit and window verified.
- [ ] Owner validation recorded in the PR.

## Exit of V1

- [ ] Adoption day rehearsal with ≥ 3 cubes: adoption ritual, greeting, contagion, chain.
