#!/usr/bin/env python3
"""Build a small demo of every new Dust3D / agent-toolkit feature out of simple geometry.

    DUST3D_BIN=/path/to/dust3d python3 examples/features/make_features.py [out_dir]

For each feature it writes the demo specs (``specs/*.json``, open the built ``.ds3`` files
in Dust3D to inspect them) and one labelled sheet (``NN_feature.png``): shaded views, and
wireframes where the point is the topology.
"""
import copy
import json
import os
import subprocess
import sys
import tempfile

import numpy as np
from PIL import Image, ImageDraw, ImageFont

HERE = os.path.dirname(os.path.abspath(__file__))
TOOLKIT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, TOOLKIT)

from dust3d_agent import glb as glbmod  # noqa: E402
from dust3d_agent import render as R  # noqa: E402
from dust3d_agent import wire as W  # noqa: E402

OUT = sys.argv[1] if len(sys.argv) > 1 else os.path.join(HERE, "out")
SPECS = os.path.join(OUT, "specs")
BUILD = os.path.join(OUT, "build")
TILE = 300

STEEL, DARK, AMBER, CYAN, RUST, SAND = "#6f7680", "#3c4046", "#d08a2a", "#6ff3ff", "#a0603e", "#c8a878"
HARD = {"rounded": False, "subdivided": False, "interpolate": False, "smooth": 30}


def font(size):
    for f in ("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", "/Library/Fonts/Arial.ttf"):
        if os.path.exists(f):
            return ImageFont.truetype(f, size)
    return ImageFont.load_default()


def build(name, spec):
    spec = dict(spec, name=name)
    os.makedirs(SPECS, exist_ok=True)
    path = os.path.join(SPECS, name + ".json")
    json.dump(spec, open(path, "w"), indent=1)
    out = os.path.join(BUILD, name)
    r = subprocess.run([sys.executable, "-m", "dust3d_agent", "build", path, "-o", out, "--no-render"],
                       cwd=TOOLKIT, capture_output=True, text=True)
    rep = os.path.join(out, name + "_report.json")
    if not os.path.exists(rep):
        raise SystemExit("build of %s failed:\n%s" % (name, (r.stdout + r.stderr)[-2000:]))
    return out, json.load(open(rep))


def shaded(out, name, view="three_quarter", size=TILE):
    g = glbmod.load(os.path.join(out, name + ".glb"))
    return R.render_view(g, view, size, draw_ground=False)


def wire(out, name, view="three_quarter", size=TILE):
    v, f = W.load_obj(os.path.join(out, name + "_topology.obj"))
    return W.render_wire(v, f, view, size)


def caption(im, title, sub=""):
    """A tile with a title bar and an optional second line underneath."""
    h = 44 if sub else 26
    t = Image.new("RGB", (im.width, im.height + h), (255, 255, 255))
    t.paste(im, (0, h))
    d = ImageDraw.Draw(t)
    d.rectangle([0, 0, im.width, h], fill=(34, 36, 40))
    d.text((8, 4), title, fill=(255, 255, 255), font=font(15))
    if sub:
        d.text((8, 24), sub, fill=(190, 196, 204), font=font(12))
    return t


def _lines_needed(text, width):
    d = ImageDraw.Draw(Image.new("RGB", (1, 1)))
    f14, n, cur = font(14), 1, ""
    for w in text.split():
        trial = (cur + " " + w).strip()
        if d.textlength(trial, font=f14) > width and cur:
            n, cur = n + 1, w
        else:
            cur = trial
    return n


