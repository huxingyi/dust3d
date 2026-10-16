"""Make Dust3D's built-in head mesh from the ICT FaceKit neutral face.

    python make_head.py <ICT-FaceKit>/FaceXModel/generic_neutral_mesh.obj human_head.glb

Takes the face and the head-and-neck regions of the ICT FaceKit generic neutral mesh
(https://github.com/USC-ICT/ICT-FaceKit, MIT License, (c) 2020 USC Institute for Creative
Technologies), closes the eye openings with the eyeballs' front, the lips and the cut
across the neck, and writes a watertight triangle mesh as a GLB: Y up, facing +Z,
centimetres; and <name>_landmarks.json beside it (the 68 Multi-PIE face landmarks and the
eyeballs, in the same frame).
"""
import json
import struct
import sys

import numpy as np

FACE_AND_HEAD_POLYGONS = 11144  # ICT polygon ranges #0 face and #1 head and neck
SCLERA_LEFT = (21451, 22221)
SCLERA_RIGHT = (23021, 23791)
# Multi-PIE 68 point landmarks on the ICT face (from the ICT FaceKit README); written beside
# the mesh so a face can be painted where its features are
LANDMARKS = [1225, 1888, 1052, 367, 1719, 1722, 2199, 1447, 966, 3661, 4390, 3927, 3924, 2608, 3272,
             4088, 3443, 268, 493, 1914, 2044, 1401, 3615, 4240, 4114, 2734, 2509, 978, 4527, 4942,
             4857, 1140, 2075, 1147, 4269, 3360, 1507, 1542, 1537, 1528, 1518, 1511, 3742, 3751, 3756,
             3721, 3725, 3732, 5708, 5695, 2081, 0, 4275, 6200, 6213, 6346, 6461, 5518, 5957, 5841,
             5702, 5711, 5533, 6216, 6207, 6470, 5517, 5966]
EYE_RECESS = 0.12  # the eyeball sits this far (cm) under the lids' rim: a crease at the lid


def load_obj(path):
    vertices, faces = [], []
    for line in open(path):
        if line.startswith("v "):
            vertices.append([float(x) for x in line.split()[1:4]])
        elif line.startswith("f "):
            faces.append([int(t.split("/")[0]) - 1 for t in line.split()[1:]])
    return np.array(vertices), faces


def boundary_loops(faces):
    half = {}
    for f in faces:
        for i in range(len(f)):
            half[(f[i], f[(i + 1) % len(f)])] = True
    nxt = {}
    for a, b in half:
        if (b, a) not in half:
            nxt[b] = a  # walk the hole so its fill faces outwards
    loops, seen = [], set()
    for s in list(nxt):
        if s in seen:
            continue
        loop, cur = [], s
        while cur not in seen:
            seen.add(cur)
            loop.append(cur)
            cur = nxt[cur]
        loops.append(loop)
    return loops


def fill(vertices, faces, loop, place, rings=4):
    """Close a hole with concentric rings shrinking to its centre; place(point, t) moves
    each new point (t = 0 at the rim, 1 at the centre)."""
    rim = np.array([vertices[i] for i in loop])
    centre = rim.mean(0)
    previous = list(loop)
    for k in range(1, rings):
        t = k / rings
        ring = []
        for p in rim:
            vertices.append(place(p + (centre - p) * t, t))
            ring.append(len(vertices) - 1)
        n = len(loop)
        for i in range(n):
            j = (i + 1) % n
            faces.append([previous[i], previous[j], ring[j], ring[i]])
        previous = ring
    vertices.append(place(centre, 1.0))
    c = len(vertices) - 1
    for i in range(len(previous)):
        faces.append([previous[i], previous[(i + 1) % len(previous)], c])


def sphere(points):
    centre = points.mean(0)
    return centre, np.linalg.norm(points - centre, axis=1).mean()


