"""Hard-surface shapes: boxes, beams, cylinders, plates, bolts and grooves.

Dust3D builds every part by sweeping a cross-section (its cut face) along a chain of
nodes. With a custom polygon cut face, flat ends (``rounded: false``), no subdivision, no
extra rings along long edges (``interpolate: false``) and a low smoothing angle, that sweep
is a crisp prism: a box, an I-beam, a hex bolt, a tapered strut. Getting there by hand
means working out the chain's cross-section frame (which world direction the profile's u
and v end up on), normalising the profile the way Dust3D does, and re-centring it. These
shapes do that for you and expand into ordinary tube parts before anything else sees them,
so linting, seams, variants, rigging and decompiling work as for any other part.

Use them anywhere a part can go (``parts`` or a group), with a ``shape`` key:

    {"shape": "box", "center": [0, 1, 0], "size": [0.6, 0.4, 1.2], "bevel": 0.03}
    {"shape": "beam", "path": [[0, 0, 0], [0, 1, 0]], "profile": "I", "width": 0.2, "height": 0.3}
    {"shape": "cylinder", "from": [0, 0, 0], "to": [0, 0.2, 0], "radius": 0.1, "sides": 8}
    {"shape": "plate", "center": [0, 1, 0.3], "normal": [0, 0, 1], "size": [0.5, 0.4], "thickness": 0.02}
    {"shape": "bolts", "points": [[0.1, 1, 0.31], [-0.1, 1, 0.31]], "normal": [0, 0, 1], "radius": 0.015}
    {"shape": "groove", "from": [-0.2, 1, 0.3], "to": [0.2, 1, 0.3], "normal": [0, 0, 1], "width": 0.01}

Every shape also takes the ordinary part keys (name, color, bones, combine, metallic,
roughness, smooth, mirror, disabled, image). Their defaults differ from tubes: flat ends,
no subdivision, no interpolation and ``smooth: 30`` (edges sharper than 30 degrees stay
sharp). ``mirror: true`` emits an explicit mirrored copy (``<name>_mirror``), a part of its own
that variants can override or remove by name; its Left/Right bones are swapped.

Profiles (``profile`` on box, beam and prism; sizes in metres, ``b`` = bevel):
  rect      width x height                          (box and plate use this)
  I         flanges ``thickness`` thick, web ``web`` thick
  L, T, U   angle, tee and channel sections, walls ``thickness`` thick
  trapezoid ``width`` at the bottom, ``top`` at the top (a wedge seen end-on)
  ngon      ``sides``-sided, across-corners diameter = width (cylinder uses this)
  polygon   your own ``points`` [[side, up], ...] in metres
"""
import math
from typing import Any, Dict, List

SHAPES = ("box", "beam", "prism", "cylinder", "plate", "bolts", "groove")
COMMON = {"name", "color", "bones", "combine", "metallic", "roughness", "emissive", "slot", "smooth", "mirror",
          "disabled", "image", "interpolate", "subdivided", "rounded", "hard"}
HARD_DEFAULTS = {"rounded": False, "subdivided": False, "chamfered": False, "interpolate": False,
                 "smooth": 30.0, "hard": True}


class ShapeError(ValueError):
    pass


# ---------------------------------------------------------------- vectors
def _v(x, what="vector"):
    if not isinstance(x, (list, tuple)) or len(x) != 3:
        raise ShapeError("%s must be [x, y, z], got %r" % (what, x))
    return [float(c) for c in x]


def _add(a, b):
    return [a[i] + b[i] for i in range(3)]


def _sub(a, b):
    return [a[i] - b[i] for i in range(3)]


def _mul(a, s):
    return [c * s for c in a]


def _dot(a, b):
    return sum(a[i] * b[i] for i in range(3))


def _cross(a, b):
    return [a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]]


def _len(a):
    return math.sqrt(_dot(a, a))


def _unit(a):
    n = _len(a)
    if n < 1e-12:
        raise ShapeError("zero-length direction")
    return [c / n for c in a]


