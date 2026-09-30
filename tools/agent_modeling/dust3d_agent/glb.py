"""Minimal glTF 2.0 binary (.glb) reader for Dust3D exports: meshes, skin, animations."""

from __future__ import annotations

import io
import json
import struct
from dataclasses import dataclass, field
from typing import Dict, List, Optional

import numpy as np

_CT = {5120: np.int8, 5121: np.uint8, 5122: np.int16, 5123: np.uint16, 5125: np.uint32, 5126: np.float32}
_NC = {"SCALAR": 1, "VEC2": 2, "VEC3": 3, "VEC4": 4, "MAT4": 16}


@dataclass
class Primitive:
    positions: np.ndarray
    indices: np.ndarray
    normals: Optional[np.ndarray] = None
    uvs: Optional[np.ndarray] = None
    joints: Optional[np.ndarray] = None
    weights: Optional[np.ndarray] = None
    base_color: np.ndarray = field(default_factory=lambda: np.ones(4, np.float32))
    texture: Optional[np.ndarray] = None  # HxWx4 float 0..1
    mesh_name: str = ""  # "slot_<slot>_<variant>" for equipment meshes split out by gamekit


@dataclass
class Glb:
    json: dict
    primitives: List[Primitive]
    nodes: List[dict]
    skin: Optional[dict]
    inverse_binds: Optional[np.ndarray]
    animations: List[dict]
    mesh_node: Optional[int]

    def accessor(self, i):  # noqa: D401 - populated in load()
        raise NotImplementedError


def load(path: str) -> Glb:
    with open(path, "rb") as f:
        data = f.read()
    magic, version, length = struct.unpack_from("<III", data, 0)
    if magic != 0x46546C67:
        raise ValueError("not a GLB file")
    off = 12
    js, binary = None, b""
    while off < length:
        clen, ctype = struct.unpack_from("<II", data, off)
        chunk = data[off + 8: off + 8 + clen]
        if ctype == 0x4E4F534A:
            js = json.loads(chunk.decode("utf-8"))
        elif ctype == 0x004E4942:
            binary = chunk
        off += 8 + clen

    views = js.get("bufferViews", [])

    def view_bytes(vi):
        v = views[vi]
        o = v.get("byteOffset", 0)
        return binary[o:o + v["byteLength"]], v.get("byteStride")

    def accessor(ai):
        a = js["accessors"][ai]
        raw, stride = view_bytes(a["bufferView"])
        dt = np.dtype(_CT[a["componentType"]])
        n = _NC[a["type"]]
        o = a.get("byteOffset", 0)
        cnt = a["count"]
        if stride and stride != dt.itemsize * n:
            arr = np.ndarray((cnt, n), dt, raw, o, (stride, dt.itemsize))
        else:
            arr = np.frombuffer(raw, dt, cnt * n, o).reshape(cnt, n)
        arr = arr.astype(np.float32) if a["componentType"] == 5126 else arr.copy()
        if a.get("normalized") and a["componentType"] != 5126:
            arr = arr.astype(np.float32) / np.iinfo(dt).max
        return arr

    images = []
    for img in js.get("images", []):
        try:
            from PIL import Image
            raw, _ = view_bytes(img["bufferView"])
            im = Image.open(io.BytesIO(raw)).convert("RGBA")
            images.append(np.asarray(im, np.float32) / 255.0)
        except Exception:
            images.append(None)

    prims: List[Primitive] = []
    mesh_node = None
    for ni, n in enumerate(js.get("nodes", [])):
        if "mesh" in n:
            mesh_node = ni
    for mesh in js.get("meshes", []):
        for p in mesh["primitives"]:
            at = p["attributes"]
            pos = accessor(at["POSITION"])
            idx = accessor(p["indices"]).reshape(-1).astype(np.int64) if "indices" in p else np.arange(len(pos))
            prim = Primitive(positions=pos, indices=idx.reshape(-1, 3), mesh_name=mesh.get("name", ""))
            if "NORMAL" in at:
                prim.normals = accessor(at["NORMAL"])
            if "TEXCOORD_0" in at:
                prim.uvs = accessor(at["TEXCOORD_0"])
            if "JOINTS_0" in at:
                prim.joints = accessor(at["JOINTS_0"]).astype(np.int64)
            if "WEIGHTS_0" in at:
                prim.weights = accessor(at["WEIGHTS_0"]).astype(np.float32)
            if "material" in p:
                m = js["materials"][p["material"]]
                pbr = m.get("pbrMetallicRoughness", {})
                prim.base_color = np.array(pbr.get("baseColorFactor", [1, 1, 1, 1]), np.float32)
                tex = pbr.get("baseColorTexture")
                if tex is not None:
                    src = js["textures"][tex["index"]]["source"]
                    prim.texture = images[src] if src < len(images) else None
            prims.append(prim)

    skin = js["skins"][0] if js.get("skins") else None
    ibm = accessor(skin["inverseBindMatrices"]).reshape(-1, 4, 4).transpose(0, 2, 1) if skin and "inverseBindMatrices" in skin else None

    anims = []
    for a in js.get("animations", []):
        chans = []
        dur = 0.0
        for ch in a["channels"]:
            s = a["samplers"][ch["sampler"]]
            t = accessor(s["input"]).reshape(-1)
            v = accessor(s["output"])
            dur = max(dur, float(t.max()) if len(t) else 0.0)
            chans.append({"node": ch["target"].get("node"), "path": ch["target"]["path"], "times": t, "values": v})
        anims.append({"name": a.get("name", ""), "channels": chans, "duration": dur})

    g = Glb(json=js, primitives=prims, nodes=js.get("nodes", []), skin=skin,
            inverse_binds=ibm, animations=anims, mesh_node=mesh_node)
    g.accessor = accessor  # type: ignore
    return g


