"""Game-ready finishing of a Dust3D export (run by `build` after Dust3D writes the .glb).

Dust3D writes one skinned mesh with the rig and its clips, and labels every vertex with
the part it came from (the ``_PART`` attribute; ``mesh.extras.dust3dParts`` lists the parts'
names). From that this module does what a game needs on top:

* equipment slots: parts whose component name ends in `` @slot/variant`` ("vest @armor/2",
  the spec's ``"slot": "armor/2"``) become meshes of their own, one per variant, named
  ``slot_<slot>_<variant>`` and skinned to the same skeleton. The game shows one variant
  per slot (the equipped item) and hides the others.
* skin-weight smoothing (``"smoothWeights": N``): N passes that blend each vertex's bone
  weights with its neighbours' across joints (shoulders, hips), up to 4 bones per vertex.
  Parts that are metal (``metallic`` >= 0.5) or ``hard`` keep their rigid weights.
* posed clips (``{"type": "Pose", ...}``): clips keyed from a few poses (bone rotations in
  degrees about the world axes, as seen in the rest pose), optionally on top of another clip.
* clip events: ``hit`` for attacks (the moment the fastest limb peaks, or the spec's
  ``events``), ``step`` for walks and runs (each foot touching down), written to
  <name>_clips.json for the game to time effects and sounds.
* budgets: triangles of the model with its heaviest outfit on, checked against ``budget``.
"""

from __future__ import annotations

import json
import math
import struct
from typing import Any, Dict, List, Optional

import numpy as np

from .ds3 import split_component_name

_CT = {5120: np.int8, 5121: np.uint8, 5122: np.int16, 5123: np.uint16, 5125: np.uint32, 5126: np.float32}
_NC = {"SCALAR": 1, "VEC2": 2, "VEC3": 3, "VEC4": 4, "MAT4": 16}
_TYPE_OF = {1: "SCALAR", 2: "VEC2", 3: "VEC3", 4: "VEC4", 16: "MAT4"}
_CT_OF = {np.dtype(np.float32): 5126, np.dtype(np.uint16): 5123, np.dtype(np.uint32): 5125,
          np.dtype(np.uint8): 5121, np.dtype(np.int16): 5122, np.dtype(np.int8): 5120}
ATTACK_WORDS = ("Attack", "Slam", "Stab", "Strike", "Cast", "Roar", "Throw", "Kick", "Bite", "Slash")
STEP_WORDS = ("Walk", "Run", "Hop", "Strafe", "Turn", "Sprint", "Sneak")


# ---------------------------------------------------------------- glTF editing

