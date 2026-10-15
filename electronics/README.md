# electronics — schematic and simulations

Reserved for the KiCad 9 project (`kicad/`, native `.kicad_sch` plus a PDF export next to it)
and the ngspice netlists and plots (`sim/`) named by `docs/architecture/verification-plan.md`.

- Specification, calculations and bench measurements: `docs/electronics/`.
- The GPIO table of `docs/electronics/electronics-spec.md` and `firmware/include/pins.h` must
  match; ERC runs in CI once the first schematic lands.
- Conventions and the per-PR checklist: `docs/architecture/audits.md` (electronics row).
