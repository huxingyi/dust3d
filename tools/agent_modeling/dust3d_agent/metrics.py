"""Objective checks on an exported GLB, so an agent gets numbers as well as pictures."""

from __future__ import annotations

from typing import Any, Dict, List, Optional

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


def analyze(path: str, clips: Optional[List[Dict[str, Any]]] = None) -> Dict[str, Any]:
    """Mesh, rig and animation metrics for an exported .glb.

    clips: the toolkit's clip list (name, type, frameCount, durationSeconds, loop). With it,
    every clip is also checked frame by frame for game use: loops must wrap without a
    seam, no clip may pop between two frames, and one-shot actions (attack, hurt, ...)
    must start and end at the rest pose so they blend with the loops.
    """
    g = glbmod.load(path)
    r: Dict[str, Any] = {"file": path, "warnings": []}
    W: List[str] = r["warnings"]
    allpos, alltri, allgroup, off = [], [], [], 0
    group_ids: Dict[str, int] = {}
    for p in g.primitives:
        allpos.append(p.positions.astype(np.float64))
        alltri.append(p.indices + off)
        # equipment variants overlap the body and each other by design: check each mesh's
        # topology on its own
        allgroup.append(np.full(len(p.positions), group_ids.setdefault(p.mesh_name, len(group_ids))))
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

    tol = max(diag * 1e-5, 1e-7)
    group = np.concatenate(allgroup).astype(np.float64)
    wid = _weld(np.concatenate([pos, group[:, None] * tol * 1e4], 1), tol)
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
    clip_info = {c["name"]: c for c in (clips or []) if c.get("type") != "Pose"}
    # One-shots blend from and back to the base loop in a game: its first frame is a valid
    # start/end pose too (e.g. when cloth or hair physics settles away from the rest pose).
    idle_pose = None
    # the base loop: a clip named "idle", else an Idle type, else the first loop (a flyer's fly)
    loops = [c for c in (clips or []) if c.get("loop")]
    idle = (next((c for c in loops if c["name"] == "idle"), None)
            or next((c for c in loops if str(c.get("type", "")).endswith("Idle")), None)
            or (loops[0] if loops else None))
    idle_anim = next((x for x in g.animations if idle and x["name"] == idle["name"]), None)
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
        clip = clip_info.get(a["name"])
        if clip:
            if idle_anim is not None and idle_pose is None:
                # every frame of the base loop: the game blends at whatever phase it is in
                n_idle = max(1, int(idle.get("frameCount") or 12))
                d_idle = float(idle.get("durationSeconds") or idle_anim["duration"] or 1.0)
                step = max(1, n_idle // 24)
                idle_pose = [np.concatenate([q for q, _ in render._gather(g, idle_anim, d_idle * i / n_idle)])
                             for i in range(0, n_idle, step)]
            info.update(_clip_quality(g, a, clip, rest, diag, W, idle_pose))
        anims.append(info)
    r["animations"] = anims
    return r


# Clip types that are allowed to end away from the rest pose (the creature stays down).
_HELD_END = ("Die",)


def _clip_quality(g, a: dict, clip: Dict[str, Any], rest: np.ndarray, diag: float, W: List[str],
                  idle_pose: Optional[List[np.ndarray]] = None) -> Dict[str, Any]:
    """Frame-by-frame checks of one clip against how a game plays it."""
    n = max(2, int(clip.get("frameCount") or 0))
    dur = float(clip.get("durationSeconds") or a["duration"] or 1.0)
    frames = [np.concatenate([q for q, _ in render._gather(g, a, dur * i / n)]) for i in range(n)]
    steps = np.array([np.linalg.norm(frames[i + 1] - frames[i], axis=1).max() for i in range(n - 1)])
    typical = float(np.median(steps)) if len(steps) else 0.0
    biggest = float(steps.max()) if len(steps) else 0.0
    d = max(diag, 1e-9)
    out: Dict[str, Any] = {"typical_frame_step_rel": round(typical / d, 4),
                           "max_frame_step_rel": round(biggest / d, 4)}
    name, ctype = a["name"], str(clip.get("type", ""))
    # A pop: one frame jumps much further than the frames on either side of it. Fast but
    # continuous motion (a strike, a hit snap spread over a few frames) speeds up and slows
    # down over neighbouring frames, so it is not a pop. A loop's wrap counts as a neighbour.
    ext = list(steps)
    if clip.get("loop"):
        ext.append(float(np.linalg.norm(frames[0] - frames[-1], axis=1).max()))
    worst, worst_i = 0.0, -1
    for i, st in enumerate(ext):
        # neighbours within two frames: a fast cycle (a wing beat) has a turning point next to
        # a fast frame, a pop stands alone
        nb = [ext[j % len(ext)] for j in (i - 2, i - 1, i + 1, i + 2) if clip.get("loop") or 0 <= j < len(ext)]
        ratio = st / max(max(nb) if nb else 0.0, 1e-9)
        # the wrap of a loop is held to a finer standard: an idle twitching once per cycle shows
        is_wrap = clip.get("loop") and i == len(ext) - 1
        if st > (0.008 if is_wrap else 0.04) * d and ratio > 3.0 and ratio > worst:
            worst, worst_i = ratio, i
    if worst_i >= 0:
        j = (worst_i + 1) % n
        if clip.get("loop") and worst_i == len(ext) - 1:
            W.append("looping animation %r jumps when it wraps (%.3f of the model size, %.1fx the frames around it)"
                     % (name, ext[worst_i] / d, worst))
        else:
            W.append("animation %r pops between frames %d and %d (%.3f of the model size, %.1fx the frames around it)"
                     % (name, worst_i, j, ext[worst_i] / d, worst))
    if clip.get("loop"):
        seam = float(np.linalg.norm(frames[0] - frames[-1], axis=1).max())
        out["loop_seam_rel"] = round(seam / d, 4)
    else:
        refs = [rest] + [p for p in (idle_pose or []) if len(p) == len(rest)]
        start = min(float(np.linalg.norm(frames[0] - r, axis=1).max()) for r in refs)
        # the last key of a one-shot is its final pose
        end_pose = np.concatenate([q for q, _ in render._gather(g, a, a["duration"])])
        end = min(float(np.linalg.norm(end_pose - r, axis=1).max()) for r in refs)
        out["start_offset_rel"] = round(start / d, 4)
        out["end_offset_rel"] = round(end / d, 4)
        if not any(ctype.endswith(k) for k in _HELD_END):
            # engines blend back to the loop over ~0.15 s; beyond ~6% of the model size that
            # blend reads as a visible slide or snap
            if end > 0.06 * d:
                W.append("one-shot animation %r ends %.3f of the model size away from the rest and idle poses - it will pop when the game blends back to idle"
                         % (name, end / d))
    return out
