"""Seam (join) quality: how well Dust3D bridged every place where two parts were unioned.

Dust3D combines parts with a boolean union, then its recombiner removes the jagged
triangles along each intersection curve and bridges the two sides with a strip of
quads. Built with DUST3D_SEAM_REPORT set, the engine prints one line per union:

    SEAM_REPORT + tail 1 1,1,1,12,8,0,0.50,-0.47,0.11
                 |  |   \\ per island: bridged, loops on each side, loop vertex counts, centre xyz, radius
                 |  joined component(s)
                 union (+) or difference (-)

The engine flag says whether a bridge was built; it does not say whether the result has
good edge flow (a small loop bridged to a big, irregular loop gives fans of long
triangles and pinches). So every seam is also scored from the exported topology (the
OBJ keeps Dust3D's quads): the faces around the seam should be mostly quads, with no
fan vertices and no sliver faces.
"""

from __future__ import annotations

import math
import os
from typing import Dict, List

import numpy as np

from . import export as exportmod
from .wire import load_obj

# thresholds calibrated on the hand-made reference models (see tests)
MAX_TRIANGLE_SHARE = 0.45
MAX_VALENCE = 7
MIN_ANGLE_DEG = 12.0
MAX_BAD_FACE_SHARE = 0.18


def parse_reports(log: str) -> List[Dict]:
    out = []
    for line in log.splitlines():
        if not line.startswith("SEAM_REPORT "):
            continue
        tok = line.split()
        if len(tok) >= 4 and tok[3] == "failed":
            # the boolean failed and Dust3D dropped these parts (newer engines report it)
            out.append({"method": tok[1], "part": tok[2], "islands": [], "failed": True})
            continue
        method, names, n = tok[1], tok[2], int(tok[3])
        islands = []
        for t in tok[4:4 + n]:
            v = t.split(",")
            isl = {"bridged": v[0] == "1", "loops": [int(v[1]), int(v[2])],
                   "loop_vertices": [int(v[3]), int(v[4])],
                   "center": [float(v[5]), float(v[6]), float(v[7])],
                   "radius": float(v[8]) if len(v) > 8 else 0.0}
            if len(v) >= 15:
                isl.update(bridge_triangles=int(float(v[9])), min_angle=float(v[10]), max_fan=int(float(v[11])),
                           max_width=float(v[12]), loop_edge=[float(v[13]), float(v[14])])
            islands.append(isl)
        out.append({"method": method, "part": names, "islands": islands})
    return out


def _face_angles(p: np.ndarray) -> List[float]:
    n = len(p)
    out = []
    for i in range(n):
        a, b, c = p[i - 1], p[i], p[(i + 1) % n]
        u, v = a - b, c - b
        nu, nv = np.linalg.norm(u), np.linalg.norm(v)
        if nu < 1e-12 or nv < 1e-12:
            out.append(0.0)
            continue
        out.append(math.degrees(math.acos(max(-1.0, min(1.0, float(u @ v / (nu * nv)))))))
    return out


def score_region(verts: np.ndarray, faces: List[List[int]], center, radius: float) -> Dict:
    c = np.asarray(center)
    r = max(radius * 1.3, 1e-4)
    near = np.linalg.norm(verts - c, axis=1) <= r
    region = [i for i, f in enumerate(faces) if near[f].any()]
    if not region:
        return {"faces": 0}
    tri = sum(1 for i in region if len(faces[i]) == 3)
    valence: Dict[int, int] = {}
    bad = 0
    min_angle = 180.0
    for i in region:
        f = faces[i]
        ang = _face_angles(verts[f])
        m = min(ang)
        min_angle = min(min_angle, m)
        if m < MIN_ANGLE_DEG:
            bad += 1
        for k in f:
            if near[k]:
                valence[k] = valence.get(k, 0) + 1
    maxval = max(valence.values()) if valence else 0
    return {"faces": len(region), "triangle_share": round(tri / len(region), 3),
            "max_valence": maxval, "bad_faces": bad, "bad_face_share": round(bad / len(region), 3),
            "min_angle": round(min_angle, 1), "face_ids": region}


# Targets, calibrated on the 129 bridged seams of the hand-made reference models
# (median fan 5, 95th percentile 8; median smallest angle 9.5 degrees).
FAN_BAD = 8
MIN_ANGLE_BAD = 5.0
EDGE_RATIO_BAD = 4.5  # 95th percentile of the hand-made reference seams
WIDTH_RATIO_BAD = 1.35
NO_SEAM_PENALTY = 15.0  # a floating part is worse than any imperfect join
FAILED_PENALTY = 40.0  # a failed boolean drops the part from the mesh


def seam_penalty(island: Dict, islands_in_union: int = 1, shell: bool = False) -> float:
    """0 for a clean seam; grows with every defect. Used to compare candidate adjustments."""
    p = 0.0
    if islands_in_union > 1:
        p += 5.0
    if not island.get("bridged"):
        # No bridge was built, so there is no bridge geometry to score (a missing loop
        # reports edge length 0, which would otherwise blow the ratio terms up).
        return round(p + 10.0, 3)
    fan = island.get("max_fan", 0)
    p += max(0, fan - 6) * 1.0
    ang = island.get("min_angle", 90.0)
    p += max(0.0, 8.0 - ang) * 0.3
    e = island.get("loop_edge") or [1.0, 1.0]
    ratio = max(e) / max(min(e), 1e-9)
    if not shell:  # a stitched shell's root loop is mostly tiny across-the-thickness edges
        p += max(0.0, ratio - 2.0)
    wr = island.get("max_width", 0.0) / max(island.get("radius", 0.0), 1e-9)
    p += max(0.0, wr - 1.25) * 4.0
    return round(p, 3)


