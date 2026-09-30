"""Compile a ModelSpec into a Dust3D snapshot XML and a .ds3 document."""

from __future__ import annotations

import hashlib
import uuid
from typing import Dict, List, Tuple
from xml.sax.saxutils import quoteattr

from .spec import Group, ModelSpec, Part, animation_timing, resolve_flatten

CENTER_NUDGE = 0.001
_NS = uuid.UUID("6f1c3a52-9d3e-4f1b-8a57-d3d3d3d3d3d3")

def canvas_origin(spec: ModelSpec) -> Tuple[float, float, float]:
    """Place the world origin on Dust3D's editing canvas.

    Only affects where the node profiles appear in the editor (geometry is always
    relative to the origin): the front profile hugs the left margin and the side
    profile sits right after it, like hand-made documents.
    """
    nodes = [n for p in spec.parts for n in p.nodes]
    xs = [abs(n[0]) + n[3] for n in nodes]
    ys = [n[1] + n[3] for n in nodes]
    zs = [n[2] + n[3] for n in nodes]
    ox = 0.1 + max(xs)
    oy = 0.1 + max(ys)
    oz = ox + max(xs) + 0.15 + max(zs)
    return ox, oy, oz


def _uid(*names: str) -> str:
    """Deterministic ids, so re-compiling an edited spec produces a minimal diff."""
    return "{%s}" % uuid.uuid5(_NS, "/".join(names))


def _fmt(v: float) -> str:
    return "%.6f" % v


def _attrs(d: Dict[str, str]) -> str:
    # Dust3D's own saver historically did not escape attribute values; we always do.
    return "".join(" %s=%s" % (k, quoteattr(str(v))) for k, v in sorted(d.items()))


SLOT_MARK = " @"


def component_name(name: str, slot: str = "") -> str:
    """A part's component name in Dust3D. An equipment slot rides along as a suffix
    ("vest @armor/2"): it shows in the editor's part list, can be set there by renaming, and
    reaches the exported glTF (mesh.extras.dust3dParts), where gamekit splits the slots."""
    return name + SLOT_MARK + slot if slot else name


def split_component_name(name: str):
    """(name, slot) from a component name."""
    if SLOT_MARK in (name or ""):
        base, slot = name.rsplit(SLOT_MARK, 1)
        return base.strip(), slot.strip()
    return name or "", ""


def _swap_side(bone: str) -> str:
    return bone.replace("Left", "\0").replace("Right", "Left").replace("\0", "Right")


