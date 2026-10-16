# Mechanical specification — cubbies V1 enclosure

> **Status**: draft. Decisions: ADR-0006 (display size), ADR-0010 (FreeCAD scripted). Source of
> truth: `hardware/case/cubbies_case.py` (to create); generated `.FCStd`, `.step` and `.stl`
> committed as artefacts for printing and sharing.

## Envelope and layout

| Item | Value | Why |
|------|-------|-----|
| Outer size | 40 × 40 × 40 mm, 2 mm corner radius | fits the XIAO (21 × 17.8 mm) + the OLED module (size depends on the SKU: Waveshare 0.95" RGB OLED (A) is 31.7 × 37 mm; the generic 7-pin module is smaller and is the target, measured at the CDR) + modules stacked; desk-sized |
| Wall thickness | 1.6 mm (4 perimeters at 0.4 mm) | stiffness, touch sensing through the top (≤ 2 mm) |
| Front face | OLED window 22.2 × 15.5 mm (the panel's viewing area, 22.14 × 15.42 mm; the active area is 20.14 × 13.42 mm), 1 mm recess for the 25.7 × 22.2 × 1.45 mm glass | eyes centred, no visible PCB, ≥ 0.2 mm print tolerance kept |
| Top face | touch zone marked by a shallow 15 × 15 mm dimple; copper pad glued inside | REQ-F-03 |
| Back face | USB-C cut-out 9.5 × 4 mm aligned with the XIAO connector; 4 sound slots 1 × 8 mm in front of the buzzer | power, sound |
| Side faces | 3 × Ø1.2 mm holes in front of the microphone port | sound entry, keep dust out |
| Bottom | flat, 4 silicone feet; V3 reserve: Ø 40 mm recess for a Qi receiver coil, 1 mm thick, blocked off in V1 | ADR-0002 |
| Split | two parts: front shell (holds OLED) + rear shell (holds XIAO and modules); snap-fit, 4 cantilever hooks | REQ-NF-11, no screws |
| Expansion bay (ADR-0016) | inside the rear shell: free volume 22 × 22 × 6 mm (one Qwiic module up to 20 × 20 mm, laid flat on a foam pad) with J1 (JST-SH, horizontal, 6 × 4.3 mm) reachable from the bay and ≥ 8 mm of free space in front of its mouth for a 50 mm Qwiic cable; the rear wall is a removable cover clipped by 2 cantilever hooks, opened with a fingernail notch | REQ-F-20, UC12; no screws (REQ-NF-11) |
| Internal | XIAO on a 2 mm rail, USB-C flush; modules on 3M foam pads; wire channel | assembly ≤ 45 min |

## Printing

| Parameter | Value |
|-----------|-------|
| Material | PLA (colour of the cube's identity or neutral white) |
| Layer height | 0.2 mm, no supports (overhangs ≤ 45°, window bridging ≤ 20 mm) |
| Orientation | each shell open face down |
| Tolerance | 0.2 mm on snap hooks; test coupon printed first |

## V2 / V3 reserves

- V2: battery pocket 35 × 25 × 8 mm under the modules (shell height +6 mm) and a slide switch
  cut-out on the back; separate revision of the script with a `battery=True` parameter.
- Expansion bay: script parameters `bay=(22, 22, 6)`, `j1_clearance=8`, `rear_cover_hooks=2`; the
  envelope stays 40 mm if the bay fits beside the modules, else the shell grows by the bay depth
  (to check when the script is written with the measured parts).
- V3: the Qi coil recess becomes open; receiver board under the coil, 5 V to the XIAO input.

## Deliverables

- `hardware/case/cubbies_case.py` (parametric, all values above as named parameters).
- `hardware/case/export/cubbies-front.stl`, `cubbies-rear.stl`, `cubbies.step`, `cubbies.FCStd`.
- `hardware/case/README.md`: how to regenerate with FreeCAD 1.1 (GUI or `freecadcmd`).
