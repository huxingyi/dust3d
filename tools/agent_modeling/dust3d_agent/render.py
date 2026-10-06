"""Headless software renderer (numpy + Pillow) for Dust3D GLB exports.

Produces orthographic turnaround sheets and animation strips an agent can look
at to judge proportions, silhouettes and rig/animation behaviour. No GPU needed.
"""

from __future__ import annotations

from typing import List, Optional, Sequence, Tuple

import numpy as np
from PIL import Image, ImageDraw

from . import glb as glbmod

BG = np.array([0.95, 0.94, 0.92])
LIGHT = np.array([0.35, 0.8, 0.5])
LIGHT = LIGHT / np.linalg.norm(LIGHT)

# name -> (screen-right, screen-up) world axes; view direction = up x right
VIEWS = {
    "front": (np.array([1.0, 0, 0]), np.array([0, 1.0, 0])),    # camera at +Z looking at the face
    "left": (np.array([0, 0, -1.0]), np.array([0, 1.0, 0])),    # camera at +X (creature's left side)
    "back": (np.array([-1.0, 0, 0]), np.array([0, 1.0, 0])),    # camera at -Z
    "top": (np.array([-1.0, 0, 0]), np.array([0, 0, 1.0])),     # camera above, head up on screen
}


def _view_axes(name: str) -> Tuple[np.ndarray, np.ndarray, np.ndarray]:
    if name == "three_quarter":
        fwd = np.array([-0.62, -0.35, -0.70])  # camera looks from front-left-above
        fwd /= np.linalg.norm(fwd)
        up0 = np.array([0, 1.0, 0])
        right = np.cross(fwd, up0)
        right /= np.linalg.norm(right)
        up = np.cross(right, fwd)
        return right, up, fwd
    right, up = VIEWS[name]
    fwd = np.cross(up, right)
    return right, up, fwd


def _gather(g: glbmod.Glb, anim=None, t=0.0):
    """Return per-primitive (positions, normals) in world space, optionally posed."""
    out = []
    jm = None
    if anim is not None and g.skin is not None and g.inverse_binds is not None:
        W = glbmod.world_matrices(g, anim, t)
        jm = np.stack([W[j] @ g.inverse_binds[k] for k, j in enumerate(g.skin["joints"])])
    for p in g.primitives:
        pos = p.positions.astype(np.float64)
        nrm = p.normals.astype(np.float64) if p.normals is not None else None
        if jm is not None and p.joints is not None and p.weights is not None:
            w = p.weights / np.maximum(p.weights.sum(1, keepdims=True), 1e-9)
            ph = np.concatenate([pos, np.ones((len(pos), 1))], 1)
            npos = np.zeros_like(pos)
            nn = np.zeros_like(pos) if nrm is not None else None
            for k in range(p.joints.shape[1]):
                m = jm[p.joints[:, k]]
                npos += w[:, k:k + 1] * np.einsum("nij,nj->ni", m, ph)[:, :3]
                if nn is not None:
                    nn += w[:, k:k + 1] * np.einsum("nij,nj->ni", m[:, :3, :3], nrm)
            pos = npos
            nrm = nn
        if nrm is not None:
            nrm = nrm / np.maximum(np.linalg.norm(nrm, axis=1, keepdims=True), 1e-9)
        out.append((pos, nrm))
    return out


def bounds(g: glbmod.Glb, anims: Sequence = (None,), samples: int = 1):
    lo, hi = np.full(3, np.inf), np.full(3, -np.inf)
    for a in anims:
        ts = [0.0] if a is None else np.linspace(0, a["duration"], samples)
        for t in ts:
            for pos, _ in _gather(g, a, t):
                lo = np.minimum(lo, pos.min(0))
                hi = np.maximum(hi, pos.max(0))
    return lo, hi


