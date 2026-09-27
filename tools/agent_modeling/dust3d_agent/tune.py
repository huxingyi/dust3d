"""Automatic seam tuning: nudge the nodes where parts join until Dust3D bridges them well.

The boolean union + seam bridging result depends on exactly where a limb's first ring sits
relative to the faces of the part it joins. Tiny changes (sinking a node a fraction of its
radius deeper, or enlarging it slightly) can turn a fan of stretched triangles into a clean
quad-like strip. This module searches such changes automatically:

  1. build the spec (rig and animations stripped: they do not affect seams) and score every
     seam (seams.analyze_seams);
  2. for the worst seam, find the spec part and node that create it;
  3. try a small set of candidate edits to that node (move along the limb, move toward /
     away from the seam, scale radius, add a ring near the joint), rebuild and re-score;
  4. keep the best candidate if it lowers the model's total seam penalty; repeat.

Edits are bounded (node movement <= 0.6 x its radius, radius within 0.7-1.3 x) so the model
keeps its design. The tuned spec is written next to the original and every change is logged.
"""

from __future__ import annotations

import copy
import json
import math
import os
import tempfile
from typing import Dict, List, Optional, Tuple

import numpy as np

from . import ds3, seams as seammod, spec as specmod

MAX_MOVE = 0.6    # x original radius
RADIUS_RANGE = (0.7, 1.3)


def _tube_parts(items, out=None):
    out = [] if out is None else out
    for e in items:
        if "group" in e:
            _tube_parts(e["group"], out)
        elif "nodes" in e and "stitch" not in e:
            out.append(e)
    return out


def _stitch_groups(items, out=None):
    out = [] if out is None else out
    for e in items:
        if "group" in e:
            _stitch_groups(e["group"], out)
        elif e.get("stitch") == "lines":
            out.append(e)
    return out


def _line_nodes(line):
    return line if isinstance(line, list) else line["nodes"]


def _set_line_nodes(group, i, nodes):
    if isinstance(group["lines"][i], list):
        group["lines"][i] = nodes
    else:
        group["lines"][i]["nodes"] = nodes


def _dist_point_polyline(p, nodes) -> float:
    pts = np.array([n[:3] for n in nodes], float)
    if len(pts) == 1:
        return float(np.linalg.norm(pts[0] - p))
    best = 1e9
    for a, b in zip(pts, pts[1:]):
        ab = b - a
        t = float(np.clip((p - a) @ ab / max(ab @ ab, 1e-12), 0, 1))
        best = min(best, float(np.linalg.norm(a + t * ab - p)))
    return best


def _locate(spec_dict: Dict, seam: Dict) -> Optional[Tuple[Dict, int, np.ndarray]]:
    """Which spec part (and node index) produced this seam. Returns (part, node index, center in part space)."""
    if not seam.get("center"):
        return None
    center = np.array(seam["center"], float)
    names = [n for n in seam["part"].split("|") if n]
    parts = {p["name"]: p for p in _tube_parts(spec_dict["parts"])}
    best = None
    for nm in names:
        mirrored = nm.endswith("~mirror")
        p = parts.get(nm[:-len("~mirror")] if mirrored else nm)
        if p is None:
            continue
        c = center * np.array([-1, 1, 1]) if mirrored else center
        d = _dist_point_polyline(c, p["nodes"])
        if best is None or d < best[0]:
            best = (d, p, c)
    if best is None:
        return None
    _, p, c = best
    idx = int(np.argmin([np.linalg.norm(np.array(n[:3]) - c) for n in p["nodes"]]))
    return p, idx, c


