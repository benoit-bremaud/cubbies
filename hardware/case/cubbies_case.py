"""cubbies V1 enclosure — parametric FreeCAD script (ADR-0010, CC BY-SA 4.0).

This script is the source of truth; everything under export/ is generated from it.
Regenerate headless (FreeCAD 1.1):

    freecadcmd hardware/case/cubbies_case.py

Frame: x = width (left to right seen from the front), y = depth (front face at y = 0,
back face at y = SIZE), z = height (floor at z = 0). Dimensions in mm; rationale in
docs/hardware/mechanical-spec.md. The script ends with the dimension checks of the
verification plan and exits non-zero if one fails, before writing anything.
"""

from __future__ import annotations

import os
import sys

import FreeCAD as App
import MeshPart
import Part

V = App.Vector

# --- Envelope and shells ---------------------------------------------------------------
SIZE = 40.0  # outer cube edge
CORNER_RADIUS = 2.0
WALL = 1.6  # 4 perimeters at 0.4 mm; also the TTP223 sensing wall
TOL = 0.2  # print clearance on every mating feature
SPLIT_Y = 12.0  # front shell depth; the rear shell takes the rest

# --- Shell snap-fit: 4 cantilever hooks on the rear shell, one per side wall -----------
HOOK_WIDTH = 6.0
HOOK_THICKNESS = 1.2
HOOK_LENGTH = 4.5  # beyond the split plane; stops 0.9 mm short of the OLED module
BARB = 0.6  # barb height (interference with the mating wall)
BARB_LENGTH = 1.5

# --- Front face: OLED ------------------------------------------------------------------
EYE_Z = 24.0  # centre height of the eyes
OLED_WINDOW = (22.2, 15.5)  # panel viewing area 22.14 x 15.42
OLED_GLASS = (25.7, 22.2, 1.45)  # glass recess from the inside, 1 mm deep
OLED_RECESS_DEPTH = 1.0
OLED_MODULE = (30.0, 27.0, 5.0)  # generic 7-pin module + soldered wires (estimate, CDR)

# --- Top face: TTP223 touch zone -------------------------------------------------------
TOUCH_DIMPLE = (15.0, 15.0, 0.4)  # marks the pad; copper pad glued inside
TOUCH_CENTER_Y = 22.0

# --- XIAO ESP32-C3 on a rail, USB-C flush with the back face ---------------------------
XIAO = (17.8, 21.0, 4.5)  # PCB 1.2 + USB-C receptacle 3.2, rounded up
RAIL_HEIGHT = 2.0
RAIL_WIDTH = 1.5
USB_C = (9.5, 4.0)
USB_CENTER_Z = WALL + RAIL_HEIGHT + 1.2 + 1.6

# --- Back face: buzzer vent and microphone port ---------------------------------------
VENT_SLOT = (1.0, 8.0)
VENT_COUNT = 4
VENT_PITCH = 1.8
VENT_X0 = 33.6  # first slot, right of the rear cover
VENT_Z0 = 12.0
MIC_HOLE_D = 1.2
MIC_HOLES = 3
MIC_PITCH = 2.4
MIC_CENTER = (30.0, 20.0)  # (y, z) on the left side face

# --- Expansion bay behind the clipped rear cover (ADR-0016) ----------------------------
BAY = (22.0, 22.0, 6.0)  # width, height, depth (free volume)
BAY_Z0 = 10.0
J1 = (6.0, 4.3)  # JST-SH horizontal, mouth facing into the bay
J1_CLEARANCE = 8.0  # free space in front of the J1 mouth for a 50 mm Qwiic cable
REAR_COVER_HOOKS = 2  # one on each vertical edge
NOTCH = (6.0, 1.0, 0.8)  # fingernail notch at the top edge of the cover

EXPORT_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "export")


def box(
    sx: float, sy: float, sz: float, x: float = 0.0, y: float = 0.0, z: float = 0.0
) -> Part.Shape:
    return Part.makeBox(sx, sy, sz, V(x, y, z))


def centered_box(
    sx: float, sy: float, sz: float, cx: float, cy: float, cz: float
) -> Part.Shape:
    return box(sx, sy, sz, cx - sx / 2, cy - sy / 2, cz - sz / 2)


