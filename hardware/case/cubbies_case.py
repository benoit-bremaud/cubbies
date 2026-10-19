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
import Mesh
import MeshPart
import Part

V = App.Vector

# --- Envelope and shells ---------------------------------------------------------------
SIZE = 40.0  # outer cube edge
CORNER_RADIUS = 2.0
WALL = 1.6  # 4 perimeters at 0.4 mm; also the TTP223 sensing wall
TOL = 0.2  # print clearance on every mating feature
SPLIT_Y = 12.0  # front shell depth; the rear shell takes the rest

# --- Snap hooks (4 on the rear shell, one per side wall; 2 on the rear cover) ----------
HOOK_WIDTH = 6.0
HOOK_THICKNESS = 1.0
HOOK_LENGTH = 4.5  # shell hooks, past the split; stops 0.9 mm short of the OLED module
COVER_HOOK_LENGTH = 6.0  # rear cover hooks, from the plate's inner face
BARB = 0.5  # barb height; it engages the mating edge by BARB - TOL
BARB_LENGTH = 1.5  # lead-in ramp; the 45-degree back face (BARB long) retains but
# releases under a firm pull, so shells and cover can be opened again
BARB_OFFSET = 0.5  # shell hooks: ramp start, measured from the arm tip
PLA_STRAIN_LIMIT = 0.025  # printed PLA breaks around 2-3 % strain

# --- Front face: OLED ------------------------------------------------------------------
EYE_Z = 24.0  # centre height of the eyes
OLED_WINDOW = (22.2, 15.5)  # (x, z): panel viewing area 22.14 x 15.42
OLED_GLASS = (25.7, 22.2)  # (x, z): glass seated in a recess from the inside
OLED_RECESS_DEPTH = 1.0
OLED_MODULE = (30.0, 27.0, 5.0)  # (x, z, y): 7-pin module + soldered wires, estimate

# --- Top face: TTP223 touch zone -------------------------------------------------------
TOUCH_DIMPLE = (15.0, 15.0, 0.4)  # (x, y, depth): marks the pad glued inside
TOUCH_CENTER_Y = 22.0

# --- XIAO ESP32-C3 on two rails, USB-C flush with the back face ------------------------
XIAO = (17.8, 21.0, 4.5)  # (x, y, z): PCB + USB-C receptacle, rounded up
XIAO_PCB = 1.2
RAIL_HEIGHT = 2.0
RAIL_WIDTH = 1.5
USB_C = (9.5, 4.0)  # (x, z) opening
USB_C_HEIGHT = 3.2  # receptacle body, sits on the PCB
USB_CENTER_Z = WALL + RAIL_HEIGHT + XIAO_PCB + USB_C_HEIGHT / 2

# --- Back face: buzzer vent (bottom right, under the cover); left face: microphone -----
VENT_SLOT = (8.0, 1.0)  # (x, z): horizontal slots
VENT_COUNT = 4
VENT_PITCH = 1.7  # leaves 0.7 mm ribs
VENT_X0 = 29.5  # clear of the XIAO (ends at x 28.9) and of the rounded corner
VENT_Z0 = 2.2
MIC_HOLE_D = 1.2
MIC_HOLES = 3
MIC_PITCH = 2.4
MIC_CENTER_Y = 30.0
MIC_CENTER_Z = 20.0

# --- Expansion bay behind the clipped rear cover (ADR-0016) ----------------------------
BAY_W, BAY_H, BAY_DEPTH = 22.0, 22.0, 6.0  # free volume (x, z, y)
BAY_Z0 = 10.0
J1_MOUTH = 6.0  # JST-SH horizontal, mouth width
J1_CLEARANCE = 8.0  # free space in front of the J1 mouth for a 50 mm Qwiic cable
COVER_THICKNESS = 1.0  # plate seated in a rebate, flush with the back face
COVER_LEDGE = 1.0  # rebate ledge above and below the hole: the push-in stop
NOTCH_W, NOTCH_H = 6.0, 1.0  # fingernail recess in the wall above the cover

EXPORT_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "export")
PRINT_DIR = os.path.join(EXPORT_DIR, "print")
COUPON_MARGIN = 3.0  # wall kept around the sampled hook, pocket and rebate
PLATE_GAP = 5.0  # spacing between parts on the 3MF plates