def write_glb(path, positions, triangles):
    positions = positions.astype(np.float32)
    indices = np.array(triangles, dtype=np.uint32).ravel()
    pos_bytes, idx_bytes = positions.tobytes(), indices.tobytes()
    gltf = {
        "asset": {"version": "2.0", "generator": "dust3d make_head.py"},
        "scene": 0, "scenes": [{"nodes": [0]}],
        "nodes": [{"mesh": 0, "name": "head"}],
        "meshes": [{"primitives": [{"attributes": {"POSITION": 0}, "indices": 1}]}],
        "buffers": [{"byteLength": len(pos_bytes) + len(idx_bytes)}],
        "bufferViews": [
            {"buffer": 0, "byteOffset": 0, "byteLength": len(pos_bytes), "target": 34962},
            {"buffer": 0, "byteOffset": len(pos_bytes), "byteLength": len(idx_bytes), "target": 34963}],
        "accessors": [
            {"bufferView": 0, "componentType": 5126, "count": len(positions), "type": "VEC3",
             "min": positions.min(0).tolist(), "max": positions.max(0).tolist()},
            {"bufferView": 1, "componentType": 5125, "count": len(indices), "type": "SCALAR"}],
    }
    js = json.dumps(gltf, separators=(",", ":")).encode()
    js += b" " * (-len(js) % 4)
    binary = pos_bytes + idx_bytes
    binary += b"\0" * (-len(binary) % 4)
    with open(path, "wb") as f:
        f.write(struct.pack("<III", 0x46546C67, 2, 12 + 8 + len(js) + 8 + len(binary)))
        f.write(struct.pack("<II", len(js), 0x4E4F534A) + js)
        f.write(struct.pack("<II", len(binary), 0x004E4942) + binary)


def main(src, dst):
    all_vertices, all_faces = load_obj(src)
    faces = all_faces[:FACE_AND_HEAD_POLYGONS]
    used = sorted({i for f in faces for i in f})
    remap = {old: new for new, old in enumerate(used)}
    vertices = [all_vertices[i] for i in used]
    faces = [[remap[i] for i in f] for f in faces]
    eyes = [sphere(all_vertices[a:b]) for a, b in (SCLERA_LEFT, SCLERA_RIGHT)]
    for loop in boundary_loops(faces):
        rim = np.array([vertices[i] for i in loop])
        centre = rim.mean(0)
        if centre[1] < -10:  # the cut across the neck: flat
            fill(vertices, faces, loop, lambda p, t: p)
        elif centre[1] > 0:  # an eye: the front of its eyeball
            c, r = min(eyes, key=lambda e: np.linalg.norm(e[0] - centre))
            fill(vertices, faces, loop, lambda p, t, c=c, r=r: c + (p - c) / np.linalg.norm(p - c) * (r - EYE_RECESS * min(1.0, 3 * t)), rings=6)
        else:  # the lips: closed, pressed slightly in
            fill(vertices, faces, loop, lambda p, t: p - np.array([0, 0, 0.12 * np.sin(t * np.pi / 2)]))
    triangles = []
    for f in faces:
        for i in range(1, len(f) - 1):
            triangles.append([f[0], f[i], f[i + 1]])
    assert not boundary_loops(triangles), "head mesh is not closed"
    write_glb(dst, np.array(vertices), triangles)
    positions = np.array(vertices)
    marks = {"bounds": [[round(float(x), 4) for x in positions.min(0)], [round(float(x), 4) for x in positions.max(0)]],
             "landmarks68": [[round(float(x), 4) for x in all_vertices[i]] for i in LANDMARKS],
             "eyeballs": [{"centre": [round(float(x), 4) for x in c], "radius": round(float(r), 4)} for c, r in eyes]}
    with open(dst.rsplit(".", 1)[0] + "_landmarks.json", "w") as f:
        json.dump(marks, f)
    print(len(vertices), "vertices", len(triangles), "triangles")


if __name__ == "__main__":
    main(*sys.argv[1:3])