def wedge_barb(
    width: float,
    length: float,
    height: float,
    x: float,
    y: float,
    z: float,
    outward: App.Vector,
) -> Part.Shape:
    """Triangular barb (a ramp, printable without supports).

    The profile lies in the y/outward plane and is extruded along `width`; `outward`
    is the unit vector pointing from the hook arm into the mating wall.
    """
    along = V(1, 0, 0) if outward.x == 0 else V(0, 0, 1)
    base = V(x, y, z)
    pts = [base, base + V(0, length, 0), base + outward * height, base]
    face = Part.Face(Part.makePolygon(pts))
    return face.extrude(along * width)


def hollow_cube() -> Part.Shape:
    outer = box(SIZE, SIZE, SIZE)
    outer = outer.makeFillet(CORNER_RADIUS, outer.Edges)
    inner = box(SIZE - 2 * WALL, SIZE - 2 * WALL, SIZE - 2 * WALL, WALL, WALL, WALL)
    return outer.cut(inner)


def bay_box() -> Part.Shape:
    """Free volume of the bay, against the inner face of the rear cover."""
    x0 = (SIZE - BAY[0]) / 2
    return box(BAY[0], BAY[2], BAY[1], x0, SIZE - WALL - BAY[2], BAY_Z0)


def cover_opening() -> Part.Shape:
    """Rear-wall opening: the bay footprint plus room for the two cover hooks."""
    width = BAY[0] + 2 * (HOOK_THICKNESS + TOL)
    return box(width, WALL, BAY[1], (SIZE - width) / 2, SIZE - WALL, BAY_Z0)


def around_y_axis(shape: Part.Shape) -> list[Part.Shape]:
    """The shape and its copies turned 90, 180 and 270 degrees about the cube's y axis.

    Features are modelled once on the left wall; the square section makes the turned
    copies land exactly on the bottom, right and top walls.
    """
    centre, axis = V(SIZE / 2, 0, SIZE / 2), V(0, 1, 0)
    return [shape.copy().rotate(centre, axis, angle) for angle in (0, 90, 180, 270)]


def shell_hooks() -> list[Part.Shape]:
    """Four hook arms (with barbs) on the rear shell, reaching into the front shell."""
    inset = WALL + TOL  # arm outer face sits TOL inside the front shell wall
    y0 = SPLIT_Y - HOOK_LENGTH
    z0 = SIZE / 2 - HOOK_WIDTH / 2
    arm = box(HOOK_THICKNESS, HOOK_LENGTH + WALL, HOOK_WIDTH, inset, y0, z0)
    barb = wedge_barb(HOOK_WIDTH, BARB_LENGTH, BARB, inset, y0 + 0.5, z0, V(-1, 0, 0))
    # Root block tying the arm to the rear shell wall, behind the split plane.
    root = box(TOL + HOOK_THICKNESS, WALL, HOOK_WIDTH, WALL, SPLIT_Y, z0)
    return around_y_axis(arm.fuse(barb).fuse(root))


def hook_pockets() -> list[Part.Shape]:
    """Pockets in the front shell walls receiving the barbs, TOL all around."""
    depth = BARB + TOL
    w = HOOK_WIDTH + 2 * TOL
    length = BARB_LENGTH + 2 * TOL
    y = SPLIT_Y - HOOK_LENGTH + 0.5 - TOL
    return around_y_axis(box(depth, length, w, WALL - depth, y, SIZE / 2 - w / 2))


def front_shell(cube: Part.Shape) -> Part.Shape:
    shell = cube.common(box(SIZE, SPLIT_Y, SIZE))
    c = SIZE / 2
    window = centered_box(OLED_WINDOW[0], WALL * 3, OLED_WINDOW[1], c, WALL / 2, EYE_Z)
    glass = centered_box(
        OLED_GLASS[0] + 2 * TOL,
        OLED_RECESS_DEPTH,
        OLED_GLASS[1] + 2 * TOL,
        c,
        WALL - OLED_RECESS_DEPTH / 2,
        EYE_Z,
    )
    for tool in [window, glass, *hook_pockets()]:
        shell = shell.cut(tool)
    return shell


