"""Garment shells that follow a body part: {"shell": "torso", ...}.

A game character's clothes and armour have to move exactly with the body under them, or
the body pokes through when it bends. In Dust3D a part's surface is swept along its node
chain and skinned by the bones on its edges, so a garment built on *the same chain* with
slightly larger radii stays outside the body in every pose: it bends at the same nodes, by
the same bones, with the same cut face.

    {"shell": "torso", "name": "vest", "offset": 0.015, "range": [0.1, 0.85],
     "color": "#6a4a30", "slot": "armor/2"}

  shell     the body part to follow (a tube part, by name; inside groups is fine).
            "wrap" is its old name, still read on a part (on a group, "wrap" is the
            wrap modifier)
  offset    added to every radius (default 0.012); the garment's thickness over the body
  scale     multiplies every radius first (default 1.0)
  range     [from, to]: the stretch of the chain to cover, as fractions of its length
            (default [0, 1]; e.g. [0, 0.5] = the upper half of an arm: a short sleeve)
  flare     extra radius added at the `to` end, growing along the garment (a skirt, a cuff)
  flareStart extra radius at the `from` end (a collar)

The garment copies the body part's nodes (cut to the range), bones, mirror, cut face,
cut rotation, deform and flatten. Anything else is an ordinary part key (color, slot,
metallic, cutFace to override, ...). Defaults: ``combine: Uncombined`` (a separate shell over
the body, never unioned into it), flat ends (``rounded: false``) and the body part's
subdivision.
"""

from __future__ import annotations

import math
from typing import Any, Dict, List

from .shapes import ShapeError

SHELL_KEYS = {"shell", "offset", "scale", "range", "flare", "flareStart"}
COPIED = ("mirror", "cutFace", "cutRotation", "deformThickness", "deformWidth", "flatten",
          "deformUnified", "subdivided", "chamfered", "smooth")


def _find(elements, name):
    for e in elements or []:
        if not isinstance(e, dict):
            continue
        if e.get("name") == name and "nodes" in e:
            return e
        for key in ("group",):
            if isinstance(e.get(key), list):
                hit = _find(e[key], name)
                if hit is not None:
                    return hit
    return None


def _lerp(a, b, t):
    return [x + (y - x) * t for x, y in zip(a, b)]


def shell_part(raw: Dict[str, Any], source: Dict[str, Any], defaults: Dict[str, Any]) -> Dict[str, Any]:
    name = raw.get("name") or "shell_" + str(raw["shell"])
    nodes = source["nodes"]
    if len(nodes) < 2:
        raise ShapeError("shell %r: body part %r has a single node; a shell needs a chain" % (name, raw["shell"]))
    if source.get("loop"):
        raise ShapeError("shell %r: body part %r is a closed loop; a shell needs an open chain" % (name, raw["shell"]))
    offset = float(raw.get("offset", 0.012))
    scale = float(raw.get("scale", 1.0))
    lo, hi = (float(v) for v in raw.get("range", [0.0, 1.0]))
    if not 0.0 <= lo < hi <= 1.0:
        raise ShapeError("shell %r: range must be [from, to] with 0 <= from < to <= 1" % name)
    flare, flare0 = float(raw.get("flare", 0.0)), float(raw.get("flareStart", 0.0))

    # the chain as points along its length, 6-number nodes kept (per-node width/thickness)
    pts = [list(map(float, n)) + ([1.0, 1.0] if len(n) == 4 else []) for n in nodes]
    seg = [math.dist(a[:3], b[:3]) for a, b in zip(pts, pts[1:])]
    total = sum(seg) or 1.0
    cum = [0.0]
    for s in seg:
        cum.append(cum[-1] + s / total)

    def at(f):
        for i in range(len(seg)):
            if cum[i + 1] >= f - 1e-9:
                t = 0.0 if cum[i + 1] - cum[i] < 1e-12 else (f - cum[i]) / (cum[i + 1] - cum[i])
                return _lerp(pts[i], pts[i + 1], min(max(t, 0.0), 1.0)), i
        return pts[-1], len(seg) - 1

    start, _ = at(lo)
    end, _ = at(hi)
    chain = [(lo, start)] + [(cum[i], pts[i]) for i in range(1, len(pts) - 1) if lo + 1e-4 < cum[i] < hi - 1e-4] + [(hi, end)]

    src_bones = source.get("bones") or []
    if isinstance(src_bones, str):
        src_bones = [src_bones] * len(seg)
    bones = []
    for (fa, _), (fb, _) in zip(chain, chain[1:]):
        _, i = at((fa + fb) / 2)
        bones.append(src_bones[i] if i < len(src_bones) else "")

    out_nodes = []
    for f, p in chain:
        u = (f - lo) / (hi - lo)
        r = p[3] * scale + offset + flare0 * (1 - u) + flare * u
        if abs(p[4] - 1.0) > 1e-9 or abs(p[5] - 1.0) > 1e-9:
            # a deformed ring grows by the same offset on both axes
            w_ = (p[3] * scale * p[4] + offset) / r
            t_ = (p[3] * scale * p[5] + offset) / r
            out_nodes.append([round(v, 6) for v in (p[0], p[1], p[2], r, w_, t_)])
        else:
            out_nodes.append([round(v, 6) for v in (p[0], p[1], p[2], r)])

    part: Dict[str, Any] = {"name": name, "nodes": out_nodes, "combine": "Uncombined", "rounded": False}
    if any(bones):
        part["bones"] = bones
    for k in COPIED:
        if k in source:
            part[k] = source[k]
    for k, v in raw.items():
        if k not in SHELL_KEYS:
            part[k] = v
    return part


def _as_shell(e):
    """The entry as a shell (its old "wrap" key renamed), or None if it is not one. A group's
    "wrap" is the wrap modifier, never a shell."""
    if not isinstance(e, dict) or "group" in e:
        return None
    if "shell" in e:
        if "wrap" in e:
            raise ShapeError("shell %r: has both \"shell\" and \"wrap\" (its old name); keep only \"shell\""
                             % e.get("name"))
        return e
    if isinstance(e.get("wrap"), str):
        e = dict(e)
        e["shell"] = e.pop("wrap")
        return e
    return None


def expand_shells(elements: List[Any], defaults: Dict[str, Any] = None, _root: List[Any] = None) -> List[Any]:
    """Replace every {"shell": ...} entry (also inside groups) with a tube part that follows
    its body part. The body part is looked up anywhere in the model."""
    root = elements if _root is None else _root
    out = []
    for e in elements or []:
        shell = _as_shell(e)
        if shell is not None:
            src = _find(root, shell["shell"])
            if src is None:
                raise ShapeError("shell %r: no tube part named %r to follow" % (shell.get("name"), shell["shell"]))
            out.append(shell_part(shell, src, defaults or {}))
        elif isinstance(e, dict) and isinstance(e.get("group"), list):
            g = dict(e)
            g["group"] = expand_shells(e["group"], defaults, root)
            out.append(g)
        else:
            out.append(e)
    return out