class GlbDoc:
    """A .glb as JSON plus one byte string per buffer view; new data goes into new views,
    and save() writes only what is still referenced."""

    def __init__(self, path: str):
        with open(path, "rb") as f:
            data = f.read()
        magic, _, length = struct.unpack_from("<III", data, 0)
        if magic != 0x46546C67:
            raise ValueError("not a GLB file: %s" % path)
        off, binary, js = 12, b"", None
        while off < length:
            clen, ctype = struct.unpack_from("<II", data, off)
            chunk = data[off + 8: off + 8 + clen]
            if ctype == 0x4E4F534A:
                js = json.loads(chunk.decode("utf-8"))
            elif ctype == 0x004E4942:
                binary = bytes(chunk)
            off += 8 + clen
        self.j = js
        self.views: List[bytes] = []
        for v in js.get("bufferViews", []):
            o = v.get("byteOffset", 0)
            self.views.append(binary[o:o + v["byteLength"]])

    def read(self, ai: int) -> np.ndarray:
        a = self.j["accessors"][ai]
        v = self.j["bufferViews"][a["bufferView"]]
        raw = self.views[a["bufferView"]]
        dt = np.dtype(_CT[a["componentType"]])
        n = _NC[a["type"]]
        o = a.get("byteOffset", 0)
        stride = v.get("byteStride")
        if stride and stride != dt.itemsize * n:
            arr = np.ndarray((a["count"], n), dt, raw, o, (stride, dt.itemsize))
        else:
            arr = np.frombuffer(raw, dt, a["count"] * n, o).reshape(a["count"], n)
        return arr.copy()

    def read_float(self, ai: int) -> np.ndarray:
        a = self.j["accessors"][ai]
        arr = self.read(ai)
        if a["componentType"] != 5126:
            arr = arr.astype(np.float32)
            if a.get("normalized"):
                arr /= float(np.iinfo(_CT[a["componentType"]]).max)
        return arr.astype(np.float32)

    def add(self, arr: np.ndarray, target: Optional[int] = None, minmax: bool = False, normalized: bool = False) -> int:
        arr = np.ascontiguousarray(arr)
        if arr.ndim == 1:
            arr = arr.reshape(-1, 1)
        n = arr.shape[1]
        self.views.append(arr.tobytes())
        view = {"buffer": 0, "byteLength": len(self.views[-1])}
        if target:
            view["target"] = target
        self.j.setdefault("bufferViews", []).append(view)
        acc = {"bufferView": len(self.views) - 1, "componentType": _CT_OF[arr.dtype], "count": int(arr.shape[0]),
               "type": _TYPE_OF[n]}
        if normalized:
            acc["normalized"] = True
        if minmax and len(arr):
            acc["min"] = [float(v) for v in arr.min(0)]
            acc["max"] = [float(v) for v in arr.max(0)]
        self.j.setdefault("accessors", []).append(acc)
        return len(self.j["accessors"]) - 1

    def save(self, path: str) -> None:
        j = self.j
        used_acc: List[int] = []

        def use(ai):
            if ai is not None and ai not in used_acc:
                used_acc.append(ai)
        for m in j.get("meshes", []):
            for p in m["primitives"]:
                for ai in p["attributes"].values():
                    use(ai)
                use(p.get("indices"))
        for s in j.get("skins", []):
            use(s.get("inverseBindMatrices"))
        for a in j.get("animations", []):
            for s in a["samplers"]:
                use(s["input"])
                use(s["output"])
        acc_map = {old: new for new, old in enumerate(used_acc)}
        used_views: List[int] = []
        for ai in used_acc:
            vi = j["accessors"][ai]["bufferView"]
            if vi not in used_views:
                used_views.append(vi)
        for img in j.get("images", []):
            if "bufferView" in img and img["bufferView"] not in used_views:
                used_views.append(img["bufferView"])
        view_map = {old: new for new, old in enumerate(used_views)}
        blob = bytearray()
        views = []
        for vi in used_views:
            v = dict(j["bufferViews"][vi])
            while len(blob) % 4:
                blob.append(0)
            v["byteOffset"] = len(blob)
            v["byteLength"] = len(self.views[vi])
            blob += self.views[vi]
            views.append(v)
        while len(blob) % 4:
            blob.append(0)
        accessors = []
        for ai in used_acc:
            a = dict(j["accessors"][ai])
            a["bufferView"] = view_map[a["bufferView"]]
            accessors.append(a)
        for m in j.get("meshes", []):
            for p in m["primitives"]:
                p["attributes"] = {k: acc_map[v] for k, v in p["attributes"].items()}
                if "indices" in p:
                    p["indices"] = acc_map[p["indices"]]
        for s in j.get("skins", []):
            if "inverseBindMatrices" in s:
                s["inverseBindMatrices"] = acc_map[s["inverseBindMatrices"]]
        for a in j.get("animations", []):
            for s in a["samplers"]:
                s["input"] = acc_map[s["input"]]
                s["output"] = acc_map[s["output"]]
        for img in j.get("images", []):
            if "bufferView" in img:
                img["bufferView"] = view_map[img["bufferView"]]
        j["accessors"] = accessors
        j["bufferViews"] = views
        j["buffers"] = [{"byteLength": len(blob)}]
        # re-read the views from the new layout so the document stays usable after saving
        self.views = [bytes(blob[v["byteOffset"]:v["byteOffset"] + v["byteLength"]]) for v in views]
        js = json.dumps(j, separators=(",", ":")).encode("utf-8")
        js += b" " * ((4 - len(js) % 4) % 4)
        total = 12 + 8 + len(js) + 8 + len(blob)
        with open(path, "wb") as f:
            f.write(struct.pack("<III", 0x46546C67, 2, total))
            f.write(struct.pack("<II", len(js), 0x4E4F534A))
            f.write(js)
            f.write(struct.pack("<II", len(blob), 0x004E4942))
            f.write(bytes(blob))

    # -- helpers
    def mesh_node(self) -> Optional[int]:
        for i, n in enumerate(self.j.get("nodes", [])):
            if "mesh" in n:
                return i
        return None

    def part_names(self, mesh_index: int = 0) -> List[str]:
        parts = (self.j["meshes"][mesh_index].get("extras") or {}).get("dust3dParts") or []
        return [p.get("name", "") if isinstance(p, dict) else "" for p in parts]