def render_view(g: glbmod.Glb, view: str, size: int = 384, anim=None, t: float = 0.0,
                frame_bounds=None, draw_skeleton: bool = False, draw_ground: bool = True) -> Image.Image:
    right, up, fwd = _view_axes(view)
    geo = _gather(g, anim, t)
    lo, hi = frame_bounds if frame_bounds is not None else bounds(g)
    center = (lo + hi) / 2
    corners = np.array([[x, y, z] for x in (lo[0], hi[0]) for y in (lo[1], hi[1]) for z in (lo[2], hi[2])])
    extent = max(np.ptp(corners @ right), np.ptp(corners @ up), 1e-6)
    scale = size * 0.86 / extent
    cx, cy = center @ right, center @ up

    def project(P):
        sx = (P @ right - cx) * scale + size / 2
        sy = size / 2 - (P @ up - cy) * scale
        d = P @ fwd  # larger = farther
        return sx, sy, d

    img = np.tile(BG, (size, size, 1)).astype(np.float64)
    zbuf = np.full((size, size), np.inf)

    # ground plane line (y = 0 after Dust3D grounding) for side/front views
    if draw_ground and view in ("front", "left", "back"):
        gy = int(round(size / 2 - (0 - cy) * scale))
        if 0 <= gy < size:
            img[gy:, :] = BG * 0.93

    for (pos, nrm), prim in zip(geo, g.primitives):
        sx, sy, d = project(pos)
        tri = prim.indices
        tx, ty, td = sx[tri], sy[tri], d[tri]
        # back-face culling is unsafe for non-manifold input; rely on z-buffer only
        x0 = np.clip(np.floor(tx.min(1)).astype(int), 0, size - 1)
        x1 = np.clip(np.ceil(tx.max(1)).astype(int), 0, size - 1)
        y0 = np.clip(np.floor(ty.min(1)).astype(int), 0, size - 1)
        y1 = np.clip(np.ceil(ty.max(1)).astype(int), 0, size - 1)
        # face normals for fallback shading
        v = pos[tri]
        fn = np.cross(v[:, 1] - v[:, 0], v[:, 2] - v[:, 0])
        fn /= np.maximum(np.linalg.norm(fn, axis=1, keepdims=True), 1e-12)
        tex = prim.texture
        uv = prim.uvs
        base = prim.base_color[:3].astype(np.float64)
        for i in range(len(tri)):
            if x1[i] < x0[i] or y1[i] < y0[i]:
                continue
            ax, bx, cx_ = tx[i]
            ay, by, cy_ = ty[i]
            den = (by - cy_) * (ax - cx_) + (cx_ - bx) * (ay - cy_)
            if abs(den) < 1e-12:
                continue
            xs = np.arange(x0[i], x1[i] + 1) + 0.5
            ys = np.arange(y0[i], y1[i] + 1) + 0.5
            X, Y = np.meshgrid(xs, ys)
            w0 = ((by - cy_) * (X - cx_) + (cx_ - bx) * (Y - cy_)) / den
            w1 = ((cy_ - ay) * (X - cx_) + (ax - cx_) * (Y - cy_)) / den
            w2 = 1 - w0 - w1
            m = (w0 >= -1e-6) & (w1 >= -1e-6) & (w2 >= -1e-6)
            if not m.any():
                continue
            z = w0 * td[i, 0] + w1 * td[i, 1] + w2 * td[i, 2]
            ii, jj = np.nonzero(m)
            py, px = ii + y0[i], jj + x0[i]
            zz = z[ii, jj]
            closer = zz < zbuf[py, px]
            if not closer.any():
                continue
            py, px, zz = py[closer], px[closer], zz[closer]
            a0, a1, a2 = w0[ii, jj][closer], w1[ii, jj][closer], w2[ii, jj][closer]
            zbuf[py, px] = zz
            if nrm is not None:
                n = (a0[:, None] * nrm[tri[i, 0]] + a1[:, None] * nrm[tri[i, 1]] + a2[:, None] * nrm[tri[i, 2]])
                n /= np.maximum(np.linalg.norm(n, axis=1, keepdims=True), 1e-12)
            else:
                n = np.tile(fn[i], (len(py), 1))
            # make normals face the camera (two-sided lighting)
            flip = (n @ fwd) > 0
            n[flip] *= -1
            if tex is not None and uv is not None:
                u = a0 * uv[tri[i, 0], 0] + a1 * uv[tri[i, 1], 0] + a2 * uv[tri[i, 2], 0]
                vv = a0 * uv[tri[i, 0], 1] + a1 * uv[tri[i, 1], 1] + a2 * uv[tri[i, 2], 1]
                h, w_ = tex.shape[:2]
                tu = np.clip((u % 1.0) * (w_ - 1), 0, w_ - 1).astype(int)
                tv = np.clip((vv % 1.0) * (h - 1), 0, h - 1).astype(int)
                col = tex[tv, tu, :3] * base
            else:
                col = np.tile(base, (len(py), 1))
            lam = np.clip(n @ LIGHT, 0, 1)
            rim = np.clip(-(n @ fwd), 0, 1)
            shade = 0.38 + 0.55 * lam[:, None] + 0.12 * rim[:, None]
            img[py, px] = np.clip(col * shade, 0, 1)

    # outline: darken pixels on depth discontinuities
    filled = np.isfinite(zbuf)
    edge = np.zeros_like(filled)
    edge[1:, :] |= filled[1:, :] != filled[:-1, :]
    edge[:, 1:] |= filled[:, 1:] != filled[:, :-1]
    zf = np.where(filled, zbuf, 0)
    dz = np.zeros_like(zf)
    dz[1:, :] = np.maximum(dz[1:, :], np.abs(zf[1:, :] - zf[:-1, :]))
    dz[:, 1:] = np.maximum(dz[:, 1:], np.abs(zf[:, 1:] - zf[:, :-1]))
    edge |= filled & (dz * scale > 6)
    img[edge] *= 0.35
    im = Image.fromarray((img * 255).astype(np.uint8))

    if draw_skeleton and g.skin is not None:
        W = glbmod.world_matrices(g, anim, t)
        dr = ImageDraw.Draw(im)
        joints = g.skin["joints"]
        jset = set(joints)
        for j in joints:
            if g.nodes[j].get("name") == "Root":
                continue  # the root bone sits at the world origin; drawing it only adds clutter
            p = W[j][:3, 3]
            for c in g.nodes[j].get("children", []):
                if c in jset:
                    q = W[c][:3, 3]
                    a = project(p[None])
                    b = project(q[None])
                    dr.line([(a[0][0], a[1][0]), (b[0][0], b[1][0])], fill=(220, 40, 90), width=2)
            a = project(p[None])
            dr.ellipse([a[0][0] - 2.5, a[1][0] - 2.5, a[0][0] + 2.5, a[1][0] + 2.5], fill=(220, 40, 90))
    return im