def _frame(axis, up_hint):
    """(side, up) perpendicular to axis, with up as close to up_hint as possible."""
    axis = _unit(axis)
    up = [float(c) for c in (up_hint or [0, 1, 0])]
    if abs(_dot(_unit(up), axis)) > 0.98:  # up along the axis: pick another
        up = [0, 0, 1] if abs(axis[2]) < 0.9 else [1, 0, 0]
    side = _unit(_cross(up, axis))
    up = _unit(_cross(axis, side))
    return side, up


# ---------------------------------------------------------------- profiles (metres, side/up)
def _bevel_rect(w, h, b):
    hw, hh = w / 2.0, h / 2.0
    b = max(0.0, min(b, hw * 0.45, hh * 0.45))
    if b <= 1e-9:
        return [[hw, hh], [-hw, hh], [-hw, -hh], [hw, -hh]]
    return [[hw - b, hh], [-hw + b, hh], [-hw, hh - b], [-hw, -hh + b],
            [-hw + b, -hh], [hw - b, -hh], [hw, -hh + b], [hw, hh - b]]


def profile_points(kind, w, h, t=None, web=None, top=None, sides=8, b=0.0, points=None):
    """A closed polygon [[side, up], ...] in metres, counter-clockwise."""
    t = t if t is not None else min(w, h) * 0.2
    web = web if web is not None else t
    if kind == "rect":
        return _bevel_rect(w, h, b)
    hw, hh = w / 2.0, h / 2.0
    # I and T flanges taper towards their tips, as rolled steel sections do; it also keeps
    # the flange's underside points off one line, which a flat cap needs to triangulate
    tip = t * 0.75
    if kind == "I":
        return [[hw, hh], [-hw, hh], [-hw, hh - tip], [-web / 2, hh - t], [-web / 2, -hh + t], [-hw, -hh + tip],
                [-hw, -hh], [hw, -hh], [hw, -hh + tip], [web / 2, -hh + t], [web / 2, hh - t], [hw, hh - tip]]
    if kind == "L":
        return [[-hw, hh], [-hw, -hh], [hw, -hh], [hw, -hh + t], [-hw + t, -hh + t], [-hw + t, hh]]
    if kind == "T":
        return [[hw, hh], [-hw, hh], [-hw, hh - tip], [-web / 2, hh - t], [-web / 2, -hh], [web / 2, -hh],
                [web / 2, hh - t], [hw, hh - tip]]
    if kind == "U":
        return [[-hw, hh], [-hw, -hh], [hw, -hh], [hw, hh], [hw - t, hh], [hw - t, -hh + t],
                [-hw + t, -hh + t], [-hw + t, hh]]
    if kind == "trapezoid":
        tp = (top if top is not None else w * 0.5) / 2.0
        return [[hw, -hh], [tp, hh], [-tp, hh], [-hw, -hh]]
    if kind == "ngon":
        n = max(3, int(sides))
        r = w / 2.0
        # a flat side at the top, so hexagon bolts and octagon posts sit square
        off = math.pi / 2 - math.pi / n
        return [[r * math.cos(off + 2 * math.pi * i / n), r * math.sin(off + 2 * math.pi * i / n) * h / w]
                for i in range(n)]
    if kind == "polygon":
        if not points or len(points) < 3:
            raise ShapeError("profile polygon needs >= 3 points")
        return [[float(p[0]), float(p[1])] for p in points]
    raise ShapeError("unknown profile %r (rect, I, L, T, U, trapezoid, ngon, polygon)" % (kind,))


# ---------------------------------------------------------------- the sweep
def _tube_axes(nodes):
    from .spec import tube_axes  # the same frame Dust3D computes for this chain
    return tube_axes(nodes)