def slot_variant(prim: Primitive):
    """(slot, variant) of an equipment mesh, or None for the body."""
    n = prim.mesh_name
    if not n.startswith("slot_"):
        return None
    slot, _, variant = n[5:].partition("_")
    return slot, variant


def outfits(g: Glb) -> List[Dict[str, str]]:
    """Outfits to preview: the k-th variant of every slot together (k = 0, 1, ...)."""
    slots: Dict[str, List[str]] = {}
    for p in g.primitives:
        sv = slot_variant(p)
        if sv and sv[1] not in slots.setdefault(sv[0], []):
            slots[sv[0]].append(sv[1])
    if not slots:
        return []
    for v in slots.values():
        v.sort(key=lambda x: (not x.isdigit(), int(x) if x.isdigit() else 0, x))
    return [{s: v[min(k, len(v) - 1)] for s, v in slots.items()} for k in range(max(len(v) for v in slots.values()))]


def dressed(g: Glb, outfit: Optional[Dict[str, str]]) -> Glb:
    """A view of the model wearing one outfit (the body plus one variant per slot)."""
    if outfit is None:
        return g
    keep = [p for p in g.primitives if slot_variant(p) is None or outfit.get(slot_variant(p)[0]) == slot_variant(p)[1]]
    h = Glb(json=g.json, primitives=keep, nodes=g.nodes, skin=g.skin, inverse_binds=g.inverse_binds,
            animations=g.animations, mesh_node=g.mesh_node)
    h.accessor = g.accessor  # type: ignore
    return h


# ---------------------------------------------------------------- transforms

def _quat_to_mat(q):
    x, y, z, w = q
    return np.array([
        [1 - 2 * (y * y + z * z), 2 * (x * y - z * w), 2 * (x * z + y * w)],
        [2 * (x * y + z * w), 1 - 2 * (x * x + z * z), 2 * (y * z - x * w)],
        [2 * (x * z - y * w), 2 * (y * z + x * w), 1 - 2 * (x * x + y * y)]], np.float64)


def _trs(t, r, s):
    m = np.eye(4)
    m[:3, :3] = _quat_to_mat(r) * np.asarray(s)[None, :]
    m[:3, 3] = t
    return m


def _node_local(n):
    if "matrix" in n:
        return np.array(n["matrix"], np.float64).reshape(4, 4).T
    return _trs(n.get("translation", [0, 0, 0]), n.get("rotation", [0, 0, 0, 1]), n.get("scale", [1, 1, 1]))


def _sample(ch, t):
    times, vals = ch["times"], ch["values"]
    if t <= times[0]:
        return vals[0]
    if t >= times[-1]:
        return vals[-1]
    i = int(np.searchsorted(times, t)) - 1
    f = (t - times[i]) / max(times[i + 1] - times[i], 1e-9)
    a, b = vals[i].astype(np.float64), vals[i + 1].astype(np.float64)
    if ch["path"] == "rotation":
        if np.dot(a, b) < 0:
            b = -b
        q = a * (1 - f) + b * f
        return q / np.linalg.norm(q)
    return a * (1 - f) + b * f


def world_matrices(g: Glb, anim: Optional[dict] = None, t: float = 0.0) -> List[np.ndarray]:
    nodes = g.nodes
    over: Dict[int, Dict[str, np.ndarray]] = {}
    if anim:
        for ch in anim["channels"]:
            if ch["node"] is None:
                continue
            over.setdefault(ch["node"], {})[ch["path"]] = _sample(ch, t)
    parent = {}
    for i, n in enumerate(nodes):
        for c in n.get("children", []):
            parent[c] = i
    cache: Dict[int, np.ndarray] = {}

    def local(i):
        n = nodes[i]
        o = over.get(i)
        if not o:
            return _node_local(n)
        return _trs(o.get("translation", n.get("translation", [0, 0, 0])),
                    o.get("rotation", n.get("rotation", [0, 0, 0, 1])),
                    o.get("scale", n.get("scale", [1, 1, 1])))

    def world(i):
        if i in cache:
            return cache[i]
        m = local(i)
        if i in parent:
            m = world(parent[i]) @ m
        cache[i] = m
        return m

    return [world(i) for i in range(len(nodes))]


def posed_positions(g: Glb, prim: Primitive, anim: Optional[dict] = None, t: float = 0.0) -> np.ndarray:
    """Linear-blend skinned positions for a primitive (rest pose if anim is None)."""
    if g.skin is None or prim.joints is None or prim.weights is None or g.inverse_binds is None:
        return prim.positions.astype(np.float64)
    W = world_matrices(g, anim, t)
    joints = g.skin["joints"]
    jm = np.stack([W[j] @ g.inverse_binds[k] for k, j in enumerate(joints)])  # J x 4 x 4
    p = np.concatenate([prim.positions.astype(np.float64), np.ones((len(prim.positions), 1))], 1)
    out = np.zeros((len(p), 3))
    w = prim.weights / np.maximum(prim.weights.sum(1, keepdims=True), 1e-9)
    for k in range(prim.joints.shape[1]):
        m = jm[prim.joints[:, k]]  # N x 4 x 4
        out += w[:, k:k + 1] * np.einsum("nij,nj->ni", m, p)[:, :3]
    return out
