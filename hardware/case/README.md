# Enclosure (CC BY-SA 4.0)

Source of truth: `cubbies_case.py`, a parametric FreeCAD script (ADR-0010). The `.FCStd`,
`.step` and `.stl` files under `export/` are generated from it and committed for printing and
sharing; never edit them by hand.

## Regenerate

```bash
freecadcmd cubbies_case.py
```

Requires FreeCAD 1.1 (snap: `sudo snap install freecad`). The script writes to `export/`.
Dimensions and their rationale: `docs/hardware/mechanical-spec.md`.

## Agent tooling

The `freecad-mcp` server (declared in the repository's `.mcp.json`, addon installed in
`~/.local/share/FreeCAD/Mod/FreeCADMCP`) is used for inspection and screenshots during design
iterations; it is not needed to regenerate the files.

## Status

Script not written yet: it starts after the mechanical CDR inputs (module dimensions measured
on the prototype parts).