def _sweep(path, radius_scale, profile, side, up, wh_scale=None):
    """Turn a path (list of [x,y,z]), a per-node profile scale and a profile in the
    (side, up) frame into Dust3D nodes + a normalised cut face, re-centred as Dust3D will."""
    # the profile's bounding box, in metres
    us = [p[0] for p in profile]
    vs = [p[1] for p in profile]
    cs, cu = (max(us) + min(us)) / 2.0, (max(vs) + min(vs)) / 2.0
    half = max(max(us) - min(us), max(vs) - min(vs)) / 2.0
    if half <= 1e-9:
        raise ShapeError("profile has no size")
    # Dust3D re-centres the profile on its bounding box, so move the chain instead
    shift = _add(_mul(side, cs), _mul(up, cu))
    pts = [_add(p, shift) for p in path]
    nodes = [[p[0], p[1], p[2], half * s] for p, s in zip(pts, radius_scale)]
    axes = _tube_axes(nodes)
    if axes is None:
        raise ShapeError("a shape needs at least two distinct points")
    tu, tv = axes
    if wh_scale:
        # per-node width/height scale: Dust3D scales along the tube's u (width) and v
        # (thickness), so hand each to whichever of u, v the profile's side lies along
        side_on_u = abs(_dot(side, tu)) >= abs(_dot(side, tv))
        for n, (ws, hs) in zip(nodes, wh_scale):
            n += [ws, hs] if side_on_u else [hs, ws]
    face = []
    for s_, u_ in profile:
        w = _add(_mul(side, s_ - cs), _mul(up, u_ - cu))  # world offset of this profile point
        face.append([round(_dot(w, tu) / half, 5), round(_dot(w, tv) / half, 5)])
    return nodes, face


def _mirror_vec(v):
    return [-v[0], v[1], v[2]]


def _swap_side(bones):
    def one(b):
        return b.replace("Left", "\0").replace("Right", "Left").replace("\0", "Right") if b else b
    if isinstance(bones, str):
        return one(bones)
    return [one(b) for b in bones] if bones else bones


def _part(raw, nodes, face, extra=None):
    out = dict(HARD_DEFAULTS)
    for k in COMMON:
        if k in raw and k != "mirror":
            out[k] = raw[k]
    out["nodes"] = [[round(c, 6) for c in n] for n in nodes]
    out["cutFace"] = face
    if extra:
        out.update(extra)
    return out


def _emit(raw, path, scale, profile, side, up, extra=None, wh_scale=None):
    """One part (and its explicit mirror copy when asked)."""
    nodes, face = _sweep(path, scale, profile, side, up, wh_scale)
    parts = [_part(raw, nodes, face, extra)]
    if raw.get("mirror"):
        mpath = [_mirror_vec(p) for p in path]
        # mirroring flips handedness: flip the side axis so the profile mirrors too
        mnodes, mface = _sweep(mpath, scale, [[-p[0], p[1]] for p in profile], _mirror_vec(side), _mirror_vec(up),
                               wh_scale)
        m = _part(raw, mnodes, mface, extra)
        if raw.get("name"):
            m["name"] = raw["name"] + "_mirror"
        if raw.get("bones"):
            m["bones"] = _swap_side(raw["bones"])
        parts.append(m)
    return parts


# ---------------------------------------------------------------- shapes
def _check_keys(raw, allowed):
    unknown = set(raw) - allowed - COMMON - {"shape"}
    if unknown:
        raise ShapeError("shape %r (%s): unknown keys %s" % (raw.get("name"), raw.get("shape"), sorted(unknown)))