# ---------------------------------------------------------------- quaternions

def _q_mul(a, b):
    ax, ay, az, aw = a
    bx, by, bz, bw = b
    return np.array([aw * bx + ax * bw + ay * bz - az * by,
                     aw * by - ax * bz + ay * bw + az * bx,
                     aw * bz + ax * by - ay * bx + az * bw,
                     aw * bw - ax * bx - ay * by - az * bz])


def _q_inv(q):
    return np.array([-q[0], -q[1], -q[2], q[3]]) / max(float(np.dot(q, q)), 1e-12)


def _q_axis(axis, deg):
    h = math.radians(deg) / 2
    s = math.sin(h)
    return np.array([axis[0] * s, axis[1] * s, axis[2] * s, math.cos(h)])


def _q_euler(deg3):
    """World-frame rotation: about X, then Y, then Z (degrees)."""
    q = _q_axis((1, 0, 0), deg3[0])
    q = _q_mul(_q_axis((0, 1, 0), deg3[1]), q)
    return _q_mul(_q_axis((0, 0, 1), deg3[2]), q)


def _q_rotate(q, v):
    p = np.array([v[0], v[1], v[2], 0.0])
    return _q_mul(_q_mul(q, p), _q_inv(q))[:3]


def _q_norm(q):
    q = np.asarray(q, np.float64)
    return q / max(float(np.linalg.norm(q)), 1e-12)


# ---------------------------------------------------------------- slots

def slot_of_parts(names: List[str]) -> List[str]:
    return [split_component_name(n)[1] for n in names]