def sheet(number, title, blurb, tiles, cols=None):
    cols = cols or len(tiles)
    rows = (len(tiles) + cols - 1) // cols
    tw, th = tiles[0].width, max(t.height for t in tiles)
    head = 64 + 18 * max(0, _lines_needed(blurb, cols * tw - 24) - 1)
    s = Image.new("RGB", (cols * tw, head + rows * th), (246, 244, 240))
    d = ImageDraw.Draw(s)
    d.text((12, 8), "%d. %s" % (number, title), fill=(20, 20, 20), font=font(24))
    f14 = font(14)
    words, lines, cur = blurb.split(), [], ""
    for w in words:
        trial = (cur + " " + w).strip()
        if d.textlength(trial, font=f14) > cols * tw - 24 and cur:
            lines.append(cur)
            cur = w
        else:
            cur = trial
    lines.append(cur)
    for i, ln in enumerate(lines):
        d.text((12, 40 + 18 * i), ln, fill=(70, 70, 70), font=f14)
    for i, t in enumerate(tiles):
        s.paste(t, ((i % cols) * tw, head + (i // cols) * th))
    path = os.path.join(OUT, "%02d_%s.png" % (number, title.lower().replace(" ", "_").replace(",", "")
                                                 .replace("/", "_").replace("(", "").replace(")", "")))
    s.save(path)
    print("wrote", path, flush=True)
    return path


def one(name, part, **extra):
    return build(name, dict({"parts": part if isinstance(part, list) else [part]}, **extra))


# ---------------------------------------------------------------------------- demos
def demo_boxes(n):
    items = [
        ("box_plain", "box", "size [0.6, 0.4, 0.4]", {}),
        ("box_bevel", "bevel 0.06", "8-sided profile, sharp edges", {"bevel": 0.06}),
        ("box_taper", "taper 0.4", "uniform: a frustum", {"taper": 0.4}),
        ("box_wedge", "taper [0.2, 1]", "one way only: a wedge", {"taper": [0.2, 1.0]}),
        ("box_trapezoid", "profile trapezoid", "top 0.15 wide", {"profile": "trapezoid", "top": 0.15}),
    ]
    tiles = []
    for name, t, sub, kw in items:
        out, rep = one(name, dict({"shape": "box", "name": "b", "center": [0, 0.2, 0], "size": [0.6, 0.4, 0.4],
                                   "axis": "x", "color": STEEL, "metallic": 0.5}, **kw))
        tiles.append(caption(shaded(out, name), t, sub))
    return sheet(n, "Box shapes", "Boxes, bevels, frustums and wedges from one line of JSON each; "
                 "sizes are world units, the cut-face frame is worked out for you.", tiles)


def demo_profiles(n):
    # laid along z with the near end towards the camera, so each profile reads on its end cap
    base = {"shape": "beam", "name": "b", "path": [[0, 0, -0.45], [0, 0, 0]], "width": 0.24, "height": 0.2,
            "thickness": 0.045, "color": STEEL, "metallic": 0.5}
    items = [("rect", {"profile": "rect", "bevel": 0.02}), ("I", {"profile": "I"}), ("L", {"profile": "L"}),
             ("T", {"profile": "T"}), ("U", {"profile": "U"}), ("trapezoid", {"profile": "trapezoid", "top": 0.1}),
             ("ngon (6)", {"profile": "ngon", "sides": 6}),
             ("polygon", {"profile": "polygon", "points": [[0, 0.12], [0.04, 0.03], [0.12, 0.02], [0.05, -0.03],
                                                           [0.08, -0.12], [0, -0.06], [-0.08, -0.12], [-0.05, -0.03],
                                                           [-0.12, 0.02], [-0.04, 0.03]]}),
             ("bent path", {"profile": "I", "path": [[0, 0, -0.45], [0, 0, 0], [0.3, 0.25, 0.1]]}),
             ("taper list", {"profile": "rect", "path": [[0, 0, -0.45], [0, 0.1, -0.2], [0, 0, 0]],
                             "taper": [1, 0.6, 0.25]})]
    tiles = []
    for label, kw in items:
        name = "profile_" + label.split()[0].lower()
        if label == "bent path":
            name = "profile_bent"
        if label == "taper list":
            name = "profile_taper"
        out, _ = one(name, dict(base, **kw))
        view = "three_quarter"
        tiles.append(caption(shaded(out, name, view, 240), label))
    return sheet(n, "Beam profiles", "A beam sweeps a profile along a path, mitred at bends: "
                 "rect, I, L, T, U, trapezoid, n-gon or your own polygon.", tiles, cols=5)


def demo_cyl_plate_bolts(n):
    tiles = []
    out, _ = one("cyl_octagon", {"shape": "cylinder", "name": "c", "from": [0, 0, 0], "to": [0, 0.5, 0],
                                 "radius": 0.15, "sides": 8, "color": STEEL})
    tiles.append(caption(shaded(out, "cyl_octagon"), "cylinder", "sides 8"))
    out, _ = one("cyl_cone", {"shape": "cylinder", "name": "c", "from": [0, 0, 0], "to": [0, 0.5, 0],
                              "radius": 0.18, "radius_to": 0.02, "sides": 12, "color": AMBER})
    tiles.append(caption(shaded(out, "cyl_cone"), "radius_to", "a cone, 12 sides"))
    panel = [
        {"shape": "box", "name": "body", "center": [0, 0.25, 0], "size": [0.7, 0.5, 0.3], "axis": "x",
         "bevel": 0.03, "color": DARK, "metallic": 0.5},
        {"shape": "plate", "name": "plate", "center": [0, 0.27, 0.15], "normal": [0, 0, 1], "size": [0.4, 0.3],
         "thickness": 0.025, "bevel": 0.006, "color": STEEL, "metallic": 0.5},
        {"shape": "bolts", "name": "bolt", "points": [[-0.16, 0.39, 0.175], [0.16, 0.39, 0.175],
                                                      [-0.16, 0.15, 0.175], [0.16, 0.15, 0.175]],
         "normal": [0, 0, 1], "radius": 0.018, "color": AMBER},
        {"shape": "cylinder", "name": "lens", "from": [0.25, 0.4, 0.14], "to": [0.25, 0.4, 0.2], "radius": 0.04,
         "sides": 8, "color": CYAN},
    ]
    out, _ = one("plate_bolts", panel)
    tiles.append(caption(shaded(out, "plate_bolts"), "plate + bolts", "lifted clear of the surface"))
    return sheet(n, "Cylinder, plate, bolts", "n-sided posts, cones and lenses; plates that sit on a surface "
                 "and hex bolt heads sunk into it.", tiles)


def demo_carving(n):
    body = {"shape": "box", "name": "body", "center": [0, 0.25, 0], "size": [0.7, 0.5, 0.5], "axis": "x",
            "bevel": 0.03, "color": STEEL, "metallic": 0.5}
    grooves = [{"shape": "groove", "name": "g%d" % i, "from": [x, 0.05, 0.25], "to": [x, 0.45, 0.25],
                "normal": [0, 0, 1], "width": 0.025, "depth": 0.03} for i, x in enumerate((-0.15, 0.0, 0.15))]
    grooves.append({"shape": "groove", "name": "gtop", "from": [-0.3, 0.5, 0.0], "to": [0.3, 0.5, 0.0],
                    "normal": [0, 1, 0], "width": 0.025, "depth": 0.03})
    vent = {"shape": "box", "name": "vent", "center": [0.0, 0.5, -0.12], "size": [0.4, 0.12, 0.14], "axis": "x",
            "combine": "Inversion"}
    tiles = []
    out, _ = one("carve_grooves", [{"name": "hull", "group": [body] + grooves}])
    tiles.append(caption(shaded(out, "carve_grooves"), "groove", "cut-in panel lines"))
    tiles.append(caption(wire(out, "carve_grooves"), "wireframe", "the grooves are real cuts"))
    out, _ = one("carve_vent", [{"name": "hull", "group": [body, vent]}])
    tiles.append(caption(shaded(out, "carve_vent"), "Inversion box", "a recessed vent"))
    kit = [dict(body, name="body"),
           {"shape": "cylinder", "name": "pipe", "from": [-0.2, 0.3, -0.35], "to": [-0.2, 0.3, 0.35],
            "radius": 0.07, "sides": 8, "color": DARK},
           {"shape": "box", "name": "cap", "center": [0.2, 0.55, 0], "size": [0.2, 0.12, 0.3], "axis": "z",
            "bevel": 0.02, "color": AMBER}]
    out, _ = one("kitbash", kit)
    tiles.append(caption(shaded(out, "kitbash"), "kitbash (default)", "overlapping closed shells, no boolean"))
    return sheet(n, "Carving and kitbashing", "Shapes overlap as separate shells by default; a group that "
                 "holds a groove or Inversion part unions its shapes so the cutter can carve.", tiles)


def demo_hard_join(n):
    def parts(hard):
        return [
            {"name": "block", "nodes": [[-0.3, 0.2, 0, 0.2], [0.3, 0.2, 0, 0.2]], "cutFace": "Quad",
             "color": STEEL, "hard": hard, **HARD},
            {"name": "post", "nodes": [[0, 0.3, 0, 0.09], [0, 0.75, 0, 0.09]], "cutFace": "Hexagon",
             "color": AMBER, "hard": hard, **HARD},
        ]
    tiles = []
    for hard in (False, True):
        name = "join_hard" if hard else "join_soft"
        out, rep = one(name, parts(hard))
        tri = rep["metrics"]["triangles"]
        t = "hard: true" if hard else "hard: false (before)"
        s = "plain boolean, crisp edge" if hard else "smooth seam bridge"
        tiles.append(caption(shaded(out, name), t, s))
        tiles.append(caption(wire(out, name), "wireframe", "%d triangles" % tri))
    return sheet(n, "Hard joins", "A hard part joins with a plain boolean instead of Dust3D's smooth seam "
                 "bridge. Also an editor checkbox (Cut Face panel: Hard edges).", tiles, cols=4)


def demo_node_scale(n):
    tiles = []
    out, _ = one("node_uniform", {"name": "t", "nodes": [[0, 0, 0, 0.2], [0, 0.8, 0, 0.06]], "cutFace": "Quad",
                                  "color": STEEL, **HARD})
    tiles.append(caption(shaded(out, "node_uniform"), "radius only (before)", "[x, y, z, r]: both ways at once"))
    out, _ = one("node_blade", {"name": "t", "nodes": [[0, 0, 0, 0.2, 1, 1], [0, 0.8, 0, 0.2, 0.1, 1]],
                                "cutFace": "Quad", "color": STEEL, **HARD})
    tiles.append(caption(shaded(out, "node_blade"), "[x, y, z, r, w, t]", "narrows across only: a blade"))
    out, _ = one("node_hull", {"name": "t", "nodes": [[0, 0.3, -0.5, 0.2, 1.2, 0.5], [0, 0.3, 0, 0.2, 1, 1],
                                                      [0, 0.3, 0.5, 0.2, 0.3, 1.4]],
                               "cutFace": [[1, 1], [-1, 1], [-1, -1], [1, -1]], "color": DARK, **HARD})
    tiles.append(caption(shaded(out, "node_hull"), "three nodes", "wide and flat, then tall and narrow"))
    return sheet(n, "Per-node width and thickness", "Two more numbers on a node scale its cross-section "
                 "across and through independently, for wedges, blades and hulls.", tiles)


def demo_interpolate(n):
    tiles = []
    for interp in (True, False):
        name = "interp_on" if interp else "interp_off"
        out, rep = one(name, {"name": "t", "nodes": [[0, 0, 0, 0.08], [0, 0.9, 0, 0.06]], "cutFace": "Hexagon",
                              "rounded": False, "subdivided": False, "interpolate": interp, "color": SAND})
        tri = rep["metrics"]["triangles"]
        t = "interpolate: true" if interp else "interpolate: false"
        s = "extra rings + end rings" if interp else "rings only at its own nodes"
        tiles.append(caption(wire(out, name, "front"), t, "%s, %d triangles" % (s, tri)))
    return sheet(n, "Interpolation switch", "Rigid, low-poly parts skip the extra rings Dust3D adds along "
                 "long edges and near the ends. Editor checkbox: Extra rings.", tiles)


def demo_mirror(n):
    tiles = []
    out, _ = one("mirror_shape", [
        {"shape": "box", "name": "base", "center": [0, 0.05, 0], "size": [0.9, 0.1, 0.3], "axis": "x",
         "color": DARK},
        {"shape": "beam", "name": "arm", "path": [[0.15, 0.1, 0], [0.4, 0.6, 0]], "profile": "L", "width": 0.14,
         "height": 0.14, "thickness": 0.035, "mirror": True, "color": AMBER}])
    tiles.append(caption(shaded(out, "mirror_shape", "front"), "mirror: true (front)", "L sections mirror correctly"))
    tiles.append(caption(shaded(out, "mirror_shape", "top"), "top view", "explicit arm_mirror copy"))
    return sheet(n, "Mirrored shapes", "A mirrored shape is written as an explicit copy with its profile "
                 "and Left/Right bones mirrored, so asymmetric sections come out right.", tiles)


def demo_caps(n):
    tiles = []
    for prof in ("I", "T", "U"):
        name = "cap_" + prof
        out, rep = one(name, {"shape": "beam", "name": "b", "path": [[0, 0, 0], [0.3, 0.1, 0.4]], "profile": prof,
                              "width": 0.2, "height": 0.18, "thickness": 0.04, "color": STEEL})
        m = rep["metrics"]
        tiles.append(caption(wire(out, name), "%s-beam end cap" % prof,
                             "open edges %d, non-manifold %d" % (m["open_edges"], m["nonmanifold_edges"])))
    return sheet(n, "Concave end caps", "Flat caps on concave sections triangulate in the face's own plane "
                 "now, so I, T and U beams are watertight.", tiles)


RIG_SPIDER = None


def spider_parts():
    p = [{"shape": "box", "name": "thorax", "center": [0, 0.24, 0.25], "size": [0.26, 0.12, 0.26], "axis": "z",
          "bevel": 0.02, "color": DARK, "bones": "Cephalothorax"},
         {"shape": "box", "name": "head", "center": [0, 0.26, 0.43], "size": [0.14, 0.1, 0.12], "axis": "z",
          "taper": [0.7, 0.8], "color": DARK, "bones": "Head"},
         {"shape": "cylinder", "name": "eye", "from": [0, 0.28, 0.48], "to": [0, 0.28, 0.51], "radius": 0.03,
          "sides": 6, "color": CYAN, "bones": "Head"},
         {"shape": "box", "name": "abdomen", "center": [0, 0.22, -0.12], "size": [0.3, 0.2, 0.34], "axis": "z",
          "taper": [0.6, 0.7], "bevel": 0.03, "color": RUST, "bones": "Abdomen"}]
    legs = {"FrontLeft": [[0.12, 0.22, 0.3], [0.25, 0.29, 0.35], [0.4, 0.14, 0.4], [0.5, 0.0, 0.45]],
            "MidFrontLeft": [[0.12, 0.22, 0.22], [0.28, 0.29, 0.22], [0.45, 0.12, 0.22], [0.55, 0.0, 0.22]],
            "MidBackLeft": [[0.12, 0.21, 0.12], [0.28, 0.28, 0.1], [0.45, 0.12, 0.08], [0.55, 0.0, 0.05]],
            "BackLeft": [[0.1, 0.2, 0.0], [0.25, 0.28, -0.1], [0.4, 0.12, -0.15], [0.5, 0.0, -0.2]]}
    for side, path in legs.items():
        p.append({"shape": "beam", "name": side.lower() + "_leg", "path": path, "profile": "rect",
                  "width": 0.04, "height": 0.05, "taper": [1, 1, 0.8, 0.4], "mirror": True, "color": STEEL,
                  "bones": [side + "Coxa", side + "Femur", side + "Tibia"]})
    p.append({"shape": "beam", "name": "palp", "path": [[0.06, 0.25, 0.45], [0.15, 0.2, 0.55]], "profile": "rect",
              "width": 0.03, "height": 0.03, "mirror": True, "color": AMBER, "bones": "LeftPedipalp"})
    return p


def snake_parts():
    # one tube along the rig's spine (tail tip to neck), the head and jaw as their own parts
    zs = [-0.75, -0.6, -0.45, -0.3, -0.15, 0.0, 0.15, 0.3, 0.45, 0.6, 0.75, 0.88]
    rs = [0.01, 0.025, 0.035, 0.045, 0.05, 0.055, 0.056, 0.055, 0.052, 0.05, 0.046, 0.042]
    spine = [[0, 0.02 + r, z, r] for z, r in zip(zs, rs)]
    bones = ["TailTip", "Tail4", "Tail3", "Tail2", "Tail1", "Spine1", "Spine2", "Spine3", "Spine4", "Spine5",
             "Spine6"]
    return [{"name": "body", "nodes": spine, "bones": bones, "color": "#8a6a3a", "smooth": 60},
            {"name": "head", "nodes": [[0, 0.066, 0.9, 0.05], [0, 0.07, 0.98, 0.055], [0, 0.066, 1.06, 0.03]],
             "bones": "Head", "color": "#8a6a3a"},
            {"name": "jaw", "nodes": [[0, 0.04, 0.93, 0.035], [0, 0.035, 1.0, 0.03], [0, 0.035, 1.07, 0.015]],
             "bones": "Jaw", "color": "#6a4a2a"},
            {"name": "eye", "nodes": [[0.04, 0.095, 1.0, 0.012]], "mirror": True, "combine": "Uncombined",
             "color": "#1a1a1a"}]


def hopper_parts():
    # a block figure on the Biped rig, with a tail for balance
    return [
        {"shape": "box", "name": "hips", "center": [0, 0.95, 0], "size": [0.3, 0.14, 0.2], "axis": "x",
         "bevel": 0.02, "color": DARK, "bones": "Hips"},
        {"shape": "box", "name": "chest", "center": [0, 1.25, 0.02], "size": [0.34, 0.34, 0.24], "axis": "y",
         "taper": [1.0, 0.8], "bevel": 0.03, "color": RUST, "bones": "Chest"},
        {"shape": "box", "name": "head", "center": [0, 1.62, 0.05], "size": [0.2, 0.2, 0.24], "axis": "z",
         "bevel": 0.03, "color": RUST, "bones": "Head"},
        {"shape": "box", "name": "ear", "center": [0.06, 1.8, 0.02], "size": [0.05, 0.18, 0.03], "axis": "y",
         "taper": [0.4, 1.0], "mirror": True, "color": RUST, "bones": "Head"},
        {"shape": "beam", "name": "leg", "path": [[0.1, 0.92, 0], [0.12, 0.5, 0.12], [0.1, 0.1, -0.05],
                                                  [0.1, 0.02, 0.2]], "profile": "rect", "width": 0.09,
         "height": 0.09, "taper": [1.3, 1.0, 0.7, 0.6], "mirror": True, "color": RUST,
         "bones": ["LeftUpperLeg", "LeftLowerLeg", "LeftFoot"]},
        {"shape": "beam", "name": "arm", "path": [[0.18, 1.38, 0.02], [0.3, 1.2, 0.12], [0.32, 1.05, 0.2]],
         "profile": "rect", "width": 0.05, "height": 0.05, "mirror": True, "color": RUST,
         "bones": ["LeftUpperArm", "LeftLowerArm"]},
        {"shape": "beam", "name": "tail", "path": [[0, 0.95, -0.12], [0, 0.85, -0.35], [0, 0.6, -0.55],
                                                   [0, 0.35, -0.7]], "profile": "rect", "width": 0.1,
         "height": 0.1, "taper": [1, 0.8, 0.6, 0.35], "color": RUST,
         "bones": ["TailBase", "TailMid", "TailTip"]},
    ]


def demo_animation(n, title, blurb, name, rig, parts, anim_type, params=None):
    anim = {"type": anim_type, "name": anim_type.lower()}
    if params:
        anim["params"] = params
    out, rep = build(name, {"rig": rig, "parts": parts, "animations": [anim]})
    g = glbmod.load(os.path.join(out, name + ".glb"))
    a = [x for x in g.animations if x["name"] == anim["name"]][0]
    view = "left" if rig == "Snake" else "three_quarter"
    fb = R.bounds(g, [None, a], samples=6)
    ts = np.linspace(0, a["duration"], 6, endpoint=False)
    tiles = [caption(R.render_view(g, view, 240, anim=a, t=float(t), frame_bounds=fb), "t = %.2f s" % t) for t in ts]
    R.animation_gif(g, a, os.path.join(OUT, name + ".gif"), frames=20, size=320, view=view)
    return sheet(n, title, blurb, tiles)


def demo_variants(n):
    base = {"parts": [
        {"shape": "box", "name": "body", "center": [0, 0.3, 0], "size": [0.5, 0.3, 0.3], "axis": "x",
         "bevel": 0.03, "color": STEEL},
        {"shape": "cylinder", "name": "wheel", "from": [0.15, 0.12, 0.16], "to": [0.15, 0.12, 0.22],
         "radius": 0.12, "sides": 10, "mirror": False, "color": DARK},
        {"shape": "cylinder", "name": "wheel_b", "from": [-0.15, 0.12, 0.16], "to": [-0.15, 0.12, 0.22],
         "radius": 0.12, "sides": 10, "color": DARK}]}
    tiles = []
    out, _ = build("variant_base", base)
    tiles.append(caption(shaded(out, "variant_base"), "base", "variant_base.json"))
    os.makedirs(SPECS, exist_ok=True)
    var = {"extends": "variant_base.json", "recolor": {STEEL: AMBER}, "scale": 1.3,
           "add": [{"shape": "cylinder", "name": "beacon", "from": [0, 0.45, 0], "to": [0, 0.55, 0],
                    "radius": 0.05, "sides": 6, "color": CYAN}]}
    out, _ = build("variant_scout", var)
    tiles.append(caption(shaded(out, "variant_scout"), "extends base", "recolor, scale 1.3, add a beacon"))
    return sheet(n, "Variants", "A variant extends another spec: recolour, remove, override, add and scale "
                 "(shapes included), so families share one design.", tiles)


def main():
    os.makedirs(OUT, exist_ok=True)
    made = [demo_boxes(1), demo_profiles(2), demo_cyl_plate_bolts(3), demo_carving(4), demo_hard_join(5),
            demo_node_scale(6), demo_interpolate(7), demo_caps(8)]
    made.append(demo_animation(9, "SpiderAttack", "Spider rig: rear up, lift the front legs, curl the "
                               "abdomen, lunge and slam (legs are hard girder beams).", "anim_spider_attack",
                               "Spider", spider_parts(), "SpiderAttack"))
    made.append(demo_animation(10, "SnakeStrike", "Snake rig: raise the front third, coil back into an S, "
                               "lunge and snap the jaw.", "anim_snake_strike", "Snake", snake_parts(),
                               "SnakeStrike"))
    made.append(demo_animation(11, "BipedHop", "Biped rig: a looping two-legged hop with a tail for "
                               "balance (kangaroos, wallabies, hopping birds).", "anim_biped_hop", "Biped",
                               hopper_parts(), "BipedHop"))
    made.append(demo_variants(12))
    print("\n".join(made))


if __name__ == "__main__":
    main()