def rear_shell(cube: Part.Shape) -> Part.Shape:
    shell = cube.common(box(SIZE, SIZE - SPLIT_Y, SIZE, 0, SPLIT_Y, 0))
    c = SIZE / 2
    # XIAO rails on the floor, under the board edges, board flush with the back wall.
    xiao_y0 = SIZE - WALL - XIAO[1]
    for x in (c - XIAO[0] / 2, c + XIAO[0] / 2 - RAIL_WIDTH):
        shell = shell.fuse(box(RAIL_WIDTH, XIAO[1], RAIL_HEIGHT, x, xiao_y0, WALL))
    for hook in shell_hooks():
        shell = shell.fuse(hook)
    usb = centered_box(USB_C[0], WALL * 3, USB_C[1], c, SIZE - WALL / 2, USB_CENTER_Z)
    dimple = centered_box(
        TOUCH_DIMPLE[0], TOUCH_DIMPLE[1], TOUCH_DIMPLE[2] * 2, c, TOUCH_CENTER_Y, SIZE
    )
    tools = [usb, dimple, cover_opening()]
    for i in range(VENT_COUNT):
        x = VENT_X0 + i * VENT_PITCH
        tools.append(
            box(VENT_SLOT[0], WALL * 3, VENT_SLOT[1], x, SIZE - 2 * WALL, VENT_Z0)
        )
    first = -(MIC_HOLES - 1) / 2 * MIC_PITCH
    for i in range(MIC_HOLES):
        z = MIC_CENTER[1] + first + i * MIC_PITCH
        hole = Part.makeCylinder(
            MIC_HOLE_D / 2, WALL * 3, V(-WALL, MIC_CENTER[0], z), V(1, 0, 0)
        )
        tools.append(hole)
    for tool in tools:
        shell = shell.cut(tool)
    return shell.removeSplitter()


def rear_cover(cube: Part.Shape) -> Part.Shape:
    """Plate filling the rear-wall opening, held by hooks on its two vertical edges."""
    bb = cover_opening().BoundBox
    plate = cube.common(
        box(
            bb.XLength - 2 * TOL,
            WALL,
            bb.ZLength - 2 * TOL,
            bb.XMin + TOL,
            SIZE - WALL,
            bb.ZMin + TOL,
        )
    )
    arm_len = 2.0
    z = BAY_Z0 + BAY[1] / 2 - HOOK_WIDTH / 2
    y = SIZE - WALL - arm_len
    sides = (V(-1, 0, 0), V(1, 0, 0))[:REAR_COVER_HOOKS]
    for out in sides:
        x = bb.XMin + TOL if out.x < 0 else bb.XMax - TOL - HOOK_THICKNESS
        arm = box(HOOK_THICKNESS, arm_len, HOOK_WIDTH, x, y, z)
        bx = x if out.x < 0 else x + HOOK_THICKNESS
        plate = plate.fuse(arm).fuse(
            wedge_barb(HOOK_WIDTH, BARB_LENGTH, BARB, bx, y, z, out)
        )
    notch = centered_box(
        NOTCH[0], NOTCH[2] * 2, NOTCH[1] * 2, SIZE / 2, SIZE, bb.ZMax - TOL
    )
    return plate.cut(notch).removeSplitter()


# --- Dimension checks (verification plan, mechanical row) --------------------------------
def _fits(shape: Part.Shape, inner: Part.BoundBox) -> bool:
    loose = App.BoundBox(inner)
    loose.enlarge(1e-6)  # parts may rest against a wall face
    return loose.isInside(shape.BoundBox)


