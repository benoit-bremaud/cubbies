# ADR-0010: Enclosure — FreeCAD driven by a versioned Python script, MCP for inspection

**Status:** Accepted — 2026-10-14

## Context

The enclosure must be reviewable in pull requests like code, reproducible, and editable by the
owner in a GUI. Facts verified on 2026-10-14: the `.FCStd` format is a zip of XML and BREP, so
git cannot diff it and every revision is a full copy; FreeCAD 1.1 is available as a snap on this
machine (not installed yet, sudo needed); the `freecad-mcp` server (MIT, active, addon + `uvx`
server) lets the agent drive FreeCAD and take screenshots; FreeCAD is fully scriptable in Python;
OpenSCAD in the distribution is the 2021 release (no fillets, no STEP); build123d offers Python
code-CAD without FreeCAD and without a native GUI.

## Options and scoring

Criteria (note: "driveable by the agent" carries the highest weight because the owner wants the
enclosure iterated from this workflow; it is a tooling convenience, not a product force, and a
future reader may re-weigh it): driveable by the agent (3), text diff in PR (2), owner's GUI preference (2),
fillets/STEP for V2–V3 (2), maturity here (2).

| Criterion (weight) | A. FreeCAD GUI + MCP | B. OpenSCAD 2021 | C. FreeCAD GUI by owner | D. FreeCAD scripted + MCP | E. build123d |
|---|---|---|---|---|---|
| Driveable (3) | 3 | 3 | 1 | 3 | 3 |
| Text diff (2) | 1 | 3 | 1 | 3 | 3 |
| GUI preference (2) | 3 | 1 | 3 | 3 | 1 |
| Fillets / STEP (2) | 3 | 1 | 3 | 3 | 3 |
| Maturity (2) | 2 | 2 | 3 | 2 | 2 |
| **Total /33** | 27 | 23 | 23 | **31** | 27 |

## Decision

**Option D.** The source of truth is `hardware/case/cubbies_case.py`, a parametric FreeCAD
script (all dimensions as named parameters). It generates the `.FCStd`, `.step` and `.stl`
files, committed as artefacts for printing and sharing but never hand-edited. The
`freecad-mcp` server is declared in the project's `.mcp.json` for inspection, screenshots and
iteration; the owner opens the generated model in FreeCAD 1.1.

## Consequences

- Install: FreeCAD 1.1 snap (owner, sudo), the MCP addon in `~/.local/share/FreeCAD/Mod/`, and
  `uvx freecad-mcp` in `.mcp.json`.
- A regenerate step (`freecadcmd cubbies_case.py`) is documented in `hardware/case/README.md`;
  CI does not run FreeCAD.
- Reusing third-party models stays possible as imported STEP files with their own licence credit.