def _label(im: Image.Image, text: str) -> Image.Image:
    dr = ImageDraw.Draw(im)
    dr.rectangle([0, 0, 8 + 6 * len(text), 16], fill=(30, 30, 30))
    dr.text((4, 2), text, fill=(255, 255, 255))
    return im


def turnaround(g: glbmod.Glb, size: int = 320, skeleton: bool = False,
               views: Sequence[str] = ("front", "left", "top", "three_quarter")) -> Image.Image:
    fb = bounds(g)
    tiles = [_label(render_view(g, v, size, frame_bounds=fb, draw_skeleton=skeleton), v) for v in views]
    cols = 2 if len(tiles) == 4 else len(tiles)
    rows = (len(tiles) + cols - 1) // cols
    sheet = Image.new("RGB", (cols * size, rows * size), (255, 255, 255))
    for i, t in enumerate(tiles):
        sheet.paste(t, ((i % cols) * size, (i // cols) * size))
    return sheet


def preview_times(anim: dict, frames: int):
    # One-shot recovery/death endpoints must be visible in the review. Cycles
    # omit the duplicate terminal pose. Unknown metadata retains legacy sampling.
    return np.linspace(0, anim["duration"], frames, endpoint=anim.get("loop") is False)


def animation_strip(g: glbmod.Glb, anim: dict, frames: int = 6, size: int = 220,
                    view: str = "three_quarter") -> Image.Image:
    fb = bounds(g, [None, anim], samples=frames)
    ts = preview_times(anim, frames)
    strip = Image.new("RGB", (frames * size, size), (255, 255, 255))
    for i, t in enumerate(ts):
        im = render_view(g, view, size, anim=anim, t=float(t), frame_bounds=fb, draw_skeleton=False)
        _label(im, "%s t=%.2f" % (anim["name"], t))
        strip.paste(im, (i * size, 0))
    return strip


def animation_gif(g: glbmod.Glb, anim: dict, path: str, frames: int = 16, size: int = 240,
                  view: str = "three_quarter") -> None:
    fb = bounds(g, [None, anim], samples=8)
    ts = preview_times(anim, frames)
    ims = [render_view(g, view, size, anim=anim, t=float(t), frame_bounds=fb) for t in ts]
    dur = int(1000 * anim["duration"] / frames) if anim["duration"] > 0 else 80
    ims[0].save(path, save_all=True, append_images=ims[1:], duration=max(dur, 40), loop=0)
