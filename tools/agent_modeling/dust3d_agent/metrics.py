"""Objective checks on an exported GLB, so an agent gets numbers as well as pictures."""

from __future__ import annotations

from typing import Any, Dict, List

import numpy as np

from . import glb as glbmod
from . import render


def _weld(pos: np.ndarray, tol: float) -> np.ndarray:
    key = np.round(pos / tol).astype(np.int64)
    _, inv = np.unique(key, axis=0, return_inverse=True)
    return inv.reshape(-1)


def _components(n: int, tris: np.ndarray) -> np.ndarray:
    parent = np.arange(n)

    def find(a):
        root = a
        while parent[root] != root:
            root = parent[root]
        while parent[a] != root:
            parent[a], a = root, parent[a]
        return root

    for a, b, c in tris:
        ra, rb, rc = find(a), find(b), find(c)
        parent[rb] = ra
        parent[find(rc)] = ra
    return np.array([find(i) for i in range(n)])


def analyze(path: str) -> Dict[str, Any]:
    g = glbmod.load(path)
    r: Dict[str, Any] = {"file": path, "warnings": []}
    W: List[str] = r["warnings"]
    allpos, alltri, off = [], [], 0
    for p in g.primitives:
        allpos.append(p.positions.astype(np.float64))
        alltri.append(p.indices + off)
        off += len(p.positions)
    if not allpos:
        r["warnings"].append("GLB has no geometry")
        return r
    pos = np.concatenate(allpos)
    tri = np.concatenate(alltri)
    lo, hi = pos.min(0), pos.max(0)
    size = hi - lo
    r["vertices"] = int(len(pos))
    r["triangles"] = int(len(tri))
    r["bbox_min"] = [round(float(v), 4) for v in lo]
    r["bbox_max"] = [round(float(v), 4) for v in hi]
    r["size_xyz"] = [round(float(v), 4) for v in size]
    diag = float(np.linalg.norm(size))

    wid = _weld(pos, max(diag * 1e-5, 1e-7))
    wt = wid[tri]
    wt = wt[(wt[:, 0] != wt[:, 1]) & (wt[:, 1] != wt[:, 2]) & (wt[:, 0] != wt[:, 2])]
    e = np.sort(np.concatenate([wt[:, [0, 1]], wt[:, [1, 2]], wt[:, [2, 0]]]), axis=1)
    _, counts = np.unique(e, axis=0, return_counts=True)
    r["open_edges"] = int((counts == 1).sum())
    r["nonmanifold_edges"] = int((counts > 2).sum())
    comp = _components(int(wid.max()) + 1, wt)
    used = np.unique(wt)
    labels = comp[used]
    uniq, sizes = np.unique(labels, return_counts=True)
    r["islands"] = int(len(uniq))
    if r["open_edges"]:
        W.append("mesh has %d open (boundary) edges - not watertight" % r["open_edges"])
    if r["nonmanifold_edges"]:
        W.append("mesh has %d non-manifold edges" % r["nonmanifold_edges"])

    # orientation: a closed, outward-facing mesh has positive signed volume
    v0, v1, v2 = pos[tri[:, 0]], pos[tri[:, 1]], pos[tri[:, 2]]
    vol = float(np.einsum("ij,ij->i", v0, np.cross(v1, v2)).sum() / 6.0)
    r["signed_volume"] = round(vol, 6)
    if vol < 0:
        W.append("mesh encloses negative volume - surfaces are inside out")

    # seam quality: sliver triangles (min angle < 3 deg) usually come from unions Dust3D
    # could not bridge (parts meeting in more than one ring, or grazing contact)
    e0, e1, e2 = v1 - v0, v2 - v1, v0 - v2
    def ang(a, b):
        c = -np.einsum("ij,ij->i", a, b) / np.maximum(np.linalg.norm(a, axis=1) * np.linalg.norm(b, axis=1), 1e-12)
        return np.degrees(np.arccos(np.clip(c, -1, 1)))
    min_ang = np.minimum(np.minimum(ang(e2, e0), ang(e0, e1)), ang(e1, e2))
    r["sliver_triangles"] = int((min_ang < 3.0).sum())

    # left/right symmetry about x = 0 (Dust3D mirrors about the origin)
    rng = np.random.default_rng(0)
    sample = pos[rng.choice(len(pos), min(1500, len(pos)), replace=False)]
    mir = sample * np.array([-1, 1, 1])
    d = np.full(len(mir), np.inf)
    for chunk in np.array_split(pos, max(1, len(pos) // 4000)):
        d = np.minimum(d, np.sqrt(((mir[:, None, :] - chunk[None]) ** 2).sum(-1)).min(1))
    r["asymmetry"] = round(float(np.median(d) / max(diag, 1e-9)), 5)

    r["ground_y"] = round(float(lo[1]), 4)

    if g.skin is not None:
        names = [g.nodes[j].get("name", str(j)) for j in g.skin["joints"]]
        r["bones"] = names
        wsum = np.zeros(len(names))
        unweighted = 0
        for p in g.primitives:
            if p.joints is None:
                continue
            for k in range(p.joints.shape[1]):
                np.add.at(wsum, p.joints[:, k], p.weights[:, k])
            unweighted += int((p.weights.sum(1) < 1e-6).sum())
        r["unweighted_vertices"] = unweighted
        dead = [n for n, s in zip(names, wsum) if s < 1e-6 and n != "Root"]
        r["bones_without_vertices"] = dead
        if unweighted:
            W.append("%d vertices have no bone weights" % unweighted)
    else:
        r["bones"] = []

    anims = []
    for a in g.animations:
        ts = np.linspace(0, a["duration"], 12, endpoint=False) if a["duration"] > 0 else [0.0]
        rest = np.concatenate([q for q, _ in render._gather(g, None)])
        maxd, lo_y = 0.0, np.inf
        for t in ts:
            posed = np.concatenate([q for q, _ in render._gather(g, a, float(t))])
            maxd = max(maxd, float(np.linalg.norm(posed - rest, axis=1).max()))
            lo_y = min(lo_y, float(posed[:, 1].min()))
        info = {"name": a["name"], "duration": round(a["duration"], 3),
                "max_vertex_motion_rel": round(maxd / max(diag, 1e-9), 4),
                "min_y_rel": round(lo_y / max(diag, 1e-9), 4)}
        if info["max_vertex_motion_rel"] < 0.005:
            W.append("animation %r barely moves anything" % a["name"])
        if info["max_vertex_motion_rel"] > 3:
            W.append("animation %r moves vertices very far (%.1fx model size) - likely broken rig" % (a["name"], info["max_vertex_motion_rel"]))
        anims.append(info)
    r["animations"] = anims
    return r
