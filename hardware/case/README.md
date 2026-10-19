# Enclosure (CC BY-SA 4.0)

Source of truth: `cubbies_case.py`, a parametric FreeCAD script (ADR-0010). The `.FCStd`,
`.step` and `.stl` files under `export/` are generated from it and committed for printing and
sharing; never edit them by hand.

## Regenerate

```bash
freecadcmd hardware/case/cubbies_case.py
```

Requires FreeCAD 1.1. On the owner's machine (decision 2026-10-19, MCDA 440/500 against the
snap at 275): the FreeCAD 1.1.1 AppImage in `~/Applications`, called through a `freecadcmd`
wrapper in `~/.local/bin` (`exec ~/Applications/FreeCAD-1.1.1.AppImage freecadcmd "$@"`): no
sudo, a pinned version so the parts do not change under an automatic update, and driveable
headless by an agent. The snap (`sudo snap install freecad`, command `freecad.cmd`) works too.

The script first runs 17 dimension checks (envelope, walls, openings, bay, hook clearance,
barb direction and engagement, hook strain, cover stop, part collisions), prints `PASS`/`FAIL`
per check through the FreeCAD console (plain `print` is not shown by `freecadcmd`) and exits
non-zero without exporting if one fails; set `CUBBIES_CHECK_ONLY=1` to run the checks only. It
then writes to `export/`:

| File | Use |
|------|-----|
| `cubbies-front.stl` | front shell, print front face up |
| `cubbies-rear.stl` | rear shell, print back face down |
| `cubbies-rear-cover.stl` | module bay cover, print outer face down (hooks up) |
| `cubbies.step` | all three parts, for other CAD tools |
| `cubbies.FCStd` | FreeCAD document to inspect in the GUI |

Every dimension is a named parameter at the top of the script; rationale in
`docs/hardware/mechanical-spec.md`. CI does not run FreeCAD (ADR-0010).

## Agent tooling

The `freecad-mcp` server (declared in the repository's `.mcp.json`, addon installed in
`~/.local/share/FreeCAD/Mod/FreeCADMCP`) is used for inspection and screenshots during design
iterations; it is not needed to regenerate the files.

## Status

Script written with the expansion bay and clipped rear cover of ADR-0016; all checks green. The
OLED module and XIAO sizes are estimates until measured on the prototype parts; print-and-fit is
the mechanical CDR bench step.