def split_slots(doc: GlbDoc) -> Dict[str, Any]:
    """Split the skinned mesh into the base body and one mesh per slot variant."""
    ni = doc.mesh_node()
    if ni is None:
        return {}
    node = doc.j["nodes"][ni]
    mi = node["mesh"]
    mesh = doc.j["meshes"][mi]
    prim = mesh["primitives"][0]
    if "_PART" not in prim["attributes"] or len(mesh["primitives"]) != 1:
        return {}
    slots = slot_of_parts(doc.part_names(mi))
    if not any(slots):
        return {}
    part = doc.read_float(prim["attributes"]["_PART"])[:, 0].round().astype(np.int64)
    n_vert = len(part)
    tri = (doc.read(prim["indices"]).reshape(-1, 3).astype(np.int64) if "indices" in prim
           else np.arange(n_vert, dtype=np.int64).reshape(-1, 3))
    tri_slot = np.array([slots[k] if 0 <= k < len(slots) else "" for k in part[tri[:, 0]]], dtype=object)
    attrs = {k: (v, doc.j["accessors"][v]) for k, v in prim["attributes"].items()}
    arrays = {k: doc.read(v) for k, (v, _) in attrs.items()}

    def build(sel_tris):
        used, inv = np.unique(sel_tris.reshape(-1), return_inverse=True)
        new_attrs = {}
        for k, (_, acc) in attrs.items():
            new_attrs[k] = doc.add(arrays[k][used], target=34962, minmax=(k == "POSITION"),
                                   normalized=bool(acc.get("normalized")))
        idx = inv.astype(np.uint16 if len(used) < 65536 else np.uint32)
        p = {"attributes": new_attrs, "indices": doc.add(idx, target=34963)}
        if "material" in prim:
            p["material"] = prim["material"]
        return p

    info: Dict[str, Any] = {"base_triangles": int((tri_slot == "").sum()), "slots": {}}
    base = tri[tri_slot == ""]
    mesh["primitives"] = [build(base)] if len(base) else []
    node.setdefault("name", "body")
    parent = next((i for i, n in enumerate(doc.j["nodes"]) if ni in n.get("children", [])), None)
    for sv in sorted(set(s for s in tri_slot if s)):
        sel = tri[tri_slot == sv]
        slot, variant = sv.split("/", 1)
        doc.j["meshes"].append({"name": "slot_%s_%s" % (slot, variant), "primitives": [build(sel)],
                                "extras": mesh.get("extras", {})})
        new_node = {"name": "slot_%s_%s" % (slot, variant), "mesh": len(doc.j["meshes"]) - 1}
        if "skin" in node:
            new_node["skin"] = node["skin"]
        doc.j["nodes"].append(new_node)
        k = len(doc.j["nodes"]) - 1
        if parent is not None:
            doc.j["nodes"][parent].setdefault("children", []).append(k)
        else:
            doc.j["scenes"][doc.j.get("scene", 0)]["nodes"].append(k)
        info["slots"].setdefault(slot, {})[variant] = int(len(sel))
    if not mesh["primitives"]:
        # every triangle is in a slot: keep an empty-free file by dropping the base node's mesh
        del node["mesh"]
    info["max_triangles"] = info["base_triangles"] + sum(max(v.values()) for v in info["slots"].values())
    return info


# ---------------------------------------------------------------- weights

def smooth_weights(doc: GlbDoc, passes: int, rigid_parts: Optional[set] = None, amount: float = 0.5) -> Dict[str, Any]:
    ni = doc.mesh_node()
    if ni is None or passes <= 0:
        return {}
    prim = doc.j["meshes"][doc.j["nodes"][ni]["mesh"]]["primitives"][0]
    at = prim["attributes"]
    if "JOINTS_0" not in at or "WEIGHTS_0" not in at:
        return {}
    pos = doc.read_float(at["POSITION"]).astype(np.float64)
    joints = doc.read(at["JOINTS_0"]).astype(np.int64)
    weights = doc.read_float(at["WEIGHTS_0"]).astype(np.float64)
    n_joint = int(joints.max()) + 1
    V = len(pos)
    tri = (doc.read(prim["indices"]).reshape(-1, 3).astype(np.int64) if "indices" in prim
           else np.arange(V).reshape(-1, 3))
    # weld by position: Dust3D splits vertices at UV seams and hard edges
    diag = float(np.linalg.norm(pos.max(0) - pos.min(0))) or 1.0
    key = np.round(pos / (diag * 1e-5)).astype(np.int64)
    _, weld = np.unique(key, axis=0, return_inverse=True)
    weld = weld.reshape(-1)
    W = int(weld.max()) + 1
    dense = np.zeros((V, n_joint))
    for k in range(joints.shape[1]):
        np.add.at(dense, (np.arange(V), joints[:, k]), weights[:, k])
    wsum = np.zeros((W, n_joint))
    np.add.at(wsum, weld, dense)
    cnt = np.bincount(weld, minlength=W).astype(np.float64)
    w = wsum / cnt[:, None]
    fixed = np.zeros(W, bool)
    if rigid_parts and "_PART" in at:
        part = doc.read_float(at["_PART"])[:, 0].round().astype(np.int64)
        rig_v = np.isin(part, list(rigid_parts))
        fixed[weld[rig_v]] = True
    # welded edges
    e = np.concatenate([tri[:, [0, 1]], tri[:, [1, 2]], tri[:, [2, 0]]])
    e = weld[e]
    e = e[e[:, 0] != e[:, 1]]
    e = np.unique(np.sort(e, 1), axis=0)
    # never blend across a rigid part's border: a pauldron stays rigid, the arm next to it doesn't
    # pull its weights from the plate
    keep = ~(fixed[e[:, 0]] ^ fixed[e[:, 1]])
    e = e[keep]
    deg = np.bincount(e.reshape(-1), minlength=W).astype(np.float64)
    changed = 0
    for _ in range(passes):
        acc = np.zeros_like(w)
        np.add.at(acc, e[:, 0], w[e[:, 1]])
        np.add.at(acc, e[:, 1], w[e[:, 0]])
        has = deg > 0
        avg = np.where(has[:, None], acc / np.maximum(deg, 1)[:, None], w)
        new = w * (1 - amount) + avg * amount
        new[fixed] = w[fixed]
        changed = int((np.abs(new - w).sum(1) > 1e-4).sum())
        w = new
    # back to at most 4 bones per vertex
    top = np.argsort(-w, 1)[:, :4]
    tw = np.take_along_axis(w, top, 1)
    tw[tw < 0.02] = 0.0
    tw /= np.maximum(tw.sum(1, keepdims=True), 1e-9)
    out_j = top[weld].astype(np.uint16)
    out_w = tw[weld].astype(np.float32)
    out_j[out_w == 0] = 0
    at["JOINTS_0"] = doc.add(out_j, target=34962)
    at["WEIGHTS_0"] = doc.add(out_w, target=34962)
    blended = int((np.count_nonzero(tw > 0, 1) > 2).sum())
    return {"passes": passes, "vertices_changed": changed, "vertices_with_3plus_bones": blended}