def box(
    sx: float, sy: float, sz: float, x: float = 0.0, y: float = 0.0, z: float = 0.0
) -> Part.Shape:
    return Part.makeBox(sx, sy, sz, V(x, y, z))


def centered_box(
    sx: float, sy: float, sz: float, cx: float, cy: float, cz: float
) -> Part.Shape:
    return box(sx, sy, sz, cx - sx / 2, cy - sy / 2, cz - sz / 2)


def barb(x: float, y: float, z: float, outward: float) -> Part.Shape:
    """Releasable barb on a hook arm, extruded along z over HOOK_WIDTH.

    (x, y, z) is the ramp start on the arm face; the ramp rises towards +y to BARB in
    the direction outward (-1 or +1 along x), so it leads when the part moves towards
    -y. The back face drops at 45 degrees over BARB: it retains, and a firm pull
    deflects the arm so the part comes apart without breaking a hook. No support
    needed when printed arm up.
    """
    start = V(x, y, z)
    peak = start + V(outward * BARB, BARB_LENGTH, 0)
    end = start + V(0, BARB_LENGTH + BARB, 0)
    face = Part.Face(Part.makePolygon([start, peak, end, start]))
    return face.extrude(V(0, 0, HOOK_WIDTH))


def strain(length: float, deflection: float) -> float:
    """Peak strain of a rectangular cantilever hook (1.5 t d / L^2)."""
    return 1.5 * HOOK_THICKNESS * deflection / length**2


def hollow_cube() -> Part.Shape:
    """The 40 mm cube with rounded edges, hollowed to WALL; both shells are cut from it."""
    outer = box(SIZE, SIZE, SIZE)
    outer = outer.makeFillet(CORNER_RADIUS, outer.Edges)
    inner = box(SIZE - 2 * WALL, SIZE - 2 * WALL, SIZE - 2 * WALL, WALL, WALL, WALL)
    return outer.cut(inner)


def bay_box() -> Part.Shape:
    """Free volume of the bay, against the inner face of the rear wall."""
    return box(
        BAY_W, BAY_DEPTH, BAY_H, (SIZE - BAY_W) / 2, SIZE - WALL - BAY_DEPTH, BAY_Z0
    )


def xiao_box() -> Part.Shape:
    """The XIAO on its rails, USB-C against the rear wall."""
    return box(*XIAO, (SIZE - XIAO[0]) / 2, SIZE - WALL - XIAO[1], WALL + RAIL_HEIGHT)


def cover_hole() -> Part.Shape:
    """Through hole in the rear wall: the bay footprint plus the two cover hooks."""
    width = BAY_W + 2 * (HOOK_THICKNESS + 2 * TOL)
    return box(width, WALL, BAY_H, (SIZE - width) / 2, SIZE - WALL, BAY_Z0)


def cover_rebate() -> Part.Shape:
    """Outer pocket the plate sits in; its ledges above and below the hole stop it."""
    bb = cover_hole().BoundBox
    return box(
        bb.XLength,
        COVER_THICKNESS,
        bb.ZLength + 2 * COVER_LEDGE,
        bb.XMin,
        SIZE - COVER_THICKNESS,
        bb.ZMin - COVER_LEDGE,
    )


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
    tooth = barb(inset, y0 + BARB_OFFSET, z0, -1)
    # Root block tying the arm to the rear shell wall, behind the split plane.
    root = box(TOL + HOOK_THICKNESS, WALL, HOOK_WIDTH, WALL, SPLIT_Y, z0)
    return around_y_axis(arm.fuse(tooth).fuse(root))


def hook_pockets() -> list[Part.Shape]:
    """Pockets in the front shell walls receiving the barbs, TOL all around."""
    depth = BARB + TOL
    w = HOOK_WIDTH + 2 * TOL
    length = BARB_LENGTH + BARB + 2 * TOL
    y = SPLIT_Y - HOOK_LENGTH + BARB_OFFSET - TOL
    return around_y_axis(box(depth, length, w, WALL - depth, y, SIZE / 2 - w / 2))


def front_shell(cube: Part.Shape) -> Part.Shape:
    """Front shell: OLED window, glass recess and the four barb pockets."""
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