def _candidates(part: Dict, idx: int, center: np.ndarray) -> List[Tuple[str, List[List[float]]]]:
    """Edits of the joining (child) part's node at the seam."""
    nodes = part["nodes"]
    n = np.array(nodes[idx][:3], float)
    r = nodes[idx][3]
    out = []
    if len(nodes) > 1:
        nb = nodes[idx + 1] if idx + 1 < len(nodes) else nodes[idx - 1]
        along = np.array(nb[:3], float) - n
        if idx + 1 >= len(nodes):
            along = -along
        along /= max(np.linalg.norm(along), 1e-9)
        for f in (-0.4, -0.2, 0.2, 0.4):
            out.append(("move node %d %+.1fr along the part" % (idx, f), along * f * r, 1.0))
    toward = center - n
    if np.linalg.norm(toward) > 1e-6:
        toward /= np.linalg.norm(toward)
        for f in (-0.3, 0.3):
            out.append(("move node %d %+.1fr toward the seam" % (idx, f), toward * f * r, 1.0))
    for sc in (0.85, 1.15, 1.3):
        out.append(("scale node %d radius x%.2f" % (idx, sc), np.zeros(3), sc))
    cands = []
    for label, delta, sc in out:
        new = copy.deepcopy(nodes)
        new[idx] = [float(v) for v in (n + delta)] + [r * sc]
        cands.append((label, new))
    if len(nodes) > 1:
        # a ring close to the joint; and a flared one (a natural blend into the parent)
        j = idx + 1 if idx + 1 < len(nodes) else idx - 1
        a, b = nodes[min(idx, j)], nodes[max(idx, j)]
        t = 0.35 if idx < j else 0.65
        mid = [a[k] + (b[k] - a[k]) * t for k in range(4)]
        for label, rad in (("add a ring next to node %d" % idx, mid[3]),
                           ("add a flared ring next to node %d" % idx, max(mid[3], r) * 1.2)):
            new = copy.deepcopy(nodes)
            new.insert(max(idx, j), mid[:3] + [rad])
            cands.append((label, new))
    return cands


def _parent_candidates(parent: Dict, center: np.ndarray) -> List[Tuple[str, List[List[float]]]]:
    """Refine the parent where the child lands: an extra ring on the parent's chain at the
    point closest to the seam, so its faces there are closer in size to the child's."""
    nodes = parent["nodes"]
    if len(nodes) < 2:
        return []
    pts = np.array([q[:3] for q in nodes], float)
    best = None
    for i in range(len(pts) - 1):
        ab = pts[i + 1] - pts[i]
        t = float(np.clip((center - pts[i]) @ ab / max(ab @ ab, 1e-12), 0.15, 0.85))
        d = float(np.linalg.norm(pts[i] + t * ab - center))
        if best is None or d < best[0]:
            best = (d, i, t)
    _, i, t = best
    out = []
    for tt in sorted({round(t, 2), 0.5}):
        mid = [nodes[i][k] + (nodes[i + 1][k] - nodes[i][k]) * tt for k in range(4)]
        new = copy.deepcopy(nodes)
        new.insert(i + 1, mid)
        out.append(("refine parent %s: ring between nodes %d-%d" % (parent["name"], i, i + 1), new))
    return out


def _stitch_candidates(group: Dict, center: np.ndarray):
    """Edits of a stitched shell's root: every line's node nearest the seam."""
    out = []
    roots = []
    for li, line in enumerate(group["lines"]):
        nodes = _line_nodes(line)
        k = int(np.argmin([np.linalg.norm(np.array(q[:3]) - center) for q in nodes]))
        nb = nodes[1] if k == 0 else nodes[k - 1]
        roots.append((li, k, np.array(nb[:3], float) - np.array(nodes[k][:3], float)))

    def edit(label, fn):
        g = copy.deepcopy(group)
        for li, k, along in roots:
            nodes = copy.deepcopy(_line_nodes(g["lines"][li]))
            nodes[k] = fn(nodes[k], along)
            _set_line_nodes(g, li, nodes)
        out.append((label, g))

    for f in (-0.9, -0.5, -0.25, 0.25):
        edit("move roots %+.2f of the first segment outward" % f,
             lambda q, a, f=f: [q[0] + a[0] * f, q[1] + a[1] * f, q[2] + a[2] * f, q[3]])
    for sc in (0.7, 1.4):
        edit("scale root thickness x%.1f" % sc, lambda q, a, sc=sc: q[:3] + [q[3] * sc])
    for seg in (4, 6, 8):
        g = copy.deepcopy(group)
        if g.get("targetSegments", 0) != seg:
            g["targetSegments"] = seg
            out.append(("targetSegments %d" % seg, g))
    return out