def judge(island: Dict, islands_in_union: int = 1, shell: bool = False) -> List[str]:
    why = []
    if not island["bridged"]:
        why.append("not bridged (%d/%d edge loops)" % tuple(island["loops"]))
        return why
    if islands_in_union > 1:
        why.append("meets the parts before it in %d separate places" % islands_in_union)
    if island.get("max_fan", 0) >= FAN_BAD:
        why.append("fan: %d bridge triangles share one vertex" % island["max_fan"])
    if island.get("min_angle", 90) < MIN_ANGLE_BAD:
        why.append("sliver bridge triangle (%.1f deg)" % island["min_angle"])
    e = island.get("loop_edge")
    if e and not shell and max(e) > EDGE_RATIO_BAD * max(min(e), 1e-9):
        why.append("edge lengths on the two sides differ %.1fx (resolution mismatch)" % (max(e) / max(min(e), 1e-9)))
    if island.get("max_width", 0) > WIDTH_RATIO_BAD * max(island.get("radius", 0), 1e-9):
        why.append("bridge spans a wide gap (%.2f vs seam radius %.2f): stretched/pinched" % (island["max_width"], island["radius"]))
    return why


def seams_from_log(log: str, shells=()) -> Dict:
    """Score every seam reported in a Dust3D log (run with DUST3D_SEAM_REPORT set).
    shells: names of stitched-shell groups (their seams skip the resolution check)."""
    shells = set(shells)
    reports = parse_reports(log)
    seams = []
    for rep in reports:
        if rep.get("failed"):
            seams.append({"part": rep["part"], "method": rep["method"], "bridged": False, "center": None,
                          "failed": True, "penalty": FAILED_PENALTY,
                          "problems": ["boolean failed, so Dust3D dropped it (coincident or grazing surfaces): "
                                       "move or resize it slightly"]})
            continue
        if not rep["islands"]:
            seams.append({"part": rep["part"], "method": rep["method"], "bridged": False, "center": None,
                          "penalty": NO_SEAM_PENALTY, "problems": ["does not touch the parts before it (no seam)"]})
            continue
        n = len(rep["islands"])
        shell = any(nm.split("~")[0] in shells for nm in rep["part"].split("|"))
        for isl in rep["islands"]:
            seams.append(dict({k: v for k, v in isl.items()}, part=rep["part"], method=rep["method"],
                              center=[round(c, 4) for c in isl["center"]], radius=round(isl["radius"], 4),
                              penalty=seam_penalty(isl, n, shell), problems=judge(isl, n, shell)))
    bad = [s for s in seams if s["problems"]]
    # newer engines announce support once per run, so a model with no unions is not mistaken
    # for a Dust3D build without the seam report
    supported = len(reports) > 0 or "SEAM_REPORT_SUPPORTED" in log
    return {"ok": True, "seams": seams, "bad": bad, "reports_found": supported,
            "total_penalty": round(sum(s["penalty"] for s in seams), 3)}


def analyze_seams(ds3_path: str, workdir: str, dust3d: str = None, timeout: int = 300, shells=()) -> Dict:
    """Export OBJ with seam reporting on, and score every seam."""
    os.makedirs(workdir, exist_ok=True)
    obj = os.path.join(workdir, os.path.splitext(os.path.basename(ds3_path))[0] + "_topology.obj")
    ex = export_with_seams(ds3_path, [obj], dust3d, timeout)
    failed_combines = any(rep.get("failed") for rep in parse_reports(ex.get("log", "")))
    if not ex["ok"] and not (failed_combines and ex["outputs"].get(obj)):
        return {"ok": False, "seams": [], "bad": [], "export": ex, "total_penalty": float("inf")}
    # A failed boolean makes the export "unsuccessful" but still scores: the failed union is
    # reported by name, which lets the tuner move that part.
    r = seams_from_log(ex["log"], shells)
    r["obj"] = obj
    r["export_ok"] = ex["ok"]
    return r


def export_with_seams(ds3_path: str, outputs: List[str], dust3d: str = None, timeout: int = 300) -> Dict:
    before = os.environ.get("DUST3D_SEAM_REPORT")
    os.environ["DUST3D_SEAM_REPORT"] = "1"
    try:
        return exportmod.export(ds3_path, outputs, dust3d, timeout=timeout, full_log=True)
    finally:
        if before is None:
            os.environ.pop("DUST3D_SEAM_REPORT", None)
        else:
            os.environ["DUST3D_SEAM_REPORT"] = before


def closeups(obj_path: str, bad: List[Dict], out_prefix: str, limit: int = 6) -> List[str]:
    """Wireframe close-ups (quads grey, triangles orange) of the worst seams."""
    from . import wire
    files = []
    for i, s in enumerate(sorted(bad, key=lambda s: -s.get("penalty", 0))[:limit]):
        if not s.get("center"):
            continue
        name = s["part"].split("|")[0].replace("~", "_")
        path = "%s_seam_%s.png" % (out_prefix, name)
        if path in files:
            path = "%s_seam_%s_%d.png" % (out_prefix, name, i)
        wire.seam_closeup(obj_path, s["center"], max(3.0 * s.get("radius", 0.05), 0.12), path,
                          views=("three_quarter", "left", "front", "top"), size=300, label=name)
        files.append(path)
    return files
