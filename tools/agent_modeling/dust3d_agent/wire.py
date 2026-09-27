"""Edge-flow inspection: render Dust3D's quad/triangle topology (from an OBJ export).

Dust3D's OBJ export keeps the quads it recovered after combining parts, i.e. exactly the
wireframe the editor shows. Quads are drawn grey, triangles orange, so a well bridged
seam reads as a clean ring of quads and a poor one as a band of orange triangles.
"""

from __future__ import annotations

from typing import List, Optional, Sequence, Tuple

import numpy as np
from PIL import Image, ImageDraw

from .render import _view_axes


def load_obj(path: str) -> Tuple[np.ndarray, List[List[int]]]:
    verts, faces = [], []
    with open(path, encoding="utf-8", errors="replace") as f:
        for line in f:
            if line.startswith("v "):
                verts.append([float(v) for v in line.split()[1:4]])
            elif line.startswith("f "):
                faces.append([int(t.split("/")[0]) - 1 for t in line.split()[1:]])
    return np.array(verts, np.float64), faces


def render_wire(verts: np.ndarray, faces: List[List[int]], view: str = "three_quarter", size: int = 480,
                center: Optional[Sequence[float]] = None, extent: Optional[float] = None,
                highlight: Optional[set] = None, label: str = "") -> Image.Image:
    """Painter's-algorithm render of faces with outlines. highlight: face indices drawn red."""
    right, up, fwd = _view_axes(view)
    if center is None:
        lo, hi = verts.min(0), verts.max(0)
        center = (lo + hi) / 2
        extent = max(np.ptp(verts @ right), np.ptp(verts @ up)) * 1.05
    center = np.asarray(center, np.float64)
    scale = size / extent
    sx = (verts @ right - center @ right) * scale + size / 2
    sy = size / 2 - (verts @ up - center @ up) * scale
    depth = verts @ fwd
    light = np.array([0.35, 0.8, 0.5])
    light /= np.linalg.norm(light)
    order = sorted(range(len(faces)), key=lambda i: -np.mean(depth[faces[i]]))
    im = Image.new("RGB", (size, size), (242, 240, 236))
    dr = ImageDraw.Draw(im)
    for i in order:
        f = faces[i]
        p = verts[f]
        n = np.cross(p[1] - p[0], p[2] - p[0])
        nn = np.linalg.norm(n)
        if nn < 1e-15:
            continue
        n /= nn
        if n @ fwd > 0:  # back face
            continue
        shade = 0.45 + 0.5 * max(0.0, float(n @ light))
        if highlight and i in highlight:
            base = (220, 60, 60)
        elif len(f) == 3:
            base = (235, 150, 70)
        else:
            base = (200, 200, 200)
        col = tuple(int(c * shade) for c in base)
        pts = [(float(sx[k]), float(sy[k])) for k in f]
        dr.polygon(pts, fill=col, outline=(40, 40, 40))
    if label:
        dr.rectangle([0, 0, 8 + 6 * len(label), 16], fill=(30, 30, 30))
        dr.text((4, 2), label, fill=(255, 255, 255))
    return im


def seam_closeup(obj_path: str, center: Sequence[float], extent: float, out_path: str,
                 views: Sequence[str] = ("three_quarter", "left", "front", "top"), size: int = 360,
                 label: str = "") -> str:
    verts, faces = load_obj(obj_path)
    tiles = [render_wire(verts, faces, v, size, center, extent, label=("%s %s" % (label, v)).strip()) for v in views]
    sheet = Image.new("RGB", (size * len(tiles), size), (255, 255, 255))
    for i, t in enumerate(tiles):
        sheet.paste(t, (i * size, 0))
    sheet.save(out_path)
    return out_path