def vent_slots() -> list[Part.Shape]:
    """Buzzer slots through the back wall only (they stop at its inner face)."""
    return [
        box(
            VENT_SLOT[0],
            WALL,
            VENT_SLOT[1],
            VENT_X0,
            SIZE - WALL,
            VENT_Z0 + i * VENT_PITCH,
        )
        for i in range(VENT_COUNT)
    ]


def mic_holes() -> list[Part.Shape]:
    """Microphone holes through the left wall, centred on (MIC_CENTER_Y, MIC_CENTER_Z)."""
    first = MIC_CENTER_Z - (MIC_HOLES - 1) / 2 * MIC_PITCH
    return [
        Part.makeCylinder(
            MIC_HOLE_D / 2,
            WALL * 3,
            V(-WALL, MIC_CENTER_Y, first + i * MIC_PITCH),
            V(1, 0, 0),
        )
        for i in range(MIC_HOLES)
    ]


def rear_shell(cube: Part.Shape) -> Part.Shape:
    """Rear shell: XIAO rails and hooks, then every opening of the back and side walls."""
    shell = cube.common(box(SIZE, SIZE - SPLIT_Y, SIZE, 0, SPLIT_Y, 0))
    c = SIZE / 2
    xiao = xiao_box().BoundBox
    for x in (xiao.XMin, xiao.XMax - RAIL_WIDTH):
        shell = shell.fuse(box(RAIL_WIDTH, XIAO[1], RAIL_HEIGHT, x, xiao.YMin, WALL))
    for hook in shell_hooks():
        shell = shell.fuse(hook)
    usb = centered_box(USB_C[0], WALL * 3, USB_C[1], c, SIZE - WALL / 2, USB_CENTER_Z)
    # Centred on the top face, so half the box height is the cut depth.
    dimple = centered_box(
        TOUCH_DIMPLE[0], TOUCH_DIMPLE[1], TOUCH_DIMPLE[2] * 2, c, TOUCH_CENTER_Y, SIZE
    )
    rebate = cover_rebate().BoundBox
    notch = box(
        NOTCH_W, COVER_THICKNESS, NOTCH_H, c - NOTCH_W / 2, rebate.YMin, rebate.ZMax
    )
    tools = [
        usb,
        dimple,
        cover_hole(),
        cover_rebate(),
        notch,
        *vent_slots(),
        *mic_holes(),
    ]
    for tool in tools:
        shell = shell.cut(tool)
    return shell.removeSplitter()


def rear_cover(cube: Part.Shape) -> Part.Shape:
    """Plate seated in the rebate, held by a hook on each vertical edge of the hole."""
    rebate = cover_rebate().BoundBox
    hole = cover_hole().BoundBox
    plate = cube.common(
        box(
            rebate.XLength - 2 * TOL,
            COVER_THICKNESS,
            rebate.ZLength - 2 * TOL,
            rebate.XMin + TOL,
            rebate.YMin,
            rebate.ZMin + TOL,
        )
    )
    z = BAY_Z0 + BAY_H / 2 - HOOK_WIDTH / 2
    y = rebate.YMin - COVER_HOOK_LENGTH
    # The 45-degree back face ends on the wall's inner face plane, so it meets the
    # hole edge after a short pull (axial play) and releases under a firm one.
    ramp_y = SIZE - WALL - BARB - BARB_LENGTH
    for outward in (-1, 1):
        x = hole.XMin + TOL if outward < 0 else hole.XMax - TOL - HOOK_THICKNESS
        arm = box(HOOK_THICKNESS, COVER_HOOK_LENGTH, HOOK_WIDTH, x, y, z)
        face_x = x if outward < 0 else x + HOOK_THICKNESS
        plate = plate.fuse(arm).fuse(barb(face_x, ramp_y, z, outward))
    return plate.removeSplitter()