# ---------------------------------------------------------------- posed clips

def _rest(doc: GlbDoc):
    nodes = doc.j["nodes"]
    parent = {}
    for i, n in enumerate(nodes):
        for c in n.get("children", []):
            parent[c] = i
    rot = {i: _q_norm(n.get("rotation", [0, 0, 0, 1])) for i, n in enumerate(nodes)}
    tra = {i: np.array(n.get("translation", [0, 0, 0]), np.float64) for i, n in enumerate(nodes)}
    wrot = {}

    def world_rot(i):
        if i in wrot:
            return wrot[i]
        q = rot[i]
        if i in parent:
            q = _q_mul(world_rot(parent[i]), q)
        wrot[i] = _q_norm(q)
        return wrot[i]
    for i in range(len(nodes)):
        world_rot(i)
    return parent, rot, tra, wrot


def _sample_channel(times, vals, t, path):
    if t <= times[0]:
        return vals[0].astype(np.float64)
    if t >= times[-1]:
        return vals[-1].astype(np.float64)
    i = int(np.searchsorted(times, t)) - 1
    f = (t - times[i]) / max(times[i + 1] - times[i], 1e-9)
    a, b = vals[i].astype(np.float64), vals[i + 1].astype(np.float64)
    if path == "rotation":
        if np.dot(a, b) < 0:
            b = -b
        return _q_norm(a * (1 - f) + b * f)
    return a * (1 - f) + b * f