def build_document(spec: ModelSpec) -> Tuple[str, Dict[str, bytes], Dict[str, List[str]]]:
    """Return (snapshot_xml, assets, id_map).

    assets maps ds3 asset names ("models/{id}.glb", "images/{id}.png") to bytes.
    id_map maps part name -> [part id, node ids...].
    """
    ox, oy, oz = canvas_origin(spec)
    canvas = {"originX": _fmt(ox), "originY": _fmt(oy), "originZ": _fmt(oz)}
    if spec.rig:
        canvas["rigType"] = spec.rig
        if spec.headHasEyelids:
            canvas["headHasEyelids"] = "true"

    nodes_xml: List[str] = []
    edges_xml: List[str] = []
    parts_xml: List[str] = []
    extra_comps_xml: List[str] = []
    id_map: Dict[str, List[str]] = {}
    assets: Dict[str, bytes] = {}
    file_ids: Dict[str, str] = {}
    cut_face_ids: Dict[str, str] = {}

    def asset_id(path: str, folder: str, ext: str) -> str:
        if path in file_ids:
            return file_ids[path]
        with open(path, "rb") as f:
            data = f.read()
        aid = "{%s}" % uuid.uuid5(_NS, hashlib.sha1(data).hexdigest())
        assets["%s/%s.%s" % (folder, aid, ext)] = data
        file_ids[path] = aid
        return aid

    def cut_face_part(profile) -> str:
        """Emit (once per distinct profile) a target=CutFace part and return its id."""
        key = repr(profile)
        if key in cut_face_ids:
            return cut_face_ids[key]
        tag = "cutface%d" % len(cut_face_ids)
        cpid = _uid(spec.name, "part", tag)
        cut_face_ids[key] = cpid
        stroke = isinstance(profile, dict)
        pts = profile["stroke"] if stroke else [[u, v, 0.005] for u, v in profile]
        s_ = 0.1  # profiles are normalized by Dust3D, only the shape matters
        ids = []
        for i, (u, v, r) in enumerate(pts):
            nid = _uid(spec.name, "node", tag, str(i))
            ids.append(nid)
            nodes_xml.append("  <node%s/>" % _attrs({
                "id": nid, "partId": cpid, "radius": _fmt(r * s_),
                "x": _fmt(ox + u * s_), "y": _fmt(oy - v * s_), "z": _fmt(oz)}))
        pairs = list(zip(range(len(ids)), range(1, len(ids))))
        if not stroke:
            pairs.append((len(ids) - 1, 0))
        for ei, (a, b) in enumerate(pairs):
            edges_xml.append("  <edge%s/>" % _attrs({"id": _uid(spec.name, "edge", tag, str(ei)),
                                                     "partId": cpid, "from": ids[a], "to": ids[b]}))
        parts_xml.append("  <part%s/>" % _attrs({
            "id": cpid, "name": tag, "visible": "true", "locked": "false", "disabled": "false",
            "xMirrored": "false", "subdived": "false", "rounded": "false", "chamfered": "false",
            "target": "CutFace"}))
        extra_comps_xml.append("  <component%s>\n  </component>" % _attrs({
            "id": _uid(spec.name, "component", tag), "name": tag, "linkData": cpid,
            "linkDataType": "partId", "combineMode": "Normal", "expanded": "false"}))
        return cpid

    def emit_chain(p: Part, key: str, flip_x: bool = False) -> str:
        pid = _uid(spec.name, "part", key)
        node_ids = []
        for i, (x, y, z, r) in enumerate(p.nodes):
            nid = _uid(spec.name, "node", key, str(i))
            node_ids.append(nid)
            x = -x if flip_x else x
            # world -> canvas: canvas y grows downward, canvas z grows backward
            attrs = {"id": nid, "partId": pid, "radius": _fmt(r),
                     "x": _fmt(ox + x), "y": _fmt(oy - y), "z": _fmt(oz - z)}
            if p.node_deform and i < len(p.node_deform):
                w_, t_ = p.node_deform[i]
                if w_ != 1.0:
                    attrs["deformWidth"] = _fmt(w_)
                if t_ != 1.0:
                    attrs["deformThickness"] = _fmt(t_)
            nodes_xml.append("  <node%s/>" % _attrs(attrs))
        pairs = list(zip(range(len(node_ids)), range(1, len(node_ids))))
        if p.loop and len(node_ids) > 2:
            pairs.append((len(node_ids) - 1, 0))
        for ei, (a, b) in enumerate(pairs):
            e = {"id": _uid(spec.name, "edge", key, str(ei)), "partId": pid,
                 "from": node_ids[a], "to": node_ids[b]}
            if ei < len(p.bones) and p.bones[ei]:
                e["boneName"] = _swap_side(p.bones[ei]) if flip_x else p.bones[ei]
            edges_xml.append("  <edge%s/>" % _attrs(e))
        id_map[key] = [pid] + node_ids
        return pid

    def component(cid, name, extra, children_xml="", depth=0):
        pad = " " * (depth + 2)
        return "%s<component%s>\n%s%s</component>" % (pad, _attrs(dict(extra, id=cid, name=name, expanded="false")),
                                                        children_xml, pad)

    def emit_part(p: Part, depth: int) -> str:
        pid = emit_chain(p, p.name)
        part = {
            "id": pid, "name": p.name, "visible": "true", "locked": "false",
            "disabled": str(p.disabled).lower(), "xMirrored": str(p.mirror).lower(),
            "subdived": str(p.subdivided).lower(), "rounded": str(p.rounded).lower(),
            "chamfered": str(p.chamfered).lower(), "target": p.kind,
        }
        if p.kind == "ImportedModel":
            part["importedModelId"] = asset_id(p.import_path, "models", "glb")
        if isinstance(p.cutFace, str):
            if p.cutFace != "Quad":
                part["cutFace"] = p.cutFace
        else:
            part["cutFace"] = cut_face_part(p.cutFace)
        if p.cutRotation:
            part["cutRotation"] = _fmt(p.cutRotation)
        dw, dt, _ = resolve_flatten(p)
        if dt != 1.0:
            part["deformThickness"] = _fmt(dt)
        if dw != 1.0:
            part["deformWidth"] = _fmt(dw)
        if p.deformUnified:
            part["deformUnified"] = "true"
        if not p.interpolate:
            part["interpolated"] = "false"
        if p.hard:
            part["hard"] = "true"
        if p.metallic:
            part["metallic"] = _fmt(p.metallic)
        if p.roughness != 1.0:
            part["roughness"] = _fmt(p.roughness)
        if p.emissive:
            part["emissive"] = _fmt(p.emissive)
        parts_xml.append("  <part%s/>" % _attrs(part))
        comp = {"linkData": pid, "linkDataType": "partId", "combineMode": p.combine,
                "color": p.color, "smoothCutoffDegrees": _fmt(p.smooth)}
        if p.image:
            comp["colorImageId"] = asset_id(p.image, "images", "png")
        return component(_uid(spec.name, "component", p.name), component_name(p.name, p.slot), comp, depth=depth) + "\n"

    def emit_member(m: Part, g: Group, depth: int, flip_x: bool) -> str:
        key = m.name + ("~mirror" if flip_x else "")
        if m.kind == "StitchingLine" and any(abs(n[0]) < CENTER_NUDGE / 2 for n in m.nodes):
            # A stitched shell lying exactly on x = 0 passes through the centre-plane vertices of
            # the tube parts it is unioned with; Dust3D's boolean operation produces non-manifold
            # edges and slivers for such exactly-coincident geometry. An invisible offset avoids it.
            m = Part(**dict(m.__dict__, nodes=[[CENTER_NUDGE if abs(n[0]) < CENTER_NUDGE / 2 else n[0]] + n[1:]
                                              for n in m.nodes]))
        pid = emit_chain(m, key, flip_x)
        part = {"id": pid, "name": key, "visible": "true", "locked": "false",
                "disabled": str(m.disabled).lower(), "xMirrored": "false", "subdived": "false",
                "rounded": "false", "chamfered": "false", "target": m.kind}
        if m.fillInterior:
            part["fillLoopInterior"] = "true"
        parts_xml.append("  <part%s/>" % _attrs(part))
        comp = {"linkData": pid, "linkDataType": "partId", "combineMode": "Normal"}
        # stitching loops colour the surface region nearest to each loop with that loop's own
        # colour (uncoloured loops come out white), so members inherit the group colour
        color = m.color or g.color
        if color:
            comp["color"] = color
        return component(_uid(spec.name, "component", key), key, comp, depth=depth) + "\n"

    def emit_group(g: Group, depth: int, flip_x: bool = False) -> str:
        key = g.name + ("~mirror" if flip_x else "")
        comp = {"combineMode": g.combine, "smoothCutoffDegrees": _fmt(g.smooth)}
        if g.color:
            comp["color"] = g.color
        if g.image:
            comp["colorImageId"] = asset_id(g.image, "images", "png")
        children = ""
        if g.stitch:
            if g.stitch == "lines":
                comp.update(frontClosed=str(g.frontClosed).lower(), backClosed=str(g.backClosed).lower(),
                            sideClosed=str(g.sideClosed).lower())
                # mirroring reverses orientation; reversing the line order restores it
                members = list(reversed(g.children)) if flip_x else g.children
            else:
                comp.update(backClosed=str(g.backClosed).lower(),
                            backCloseDepthRatio=_fmt(g.backCloseDepthRatio),
                            backCloseSharpness=_fmt(g.backCloseSharpness))
                members = g.children
            if g.targetSegments:
                comp["targetSegments"] = str(int(g.targetSegments))
            for m in members:
                children += emit_member(m, g, depth + 1, flip_x)
        else:
            for c in g.children:
                children += emit_element(c, depth + 1)
        out = component(_uid(spec.name, "component", key), key, comp, children, depth) + "\n"
        if g.stitch == "lines" and g.mirror and not flip_x:
            out += emit_group(g, depth, flip_x=True)
        return out

    def emit_element(e, depth: int) -> str:
        return emit_group(e, depth) if isinstance(e, Group) else emit_part(e, depth)

    def arrange(items: List, owner: str) -> List:
        """Dust3D unions consecutive children with the same combine mode as one run; an
        Uncombined child (which adds no geometry) splits that run, and so do the mirrored
        copies Dust3D appends after it. Parking Uncombined parts in a trailing Normal-mode
        group keeps every solid part in a single run, which gives much cleaner bridged seams."""
        if not spec.autoOrder:
            return items
        # Detached elements (parts or whole groups, e.g. a stitched wing set) go last: one
        # left between solid parts ends the solid run, and the solid parts after it, which
        # include every mirrored copy Dust3D appends, would be unioned apart from the body.
        solid = [e for e in items if e.combine != "Uncombined"]
        detached = [e for e in items if e.combine == "Uncombined"]
        if not detached or not solid:
            return items
        return solid + [Group(name=owner + "_uncombined", children=detached)]

    def rearrange_tree(items: List, owner: str) -> List:
        out = []
        for e in items:
            if isinstance(e, Group) and not e.stitch and not e.name.endswith("_uncombined"):
                e = Group(**dict(e.__dict__, children=rearrange_tree(e.children, e.name)))
            out.append(e)
        return arrange(out, owner)

    comps_xml = "".join(emit_element(e, 0) for e in rearrange_tree(spec.elements, spec.name))

    anims_xml = []
    for i, a in enumerate(spec.animations):
        if a.type == "Pose":
            continue  # posed clips are keyed onto the exported rig by the toolkit (gamekit.py)
        duration, frames = animation_timing(a)
        d = {"id": _uid(spec.name, "animation", a.name, str(i)), "name": a.name, "type": a.type,
             "durationSeconds": _fmt(duration), "frameCount": _fmt(frames)}
        for k, v in a.params.items():
            d[k] = v if isinstance(v, str) else (str(v).lower() if isinstance(v, bool) else _fmt(float(v)))
        anims_xml.append("  <animation%s/>" % _attrs(d))

    xml = ['<?xml version="1.0" encoding="UTF-8"?>',
           "<canvas%s>" % _attrs(canvas),
           " <nodes>", *nodes_xml, " </nodes>",
           " <edges>", *edges_xml, " </edges>",
           " <parts>", *parts_xml, " </parts>",
           " <components>", comps_xml + "".join(c + "\n" for c in extra_comps_xml) + " </components>",
           " <animations>", *anims_xml, " </animations>",
           "</canvas>", ""]
    return "\n".join(xml), assets, id_map