def coupon(front: Part.Shape, rear: Part.Shape) -> tuple[Part.Shape, Part.Shape]:
    """Tolerance coupon cut from the real shells, so it fits exactly like the cube.

    Front half: the corner of the front shell holding the left barb pocket. Rear half:
    the matching corner with the left hook, plus the frame of the back wall around the
    cover rebate (the real cover is printed with it). Each half prints like its shell.
    """
    m = COUPON_MARGIN
    z0, height = SIZE / 2 - HOOK_WIDTH / 2 - m, HOOK_WIDTH + 2 * m
    corner = WALL + TOL + HOOK_THICKNESS + m
    front_half = front.common(box(corner, SPLIT_Y, height, 0, 0, z0))
    hook_y0 = SPLIT_Y - HOOK_LENGTH
    rebate = cover_rebate().BoundBox
    frame = box(
        rebate.XLength + 2 * m,
        WALL,
        rebate.ZLength + 2 * m,
        rebate.XMin - m,
        SIZE - WALL,
        rebate.ZMin - m,
    )
    rear_corner = box(corner, SIZE - hook_y0, height, 0, hook_y0, z0)
    rear_half = rear.common(rear_corner.fuse(frame))
    return front_half.removeSplitter(), rear_half.removeSplitter()


def on_bed(shape: Part.Shape, face_down: int) -> Part.Shape:
    """The shape turned for printing, resting on z = 0 at the origin.

    face_down -1 puts the front (y = 0) face on the bed, +1 the back (y = SIZE) face.
    """
    turned = shape.copy()
    turned.rotate(V(0, 0, 0), V(1, 0, 0), -90 * face_down)
    bb = turned.BoundBox
    turned.translate(V(-bb.XMin, -bb.YMin, -bb.ZMin))
    return turned


def plate(shapes: list[Part.Shape]) -> list[Part.Shape]:
    """Shapes already on the bed, laid side by side along x, PLATE_GAP apart."""
    placed, x = [], 0.0
    for shape in shapes:
        moved = shape.copy()
        moved.translate(V(x, 0, 0))
        placed.append(moved)
        x += shape.BoundBox.XLength + PLATE_GAP
    return placed


# --- Dimension checks (verification plan, mechanical row) --------------------------------
def _fits(shape: Part.Shape, inner: Part.BoundBox) -> bool:
    """True when the shape's bounding box lies inside `inner` (touching allowed)."""
    loose = App.BoundBox(inner)
    loose.enlarge(1e-6)  # parts may rest against a wall face
    return loose.isInside(shape.BoundBox)


def openings_in_place(front: Part.Shape, rear: Part.Shape) -> bool:
    """Every opening is air at its centre, and the vents stay on the flat back face."""

    def solid(shape: Part.Shape, x: float, y: float, z: float) -> bool:
        return shape.isInside(V(x, y, z), 1e-6, True)

    c, w = SIZE / 2, WALL / 2
    vents = [b.BoundBox for b in vent_slots()]
    mics = [m.BoundBox.Center for m in mic_holes()]
    vents_flat = all(
        b.XMax <= SIZE - CORNER_RADIUS and b.ZMin >= CORNER_RADIUS for b in vents
    )
    return (
        vents_flat
        and not solid(front, c, w, EYE_Z)
        and not solid(rear, c, SIZE - w, USB_CENTER_Z)
        and not any(solid(rear, b.Center.x, SIZE - w, b.Center.z) for b in vents)
        and not any(solid(rear, w, p.y, p.z) for p in mics)
        and not solid(rear, c, TOUCH_CENTER_Y, SIZE - TOUCH_DIMPLE[2] / 2)
        and solid(rear, c, TOUCH_CENTER_Y, SIZE - TOUCH_DIMPLE[2] - 0.1)
    )