def _box(raw):
    _check_keys(raw, {"center", "size", "axis", "up", "bevel", "taper", "profile", "top", "thickness", "web"})
    c = _v(raw.get("center", [0, 0, 0]), "center")
    size = _v(raw.get("size"), "size")
    if min(size) <= 0:
        raise ShapeError("box %r: sizes must be > 0" % raw.get("name"))
    axis = raw.get("axis")
    if axis is None:
        axis = "xyz"[max(range(3), key=lambda i: size[i])]
    if isinstance(axis, str):
        i = "xyz".index(axis)
        ax = [0.0, 0.0, 0.0]
        ax[i] = 1.0
        default_up = [0, 1, 0] if i != 1 else [0, 0, 1]
        side, up = _frame(ax, raw.get("up", default_up))
        length = size[i]
        # the section's width and height are the sizes along side and up
        w, h = _pick(size, side), _pick(size, up)
    else:
        ax = _unit(_v(axis, "axis"))
        side, up = _frame(ax, raw.get("up", [0, 1, 0]))
        w, h, length = size  # [width, height, length] for a free axis
    half = _mul(ax, length / 2.0)
    path = [_sub(c, half), _add(c, half)]
    prof = profile_points(raw.get("profile", "rect"), w, h, raw.get("thickness"), raw.get("web"), raw.get("top"),
                          b=float(raw.get("bevel", 0.0)))
    taper = raw.get("taper", 1.0)
    if isinstance(taper, (list, tuple)):
        # [width, height] at the far end: a wedge or a hull that narrows one way only
        if len(taper) != 2 or min(float(t) for t in taper) <= 0:
            raise ShapeError("box %r: taper is a number or [width, height] > 0" % raw.get("name"))
        return _emit(raw, path, [1.0, 1.0], prof, side, up,
                     wh_scale=[[1.0, 1.0], [float(taper[0]), float(taper[1])]])
    return _emit(raw, path, [1.0, float(taper)], prof, side, up)


def _pick(size, direction):
    return sum(abs(direction[i]) * size[i] for i in range(3))


def _beam(raw, kind=None):
    _check_keys(raw, {"path", "profile", "width", "height", "thickness", "web", "top", "up", "bevel",
                      "taper", "points", "sides"})
    path = [_v(p, "path point") for p in raw.get("path") or []]
    if len(path) < 2:
        raise ShapeError("beam %r needs a path of >= 2 points" % raw.get("name"))
    w = float(raw.get("width", 0.1))
    h = float(raw.get("height", w))
    side, up = _frame(_sub(path[1], path[0]), raw.get("up"))
    prof = profile_points(kind or raw.get("profile", "rect"), w, h, raw.get("thickness"), raw.get("web"),
                          raw.get("top"), sides=int(raw.get("sides", 8)), b=float(raw.get("bevel", 0.0)),
                          points=raw.get("points"))
    taper = raw.get("taper", 1.0)
    if isinstance(taper, (int, float)):
        n = len(path)
        scale = [1.0 + (float(taper) - 1.0) * i / (n - 1) for i in range(n)]
    else:
        scale = [float(s) for s in taper]
        if len(scale) != len(path):
            raise ShapeError("beam %r: taper list needs one value per path point" % raw.get("name"))
    return _emit(raw, path, scale, prof, side, up)


def _cylinder(raw):
    _check_keys(raw, {"from", "to", "radius", "radius_to", "sides", "up", "bevel"})
    a, b = _v(raw.get("from"), "from"), _v(raw.get("to"), "to")
    r = float(raw.get("radius", 0.05))
    r2 = float(raw.get("radius_to", r))
    side, up = _frame(_sub(b, a), raw.get("up"))
    prof = profile_points("ngon", 2 * r, 2 * r, sides=int(raw.get("sides", 8)))
    return _emit(raw, [a, b], [1.0, r2 / r], prof, side, up)


def _plate(raw):
    _check_keys(raw, {"center", "normal", "size", "thickness", "up", "bevel", "lift"})
    c = _v(raw.get("center"), "center")
    n = _unit(_v(raw.get("normal", [0, 1, 0]), "normal"))
    wh = raw.get("size")
    if not isinstance(wh, (list, tuple)) or len(wh) != 2:
        raise ShapeError("plate %r: size is [width, height] on the surface" % raw.get("name"))
    t = float(raw.get("thickness", 0.02))
    lift = float(raw.get("lift", t * 0.5))  # sit just proud of the surface: no coplanar faces
    c = _add(c, _mul(n, lift))
    # the plate's long direction is the sweep axis; the section is (other size) x thickness
    upv = raw.get("up") or ([0, 1, 0] if abs(n[1]) < 0.9 else [0, 0, 1])
    across, along = _frame(n, upv)          # two directions lying in the surface
    w, h = float(wh[0]), float(wh[1])
    axis, length, width = (along, h, w) if h >= w else (across, w, h)
    other = across if axis is along else along
    half = _mul(axis, length / 2.0)
    prof = _bevel_rect(width, t, float(raw.get("bevel", 0.0)))
    return _emit(raw, [_sub(c, half), _add(c, half)], [1.0, 1.0], prof, other, n,
                 None if "combine" in raw else {"combine": "Uncombined"})