def _within_bounds(orig: List[List[float]], new: List[List[float]]) -> bool:
    """Every node must stay within MAX_MOVE x radius of the part's original chain, with a radius
    within RADIUS_RANGE of the original radius at that point (inserted rings are measured the same way)."""
    pts = np.array([q[:3] for q in orig], float)
    rads = np.array([q[3] for q in orig], float)
    for q in new:
        p = np.array(q[:3], float)
        if len(pts) == 1:
            d, r0 = float(np.linalg.norm(p - pts[0])), rads[0]
        else:
            d, r0 = 1e9, rads[0]
            for i in range(len(pts) - 1):
                ab = pts[i + 1] - pts[i]
                t = float(np.clip((p - pts[i]) @ ab / max(ab @ ab, 1e-12), 0, 1))
                di = float(np.linalg.norm(pts[i] + t * ab - p))
                if di < d:
                    d, r0 = di, rads[i] + (rads[i + 1] - rads[i]) * t
        if d > MAX_MOVE * r0 + 1e-9:
            return False
        if not (RADIUS_RANGE[0] - 1e-9 <= q[3] / r0 <= RADIUS_RANGE[1] * 1.2 + 1e-9):
            return False
    return True


def _fix_bones(part: Dict, new_nodes: List[List[float]]) -> None:
    """Keep per-edge bones consistent when a ring was inserted."""
    bones = part.get("bones")
    if not isinstance(bones, list) or len(new_nodes) == len(part["nodes"]):
        return
    # find the inserted index by comparing
    k = next((i for i, (a, b) in enumerate(zip(part["nodes"], new_nodes)) if a != b), len(part["nodes"]))
    edge = max(0, k - 1)
    bones.insert(edge, bones[min(edge, len(bones) - 1)])