def run_checks(
    front: Part.Shape,
    rear: Part.Shape,
    cover: Part.Shape,
    coupon_parts: tuple[Part.Shape, Part.Shape],
) -> list[str]:
    """Return the failed checks (empty when every check passes)."""
    failures: list[str] = []

    def check(ok: bool, label: str) -> None:
        App.Console.PrintMessage(("PASS " if ok else "FAIL ") + label + "\n")
        if not ok:
            failures.append(label)

    def solid(shape: Part.Shape, x: float, y: float, z: float) -> bool:
        return shape.isInside(V(x, y, z), 1e-6, True)

    parts = {"front": front, "rear": rear, "cover": cover}
    for name, part in parts.items():
        check(part.isValid() and part.Volume > 0, f"{name}: valid solid")

    env = Part.makeCompound(list(parts.values())).BoundBox
    check(
        all(abs(d - SIZE) < 0.01 for d in (env.XLength, env.YLength, env.ZLength)),
        f"envelope {env.XLength:.2f} x {env.YLength:.2f} x {env.ZLength:.2f} = {SIZE} mm",
    )

    # Wall: solid at mid-wall, air just inside, on each face. Probes sit in plain wall:
    # below the eyes (front), above the rebate and notch (back), clear of the mic
    # holes (left), vents (right), XIAO rails (bottom) and dimple (top).
    c, w = SIZE / 2, WALL / 2
    back_z = (cover_rebate().BoundBox.ZMax + NOTCH_H + SIZE - WALL) / 2
    probes = [
        (front, (c, w, 6.0), (c, WALL + 0.1, 6.0)),
        (rear, (6.0, SIZE - w, back_z), (6.0, SIZE - WALL - 0.1, back_z)),
        (rear, (w, 20.0, 8.0), (WALL + 0.1, 20.0, 8.0)),
        (rear, (SIZE - w, 20.0, 30.0), (SIZE - WALL - 0.1, 20.0, 30.0)),
        (rear, (6.0, 20.0, w), (6.0, 20.0, WALL + 0.1)),
        (rear, (6.0, 36.0, SIZE - w), (6.0, 36.0, SIZE - WALL - 0.1)),
    ]
    check(
        all(solid(s, *p_in) and not solid(s, *p_out) for s, p_in, p_out in probes),
        f"wall thickness {WALL} mm on all faces",
    )

    check(
        openings_in_place(front, rear),
        "window, USB-C, vents, mic holes, dimple in place",
    )

    bay = bay_box()
    intrusion = sum(bay.common(p).Volume for p in parts.values())
    gap_hooks = cover.distToShape(bay)[0]  # the bay rests against the rear wall
    check(
        _fits(
            bay, App.BoundBox(WALL, WALL, WALL, SIZE - WALL, SIZE - WALL, SIZE - WALL)
        )
        and intrusion < 1e-6
        and gap_hooks >= TOL - 1e-6,
        f"bay {BAY_W} x {BAY_H} x {BAY_DEPTH} free, cover hooks {gap_hooks:.2f} mm away",
    )
    check(
        J1_MOUTH <= BAY_W and J1_CLEARANCE <= BAY_W,
        f"J1 mouth {J1_MOUTH} mm + {J1_CLEARANCE} mm clearance fit the bay (parameters)",
    )

    # Snap fits: the ramp leads on insertion, the step engages, the arm survives.
    ahead = box(SIZE, SPLIT_Y, SIZE)  # the part of each hook inside the front shell
    gap_fr = min(h.common(ahead).distToShape(front)[0] for h in shell_hooks())
    check(gap_fr >= TOL - 1e-6, f"shell hook clearance {gap_fr:.2f} mm >= {TOL} mm")
    hook = shell_hooks()[0]
    ramp_y = SPLIT_Y - HOOK_LENGTH + BARB_OFFSET
    tip_x = WALL + TOL - 0.9 * BARB
    leads = not solid(hook, tip_x, ramp_y + 0.05, c) and solid(
        hook, tip_x, ramp_y + BARB_LENGTH - 0.05, c
    )
    shell_engage = WALL - hook.BoundBox.XMin
    cover_engage = cover_hole().BoundBox.XMin - cover.BoundBox.XMin
    worst = max(
        strain(HOOK_LENGTH, shell_engage), strain(COVER_HOOK_LENGTH, cover_engage)
    )
    peak_y = ramp_y + BARB_LENGTH
    releases = solid(hook, WALL + TOL - BARB / 4, peak_y + BARB / 2, c)
    check(
        leads and releases, "barb ramps lead on insertion, 45-degree back face releases"
    )
    check(
        shell_engage > 0.1 and cover_engage > 0.1,
        f"barbs engage {shell_engage:.2f} mm (shells), {cover_engage:.2f} mm (cover)",
    )
    check(
        worst <= PLA_STRAIN_LIMIT,
        f"hook strain {worst:.1%} <= {PLA_STRAIN_LIMIT:.1%} (PLA)",
    )

    check(front.common(rear).Volume < 1e-6, "front and rear shells do not overlap")
    check(
        cover.common(rear).Volume < 1e-6, "rear cover does not overlap the rear shell"
    )
    pushed = cover.copy()
    pushed.translate(V(0, -TOL, 0))
    check(
        pushed.common(rear).Volume > 1e-3,
        "rear cover rests on the rebate ledge (push-in stop)",
    )

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
    for name, item in {"OLED module": oled, "XIAO": xiao_box()}.items():
        hit = (
            sum(item.common(p).Volume for p in parts.values()) + item.common(bay).Volume
        )
        check(_fits(item, inner) and hit < 1e-6, f"{name} clears shells, hooks and bay")

    # Coupon: both halves valid, holding a whole hook, its pocket and the full rebate.
    c_front, c_rear = coupon_parts
    whole_hook = abs(c_rear.common(shell_hooks()[0]).Volume - shell_hooks()[0].Volume)
    pocket_cut = c_front.common(hook_pockets()[0]).Volume
    rebate_bb = c_rear.BoundBox
    check(
        all(p.isValid() and p.Volume > 0 for p in coupon_parts)
        and whole_hook < 1e-3
        and pocket_cut < 1e-6
        and _fits(hook_pockets()[0], c_front.BoundBox)  # the void lies inside it
        and _fits(cover, rebate_bb),
        "coupon holds a whole hook, its pocket and the cover rebate",
    )
    return failures