def add_pose_clips(doc: GlbDoc, spec) -> List[str]:
    poses = [a for a in spec.animations if a.type == "Pose"]
    if not poses or not doc.j.get("skins"):
        return []
    parent, rot, tra, wrot = _rest(doc)
    joints = doc.j["skins"][0]["joints"]
    by_name = {doc.j["nodes"][i].get("name"): i for i in joints}
    clips = {a.get("name"): a for a in doc.j.get("animations", [])}
    added = []
    for a in poses:
        P = a.pose
        dur = float(P["durationSeconds"])
        frames = max(2, int(round(dur * 30)))
        times = np.linspace(0.0, dur, frames + 1 if P.get("loop") else frames).astype(np.float32)
        keys = sorted(P["keys"], key=lambda k: float(k["t"]))
        base = clips.get(P.get("base") or "")
        base_ch = {}
        base_dur = 0.0
        if base:
            for ch in base["channels"]:
                s = base["samplers"][ch["sampler"]]
                ti = doc.read_float(s["input"])[:, 0]
                base_dur = max(base_dur, float(ti.max()) if len(ti) else 0.0)
                base_ch[(ch["target"]["node"], ch["target"]["path"])] = (ti, doc.read_float(s["output"]))

        def key_value(bone, t):
            """(euler degrees, move) for a bone at time t, eased between the keys."""
            def val(k):
                v = (k.get("pose") or {}).get(bone)
                if v is None:
                    return np.zeros(3), np.zeros(3)
                if isinstance(v, dict):
                    return np.array(v.get("rotate", [0, 0, 0]), np.float64), np.array(v.get("move", [0, 0, 0]), np.float64)
                return np.array(v, np.float64), np.zeros(3)
            if t <= float(keys[0]["t"]):
                return val(keys[0])
            for k0, k1 in zip(keys, keys[1:]):
                t0, t1 = float(k0["t"]), float(k1["t"])
                if t <= t1:
                    f = (t - t0) / max(t1 - t0, 1e-9)
                    f = f * f * (3 - 2 * f)  # ease in and out
                    (r0, m0), (r1, m1) = val(k0), val(k1)
                    return r0 + (r1 - r0) * f, m0 + (m1 - m0) * f
            return val(keys[-1])

        samplers, channels = [], []
        tin = doc.add(times.reshape(-1, 1), minmax=True)
        for bi in joints:
            bname = doc.j["nodes"][bi].get("name")
            rots, moves = [], []
            for t in times:
                bt = (t % base_dur) if base_dur > 0 else 0.0
                r_l = _sample_channel(*base_ch[(bi, "rotation")], bt, "rotation") if (bi, "rotation") in base_ch else rot[bi]
                t_l = _sample_channel(*base_ch[(bi, "translation")], bt, "translation") if (bi, "translation") in base_ch else tra[bi]
                eul, mv = key_value(bname, float(t))
                if np.any(eul):
                    pw = wrot[parent[bi]] if bi in parent else np.array([0, 0, 0, 1.0])
                    # world-axis rotation (as seen in the rest pose) expressed in the parent's frame
                    local = _q_mul(_q_mul(_q_inv(pw), _q_euler(eul)), pw)
                    r_l = _q_norm(_q_mul(local, r_l))
                if np.any(mv):
                    pw = wrot[parent[bi]] if bi in parent else np.array([0, 0, 0, 1.0])
                    t_l = t_l + _q_rotate(_q_inv(pw), mv)
                rots.append(r_l)
                moves.append(t_l)
            for path, vals in (("rotation", rots), ("translation", moves)):
                out = doc.add(np.array(vals, np.float32))
                samplers.append({"input": tin, "output": out, "interpolation": "LINEAR"})
                channels.append({"sampler": len(samplers) - 1, "target": {"node": bi, "path": path}})
        doc.j.setdefault("animations", []).append({"name": a.name, "samplers": samplers, "channels": channels})
        added.append(a.name)
    return added


# ---------------------------------------------------------------- events