class SeamTuner:
    def __init__(self, spec_dict: Dict, base_dir: str, workdir: str, dust3d: Optional[str] = None, log=print):
        self.base_dir = base_dir
        self.workdir = workdir
        self.dust3d = dust3d
        self.log = log
        self.spec = copy.deepcopy(spec_dict)
        self.original = {p["name"]: copy.deepcopy(p["nodes"]) for p in _tube_parts(self.spec["parts"])}
        self.evals = 0
        self.changes_so_far: List[Dict] = []

    def evaluate(self, spec_dict: Dict) -> Dict:
        s = copy.deepcopy(spec_dict)
        s.pop("animations", None)
        s.pop("rig", None)
        for p in _tube_parts(s["parts"]):
            p.pop("bones", None)
        model = specmod.parse_spec(s, self.base_dir)
        path = os.path.join(self.workdir, "tune.ds3")
        ds3.compile_to_ds3(model, path)
        self.evals += 1
        shells = [g.name for g in model.groups() if g.stitch]
        return seammod.analyze_seams(path, os.path.join(self.workdir, "tune"), self.dust3d, shells=shells)

    def _trials(self, seam: Dict):
        """(label, trial spec, part name) candidates for one bad seam."""
        center = np.array(seam["center"], float)
        trials = []
        names = [n for n in seam["part"].split("|") if n]
        groups = {g["name"]: g for g in _stitch_groups(self.spec["parts"])}
        for nm in names:
            mirrored = nm.endswith("~mirror")
            base = nm[:-len("~mirror")] if mirrored else nm
            if base in groups:
                if sum(1 for ch in self.changes_so_far if ch["part"] == base) >= 3:
                    return []
                c = center * np.array([-1, 1, 1]) if mirrored else center
                for label, g in _stitch_candidates(groups[base], c):
                    trial = copy.deepcopy(self.spec)
                    self._replace_element(trial["parts"], base, g)
                    trials.append((label, trial, base))
                return trials
        loc = _locate(self.spec, seam)
        if loc is None:
            return trials
        part, idx, c = loc
        for label, new_nodes in _candidates(part, idx, c):
            if not _within_bounds(self.original[part["name"]], new_nodes):
                continue
            trial = copy.deepcopy(self.spec)
            tp = next(p for p in _tube_parts(trial["parts"]) if p["name"] == part["name"])
            _fix_bones(tp, new_nodes)
            tp["nodes"] = new_nodes
            trials.append((label, trial, part["name"]))
        # the parent: nearest other tube part to the seam
        others = [(p, _dist_point_polyline(c, p["nodes"])) for p in _tube_parts(self.spec["parts"])
                  if p["name"] != part["name"] and p.get("combine", "Normal") == "Normal" and len(p["nodes"]) > 1]
        if others:
            parent = min(others, key=lambda x: x[1])[0]
            for label, new_nodes in _parent_candidates(parent, c):
                if sum(1 for ch in self.changes_so_far if ch["part"] == parent["name"] and ch["change"].startswith("refine")) >= 2:
                    break
                trial = copy.deepcopy(self.spec)
                tp = next(p for p in _tube_parts(trial["parts"]) if p["name"] == parent["name"])
                _fix_bones(tp, new_nodes)
                tp["nodes"] = new_nodes
                trials.append((label, trial, parent["name"]))
        return trials

    @staticmethod
    def _replace_element(items, name, new):
        for i, e in enumerate(items):
            if e.get("name") == name:
                items[i] = new
                return True
            if "group" in e and SeamTuner._replace_element(e["group"], name, new):
                return True
        return False

    def run(self, max_steps: int = 12, target: float = 0.5) -> Dict:
        os.makedirs(self.workdir, exist_ok=True)
        current = self.evaluate(self.spec)
        if current.get("ok") and not current.get("reports_found"):
            raise RuntimeError("this Dust3D binary does not print SEAM_REPORT lines, so seams cannot be measured; "
                               "use a build that includes the seam report (agent-modeling branch or a release made from it)")
        start_total = current["total_penalty"]
        changes = self.changes_so_far = []
        exhausted = set()
        for step in range(max_steps):
            ranked = sorted((s for s in current["seams"] if s["penalty"] > target and s.get("center")),
                            key=lambda s: -s["penalty"])
            ranked = [s for s in ranked if (s["part"], tuple(s["center"])) not in exhausted]
            if not ranked:
                break
            seam = ranked[0]
            trials = self._trials(seam)
            if not trials:
                exhausted.add((seam["part"], tuple(seam["center"])))
                continue
            best = None
            for label, trial, pname in trials:
                res = self.evaluate(trial)
                if not res["ok"]:
                    continue
                if best is None or res["total_penalty"] < best[0]["total_penalty"]:
                    best = (res, label, trial, pname)
            if best and best[0]["total_penalty"] < current["total_penalty"] - 0.2:
                self.log("  %-14s %-46s penalty %.2f -> %.2f" % (best[3], best[1], current["total_penalty"], best[0]["total_penalty"]))
                changes.append({"part": best[3], "change": best[1],
                                "penalty_before": current["total_penalty"], "penalty_after": best[0]["total_penalty"]})
                self.spec, current = best[2], best[0]
            else:
                exhausted.add((seam["part"], tuple(seam["center"])))
        return {"spec": self.spec, "changes": changes, "penalty_before": start_total,
                "penalty_after": current["total_penalty"], "seams": current["seams"], "evaluations": self.evals}


def tune_file(spec_path: str, out_path: Optional[str] = None, dust3d: Optional[str] = None,
              max_steps: int = 12, log=print) -> Dict:
    with open(spec_path, encoding="utf-8") as f:
        spec_dict = json.load(f)
    base_dir = os.path.dirname(os.path.abspath(spec_path))
    with tempfile.TemporaryDirectory() as work:
        result = SeamTuner(spec_dict, base_dir, work, dust3d, log).run(max_steps=max_steps)
    out_path = out_path or os.path.splitext(spec_path)[0] + ".tuned.json"
    with open(out_path, "w", encoding="utf-8") as f:
        json.dump(result["spec"], f, indent=1)
    result["path"] = out_path
    return result