def build_snapshot(spec: ModelSpec) -> Tuple[str, Dict[str, List[str]]]:
    """Backwards-compatible wrapper: (snapshot_xml, id_map). Use build_document for assets."""
    xml, _, id_map = build_document(spec)
    return xml, id_map


def compile_to_ds3(spec: ModelSpec, path: str) -> None:
    xml, assets, _ = build_document(spec)
    write_ds3(path, xml, assets)


def write_ds3(path: str, model_xml: str, assets: Dict[str, bytes] = None) -> None:
    """Write a .ds3 container (same layout as dust3d::Ds3FileWriter)."""
    items = [("model", "model.xml", model_xml.encode("utf-8"))]
    for name, data in (assets or {}).items():
        items.append(("asset", name, data))
    header = ['<?xml version="1.0" encoding="UTF-8"?>', "<ds3>"]
    off = 0
    for typ, name, data in items:
        header.append('    <%s name=%s offset="%d" size="%d"/>' % (typ, quoteattr(name), off, len(data)))
        off += len(data)
    header.append("</ds3>")
    header_xml = ("\n".join(header) + "\n").encode("utf-8")
    first = b"\xd3\x3dDUST3D 1.0 xml "
    header_size = len(first) + 12 + len(header_xml)
    with open(path, "wb") as f:
        f.write(first)
        f.write(b"%010u\r\n" % header_size)
        f.write(header_xml)
        for _, _, data in items:
            f.write(data)


def read_ds3_model_xml(path: str) -> str:
    """Extract model.xml from a .ds3 file (useful for learning from existing models)."""
    import xml.etree.ElementTree as ET
    with open(path, "rb") as f:
        data = f.read()
    first = data.split(b"\n", 1)[0]
    off = int(first.split()[3])
    root = ET.fromstring(data[len(first) + 1:off].decode("utf-8").strip())
    for it in root:
        if it.get("name") == "model.xml":
            o, s = int(it.get("offset")), int(it.get("size"))
            return data[off + o:off + o + s].decode("utf-8")
    raise ValueError("model.xml not found in %s" % path)


def read_ds3_assets(path: str) -> Dict[str, bytes]:
    """All non-model items of a .ds3 file (imported models, images, canvas.png)."""
    import xml.etree.ElementTree as ET
    with open(path, "rb") as f:
        data = f.read()
    first = data.split(b"\n", 1)[0]
    off = int(first.split()[3])
    root = ET.fromstring(data[len(first) + 1:off].decode("utf-8").strip())
    out = {}
    for it in root:
        if it.tag == "asset":
            o, s = int(it.get("offset")), int(it.get("size"))
            out[it.get("name")] = data[off + o:off + o + s]
    return out