def _bolts(raw):
    _check_keys(raw, {"points", "normal", "radius", "height", "sides"})
    n = _unit(_v(raw.get("normal", [0, 1, 0]), "normal"))
    r = float(raw.get("radius", 0.015))
    h = float(raw.get("height", r * 0.8))
    out = []
    base = raw.get("name") or "bolt"
    for i, p in enumerate(raw.get("points") or []):
        p = _v(p, "bolt point")
        one = dict(raw)
        one["name"] = "%s%d" % (base, i)
        one.setdefault("combine", "Uncombined")
        side, up = _frame(n, None)
        prof = profile_points("ngon", 2 * r, 2 * r, sides=int(raw.get("sides", 6)))
        a = _sub(p, _mul(n, h * 0.3))           # sunk a little into the surface
        out += _emit(one, [a, _add(p, _mul(n, h))], [1.0, 0.8], prof, side, up)
    return out


def _groove(raw):
    _check_keys(raw, {"from", "to", "normal", "width", "depth"})
    a, b = _v(raw.get("from"), "from"), _v(raw.get("to"), "to")
    n = _unit(_v(raw.get("normal", [0, 1, 0]), "normal"))
    w = float(raw.get("width", 0.01))
    d = float(raw.get("depth", w))
    side, up = _frame(_sub(b, a), n)
    # cut from `depth` below the surface to `depth` above it, so the cutter never lies flush
    prof = _bevel_rect(w, 2 * d, 0.0)
    one = dict(raw)
    one.setdefault("combine", "Inversion")
    return _emit(one, [a, b], [1.0, 1.0], prof, side, up)


def expand_shape(raw: Dict[str, Any]) -> List[Dict[str, Any]]:
    kind = raw.get("shape")
    if kind == "box":
        return _box(raw)
    if kind == "beam":
        return _beam(raw)
    if kind == "prism":
        return _beam(raw, "polygon")
    if kind == "cylinder":
        return _cylinder(raw)
    if kind == "plate":
        return _plate(raw)
    if kind == "bolts":
        return _bolts(raw)
    if kind == "groove":
        return _groove(raw)
    raise ShapeError("unknown shape %r (%s)" % (kind, ", ".join(SHAPES)))


def _carves(elements):
    """Does this list hold a cutter (a groove, or anything combined as an Inversion)?"""
    return any(isinstance(e, dict) and (e.get("shape") == "groove" or e.get("combine") == "Inversion")
               for e in elements or [])


def expand_shapes(elements: List[Any], _solid_default: str = "Uncombined") -> List[Any]:
    """Expand every {"shape": ...} entry (also inside groups) into plain tube parts.

    Hard-surface pieces are kitbashed by default: each is its own closed shell
    (``combine: Uncombined``), overlapping its neighbours the way game props are built,
    with no boolean to fail or to leave slivers. Where a list also holds a cutter (a groove
    or an Inversion part), its shapes are unioned instead (a crisp boolean, ``hard``), so the
    cutter has a surface to carve. An explicit ``combine`` always wins.
    """
    default = "Normal" if _carves(elements) else _solid_default
    out = []
    for e in elements or []:
        if isinstance(e, dict) and "shape" in e:
            e2 = dict(e)
            if "combine" not in e2 and e2["shape"] != "groove":
                e2["combine"] = default
            out.extend(expand_shape(e2))
        elif isinstance(e, dict) and "group" in e:
            g = dict(e)
            g["group"] = expand_shapes(e["group"])
            out.append(g)
        else:
            out.append(e)
    return out
