# Mechanical specification — cubbies V1 enclosure

> **Status**: scripted, 18 dimension checks green (2026-10-19, issues #42, #45); print-and-fit is the
> mechanical CDR bench step. Decisions: ADR-0006 (display size), ADR-0010 (FreeCAD scripted),
> ADR-0016 (module bay). Source of truth: `hardware/case/cubbies_case.py`; generated `.FCStd`,
> `.step` and `.stl` committed as artefacts for printing and sharing.

## Envelope and layout

| Item | Value | Why |
|------|-------|-----|
| Outer size | 40 × 40 × 40 mm, 2 mm corner radius | fits the XIAO (21 × 17.8 mm) + the OLED module (size depends on the SKU: Waveshare 0.95" RGB OLED (A) is 31.7 × 37 mm; the generic 7-pin module is smaller and is the target, measured at the CDR) + modules stacked; desk-sized |
| Wall thickness | 1.6 mm (4 perimeters at 0.4 mm) | stiffness, touch sensing through the top (≤ 2 mm) |
| Front face | OLED window 22.2 × 15.5 mm (the panel's viewing area, 22.14 × 15.42 mm; the active area is 20.14 × 13.42 mm), 1 mm recess for the 25.7 × 22.2 × 1.45 mm glass | eyes centred, no visible PCB, ≥ 0.2 mm print tolerance kept |
| Top face | touch zone marked by a shallow 15 × 15 mm dimple; copper pad glued inside | REQ-F-03 |
| Back face | USB-C cut-out 9.5 × 4 mm aligned with the XIAO connector; 4 horizontal sound slots 8 × 1 mm at the bottom right (x 29.5–37.5, z 2.2–8.3, 0.7 mm ribs), clear of the XIAO, the rounded corner and the cover; the buzzer sits behind them | power, sound |
| Side faces | 3 × Ø1.2 mm holes in front of the microphone port | sound entry, keep dust out |
| Bottom | flat, 4 silicone feet; V3 reserve: Ø 40 mm recess for a Qi receiver coil, 1 mm thick, blocked off in V1 | ADR-0002 |
| Split | front shell 12 mm deep (holds the OLED) + rear shell (holds the XIAO and modules); snap-fit, 4 cantilever hooks on the rear shell, one per side wall (6 mm wide, 1.0 mm thick, 4.5 mm past the split so they stop short of the OLED module), 0.5 mm barbs (lead-in ramp, 45° back face that retains but releases under a firm pull, so the cube can be opened for service) in pockets of the front shell: 0.3 mm engagement, 2.2 % peak strain (PLA limit taken as 2.5 %) | REQ-NF-11, no screws |
| Expansion bay (ADR-0016) | inside the rear shell: free volume 22 × 22 × 6 mm (one Qwiic module up to 20 × 20 mm, laid flat on a foam pad) with J1 (JST-SH, horizontal, 6 × 4.3 mm) reachable from the bay and ≥ 8 mm of free space in front of its mouth for a 50 mm Qwiic cable; a removable 1 mm cover plate sits flush in a rebate of the rear wall (1 mm ledges above and below the hole stop it from being pushed in) and is clipped by 2 cantilever hooks 6 mm long with the same releasable barbs on the vertical edges of the hole (0.3 mm engagement, 1.3 % strain); a 6 × 1 mm fingernail recess in the wall above the plate lifts it out | REQ-F-20, UC12; no screws (REQ-NF-11) |
| Internal | XIAO on a 2 mm rail, USB-C flush; modules on 3M foam pads; the OLED glass sits in its recess and the module PCB is held against the inner front wall by a ring of 3M double-sided foam tape (the recess only stops forward motion); wire channel | assembly ≤ 45 min |

## Printing

| Parameter | Value |
|-----------|-------|
| Material | PLA (colour of the cube's identity or neutral white) |
| Layer height | 0.2 mm, no supports (overhangs ≤ 45°, window bridging ≤ 20 mm) |
| Orientation | outer face on the bed, open side and hooks pointing up (detail below) |
| Tolerance | 0.2 mm on snap hooks; test coupon printed first (`hardware/case/PRINT.md`) |
| Orientation detail | front shell front face down (the window is a through hole, the glass recess and barb pockets open upwards; no ceiling to bridge); rear shell back face down (the hooks point up, barbs are ramps); rear cover outer face down |

## V2 / V3 reserves

- V2: battery pocket 35 × 25 × 8 mm under the modules (shell height +6 mm) and a slide switch
  cut-out on the back; separate revision of the script with a `battery=True` parameter.
- Expansion bay: script parameters `BAY_W, BAY_H, BAY_DEPTH = 22, 22, 6`, `J1_CLEARANCE = 8`; the cover
  always has 2 hooks (one per vertical edge), so it is not a parameter.
  The bay fits inside the 40 mm envelope (owner decision 2026-10-19, MCDA 460/500 against a 46 mm
  shell): XIAO on its rail at the bottom, bay above it (z 10 to 32) against the rear cover, about
  26 mm of free depth (y 6.6 to 32.4) between the bay and the OLED module. The OLED module size
  (`OLED_MODULE = (30, 27, 5)`) is an estimate; the scripted collision checks re-run when the CDR
  measures it, and growing the shell is then a parameter change.
- V3: the Qi coil recess becomes open; receiver board under the coil, 5 V to the XIAO input.

## Deliverables

- `hardware/case/cubbies_case.py` (parametric, all values above as named parameters).
- `hardware/case/export/cubbies-front.stl`, `cubbies-rear.stl`, `cubbies-rear-cover.stl`,
  `cubbies.step`, `cubbies.FCStd`.
- `hardware/case/README.md`: how to regenerate with FreeCAD 1.1 (GUI or `freecadcmd`), which
  also runs the dimension checks.