def export(front: Part.Shape, rear: Part.Shape, cover: Part.Shape) -> None:
    """Write the three STLs, cubbies.step and cubbies.FCStd to EXPORT_DIR (overwrite)."""
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
    App.closeDocument(doc.Name)


def export_print(
    front: Part.Shape,
    rear: Part.Shape,
    cover: Part.Shape,
    coupon_parts: tuple[Part.Shape, Part.Shape],
) -> None:
    """Write print-oriented STLs and two slicer-neutral 3MF plates to PRINT_DIR.

    Orientation (mechanical spec): front pieces front face down, rear pieces and the
    cover outer face down. The coupon plate carries a real cover for the rebate test.
    """
    os.makedirs(PRINT_DIR, exist_ok=True)
    c_front, c_rear = coupon_parts
    parts = {
        "cubbies-front": on_bed(front, -1),
        "cubbies-rear": on_bed(rear, 1),
        "cubbies-rear-cover": on_bed(cover, 1),
        "coupon-front": on_bed(c_front, -1),
        "coupon-rear": on_bed(c_rear, 1),
    }
    plates = {
        "cubbies-plate.3mf": ["cubbies-front", "cubbies-rear", "cubbies-rear-cover"],
        "coupon-plate.3mf": ["coupon-front", "coupon-rear", "cubbies-rear-cover"],
    }
    doc = App.newDocument("cubbies_print")
    for name, shape in parts.items():
        mesh = MeshPart.meshFromShape(
            Shape=shape, LinearDeflection=0.02, AngularDeflection=0.2
        )
        mesh.write(os.path.join(PRINT_DIR, name + ".stl"))
    for file_name, names in plates.items():
        objects = []
        for i, shape in enumerate(plate([parts[n] for n in names])):
            obj = doc.addObject("Mesh::Feature", f"{names[i].replace('-', '_')}_{i}")
            obj.Mesh = MeshPart.meshFromShape(
                Shape=shape, LinearDeflection=0.02, AngularDeflection=0.2
            )
            objects.append(obj)
        Mesh.export(objects, os.path.join(PRINT_DIR, file_name))
    App.closeDocument(doc.Name)


def main() -> None:
    """Build, check, then export; CUBBIES_CHECK_ONLY=1 stops after the checks.

    Exits 1 without writing anything if a check fails. Called at module level because
    freecadcmd runs a script under its file name, not as __main__.
    """
    cube = hollow_cube()
    front, rear, cover = front_shell(cube), rear_shell(cube), rear_cover(cube)
    coupon_parts = coupon(front, rear)
    failures = run_checks(front, rear, cover, coupon_parts)
    if failures:
        App.Console.PrintError(f"{len(failures)} check(s) failed; nothing exported.\n")
        sys.exit(1)
    if os.environ.get("CUBBIES_CHECK_ONLY") != "1":
        export(front, rear, cover)
        export_print(front, rear, cover, coupon_parts)
        App.Console.PrintMessage(f"Exported to {EXPORT_DIR}\n")


main()