def run_checks(front: Part.Shape, rear: Part.Shape, cover: Part.Shape) -> list[str]:
    """Return the failed checks (empty when every check passes)."""
    failures: list[str] = []

    def check(ok: bool, label: str) -> None:
        App.Console.PrintMessage(("PASS " if ok else "FAIL ") + label + "\n")
        if not ok:
            failures.append(label)

    parts = {"front": front, "rear": rear, "cover": cover}
    for name, part in parts.items():
        check(part.isValid() and part.Volume > 0, f"{name}: valid solid")

    env = Part.makeCompound(list(parts.values())).BoundBox
    check(
        all(abs(d - SIZE) < 0.01 for d in (env.XLength, env.YLength, env.ZLength)),
        f"envelope {env.XLength:.2f} x {env.YLength:.2f} x {env.ZLength:.2f} = {SIZE} mm",
    )

    # Wall: a point at mid-wall is solid, a point just past the wall is air (all 6 faces,
    # probed away from openings).
    c = SIZE / 2
    probes = [
        (front, V(c, WALL / 2, 8.0), V(c, WALL + 0.1, 8.0)),
        (rear, V(c, SIZE - WALL / 2, 36.0), V(c, SIZE - WALL - 0.1, 36.0)),
        (rear, V(WALL / 2, 25.0, 8.0), V(WALL + 0.1, 25.0, 8.0)),
        (rear, V(SIZE - WALL / 2, 25.0, 30.0), V(SIZE - WALL - 0.1, 25.0, 30.0)),
        (rear, V(8.0, 25.0, WALL / 2), V(8.0, 25.0, WALL + 0.1)),
        (rear, V(8.0, 32.0, SIZE - WALL / 2), V(8.0, 32.0, SIZE - WALL - 0.1)),
    ]
    wall_ok = all(
        s.isInside(p_in, 1e-6, True) and not s.isInside(p_out, 1e-6, True)
        for s, p_in, p_out in probes
    )
    check(wall_ok, f"wall thickness {WALL} mm on all faces")

    bay = bay_box()
    expected = BAY[0] * BAY[1] * BAY[2]
    intruding = sum(bay.common(p).Volume for p in parts.values())
    check(
        abs(bay.Volume - expected) < 1e-6 and intruding < 1e-6,
        f"bay free volume {bay.Volume:.0f} mm3 ({BAY[0]} x {BAY[1]} x {BAY[2]})",
    )
    check(
        J1_CLEARANCE <= min(BAY[0], BAY[1]) and J1[0] <= BAY[0],
        f"J1 {J1[0]} mm mouth + {J1_CLEARANCE} mm clearance inside the bay",
    )

    ahead = box(SIZE, SPLIT_Y, SIZE)  # the part of each hook inside the front shell
    gap_fr = min(h.common(ahead).distToShape(front)[0] for h in shell_hooks())
    check(gap_fr >= TOL - 1e-6, f"shell hook clearance {gap_fr:.2f} mm >= {TOL} mm")
    check(front.common(rear).Volume < 1e-6, "front and rear shells do not overlap")
    check(
        cover.common(rear).Volume < 1e-6, "rear cover does not overlap the rear shell"
    )
    gap_cv = cover.distToShape(rear)[0]
    check(gap_cv >= TOL - 1e-6, f"rear cover clearance {gap_cv:.2f} mm >= {TOL} mm")

    # Internal parts as bounding boxes: no part may hit a shell, a hook or the bay.
    inner = App.BoundBox(WALL, WALL, WALL, SIZE - WALL, SIZE - WALL, SIZE - WALL)
    oled = centered_box(
        OLED_MODULE[0],
        OLED_MODULE[2],
        OLED_MODULE[1],
        c,
        WALL + OLED_MODULE[2] / 2,
        EYE_Z,
    )
    xiao = box(
        XIAO[0],
        XIAO[1],
        XIAO[2],
        c - XIAO[0] / 2,
        SIZE - WALL - XIAO[1],
        WALL + RAIL_HEIGHT,
    )
    for name, item in {"OLED module": oled, "XIAO": xiao}.items():
        hit = (
            sum(item.common(p).Volume for p in parts.values()) + item.common(bay).Volume
        )
        check(_fits(item, inner) and hit < 1e-6, f"{name} clears shells, hooks and bay")
    return failures


def export(front: Part.Shape, rear: Part.Shape, cover: Part.Shape) -> None:
    os.makedirs(EXPORT_DIR, exist_ok=True)
    doc = App.newDocument("cubbies")
    shapes = {"cubbies-front": front, "cubbies-rear": rear, "cubbies-rear-cover": cover}
    for name, shape in shapes.items():
        doc.addObject("Part::Feature", name.replace("-", "_")).Shape = shape
        mesh = MeshPart.meshFromShape(
            Shape=shape, LinearDeflection=0.02, AngularDeflection=0.2
        )
        mesh.write(os.path.join(EXPORT_DIR, name + ".stl"))
    Part.makeCompound(list(shapes.values())).exportStep(
        os.path.join(EXPORT_DIR, "cubbies.step")
    )
    doc.recompute()
    fcstd = os.path.join(EXPORT_DIR, "cubbies.FCStd")
    if os.path.exists(fcstd):
        os.remove(fcstd)  # overwriting would leave a .FCBak backup next to it
    doc.saveAs(fcstd)


def main() -> None:
    cube = hollow_cube()
    front, rear, cover = front_shell(cube), rear_shell(cube), rear_cover(cube)
    failures = run_checks(front, rear, cover)
    if failures:
        App.Console.PrintError(f"{len(failures)} check(s) failed; nothing exported.\n")
        sys.exit(1)
    if os.environ.get("CUBBIES_CHECK_ONLY") != "1":
        export(front, rear, cover)
        App.Console.PrintMessage(f"Exported to {EXPORT_DIR}\n")


main()