def clip_events(glb_path: str, spec, clips: List[Dict[str, Any]]) -> None:
    """Fill clips[i]["events"] ([{"name", "time", "bone"?}]) from the spec and the motion."""
    from . import glb as glbmod
    g = glbmod.load(glb_path)
    if g.skin is None:
        return
    all_joints = g.skin["joints"]
    # only bones that move some of the mesh (a rig template has bones a model leaves unused)
    used = set()
    for p in g.primitives:
        if p.joints is not None and p.weights is not None:
            used.update(int(all_joints[k]) for k in np.unique(p.joints[p.weights > 0.05]))
    joints = [j for j in all_joints if j in used] or list(all_joints)
    names = {j: g.nodes[j].get("name", "") for j in joints}
    children = {j for j in joints for c in g.nodes[j].get("children", []) if c in joints}
    leaves = [j for j in joints if j not in children]
    rest = glbmod.world_matrices(g)
    heights = {j: rest[j][1, 3] for j in joints}
    lo, hi = min(heights.values()), max(heights.values())
    feet = [j for j in leaves if heights[j] <= lo + 0.2 * (hi - lo)] or \
        [j for j in joints if "Foot" in names[j] or "Toe" in names[j]]
    anims = {a["name"]: a for a in g.animations}
    native = {a.get("name"): a.get("extras", {}).get("dust3dClip", {})
              for a in g.json.get("animations", [])}
    by_name = {a.name: a for a in spec.animations}
    for c in clips:
        a = anims.get(c["name"])
        sa = by_name.get(c["name"])
        if a is None or sa is None:
            continue
        c.update(native.get(c["name"], {}))
        dur = float(c.get("durationSeconds") or a["duration"] or 1.0)
        overrides = set((sa.events or {}).keys())
        events = [dict(e) for e in c.get("events", []) if e["name"] not in overrides]
        for ev, at in (sa.events or {}).items():
            for v in (at if isinstance(at, list) else [at]):
                events.append({"name": ev, "time": round(float(v) * dur, 3)})
        n = 48
        ts = np.linspace(0, a["duration"], n)
        need_hit = (any(w in sa.type for w in ATTACK_WORDS)
                    and sa.type not in {"BipedCastStart", "BipedCastRecover"}
                    and not any(e["name"] in {"hit", "release", "vocal"} for e in events))
        need_step = any(w in sa.type for w in STEP_WORDS) and not any(e["name"] == "step" for e in events)
        if need_hit or need_step:
            track = np.array([[m[j][:3, 3] for j in joints] for m in (glbmod.world_matrices(g, a, float(t)) for t in ts)])
        if need_hit:
            vel = np.linalg.norm(np.diff(track, axis=0), axis=2) / max(ts[1] - ts[0], 1e-6)  # (n-1) x J
            li = [joints.index(j) for j in leaves]
            best = np.unravel_index(np.argmax(vel[:, li]), (vel.shape[0], len(li)))
            events.append({"name": "hit", "time": round(float(ts[best[0] + 1]), 3), "bone": names[leaves[best[1]]]})
        if need_step and feet:
            for j in feet:
                h = track[:, joints.index(j), 1]
                span = h.max() - h.min()
                if span < 1e-4:
                    continue
                for i in range(n):
                    prev, nxt = h[(i - 1) % n], h[(i + 1) % n]
                    if h[i] <= prev and h[i] < nxt and h[i] <= h.min() + 0.25 * span:
                        events.append({"name": "step", "time": round(float(ts[i]), 3), "bone": names[j]})
        events.sort(key=lambda e: e["time"])
        if events:
            c["events"] = events


# ---------------------------------------------------------------- the whole finish

def finish(glb_path: str, spec, clips: List[Dict[str, Any]], ds3_parts: Optional[Dict[str, Any]] = None) -> Dict[str, Any]:
    """Post-process an exported .glb in place. Returns a report section."""
    report: Dict[str, Any] = {}
    doc = GlbDoc(glb_path)
    changed = False
    if spec.smoothWeights > 0:
        rigid = set()
        names = doc.part_names(0)
        rigid_names = {p.name for p in spec.parts if p.hard or p.metallic >= 0.5}
        for k, n in enumerate(names):
            if split_component_name(n)[0] in rigid_names:
                rigid.add(k)
        report["weights"] = smooth_weights(doc, spec.smoothWeights, rigid)
        changed = changed or bool(report["weights"])
    added = add_pose_clips(doc, spec)
    if added:
        report["pose_clips"] = added
        changed = True
    if changed:
        doc.save(glb_path)
    if clips:
        clip_events(glb_path, spec, clips)
    doc = GlbDoc(glb_path)
    slots = split_slots(doc)
    if slots:
        doc.save(glb_path)
        report["slots"] = slots
    tri_total = report.get("slots", {}).get("max_triangles")
    if spec.budget:
        if tri_total is None:
            from . import metrics
            tri_total = metrics.analyze(glb_path).get("triangles", 0)
        report["budget"] = {"limit": spec.budget, "heaviest_outfit": tri_total, "ok": tri_total <= spec.budget}
    return report
