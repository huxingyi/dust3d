"""Convert an existing Dust3D .ds3 / snapshot XML into an agent spec (best effort).

Useful for (a) giving an agent worked examples in its own vocabulary and
(b) round-trip testing the compiler.
"""

from __future__ import annotations

import xml.etree.ElementTree as ET
from typing import Any, Dict, List, Tuple

from .ds3 import read_ds3_assets, read_ds3_model_xml, split_component_name
from .spec import WRAP_ATTRS


def _argb_to_rgb(c: str) -> str:
    if not c:
        return "#e0e0e0"
    c = c.strip()
    if len(c) == 9:
        return "#" + c[3:]
    return c


def decompile_xml(xml: str, name: str = "model", assets: Dict[str, bytes] = None,
                  files: Dict[str, bytes] = None) -> Tuple[Dict[str, Any], List[str]]:
    """files (output) receives extracted imported meshes / images, keyed by the file name the spec refers to."""
    warnings: List[str] = []
    assets = assets or {}
    files = files if files is not None else {}
    root = ET.fromstring(xml)
    ox, oy, oz = (float(root.get(k, "0")) for k in ("originX", "originY", "originZ"))
    nodes = {n.get("id"): n for n in root.find("nodes") or []}
    edges = list(root.find("edges") or [])
    parts = {p.get("id"): p for p in root.find("parts") or []}
    comps: Dict[str, ET.Element] = {}

    def walk(el):
        for c in el:
            if c.tag == "component":
                if c.get("linkDataType") == "partId":
                    comps[c.get("linkData")] = c
                walk(c)
    if root.find("components") is not None:
        walk(root.find("components"))
    def chain_of(pid):
        """Order a part's nodes along its edges. Returns (node ids, bone per edge, is_loop, extra count)."""
        pn = [nid for nid, n in nodes.items() if n.get("partId") == pid]
        nxt: Dict[str, Tuple[str, str]] = {}
        indeg: Dict[str, int] = {n: 0 for n in pn}
        for e in edges:
            if e.get("partId") != pid:
                continue
            f, t = e.get("from"), e.get("to")
            if f in nxt:
                warnings.append("part %s branches at node %s; Dust3D keeps only one link" % (pid, f))
            nxt[f] = (t, e.get("boneName") or "")
            indeg[t] = indeg.get(t, 0) + 1
        starts = [n for n in pn if indeg.get(n, 0) == 0]
        loop = False
        if not starts:
            starts = sorted(pn)[:1]
            loop = len(pn) > 2
        chain, bones, seen = [], [], set()
        cur = starts[0] if starts else None
        while cur is not None and cur not in seen:
            seen.add(cur)
            chain.append(cur)
            if cur in nxt:
                t, b = nxt[cur]
                if t in seen:
                    if loop:
                        bones.append(b)
                    break
                bones.append(b)
                cur = t
            else:
                cur = None
        return chain, bones, loop, len(pn) - len(seen)

    def profile_of(pid):
        chain, _, loop, _ = chain_of(pid)
        pts = [(float(nodes[n].get("x")) - ox, oy - float(nodes[n].get("y")), float(nodes[n].get("radius"))) for n in chain]
        if loop or len(pts) >= 3 and not any(e.get("partId") == pid for e in edges):
            return [[round(u, 6), round(v, 6)] for u, v, _ in pts]
        if len(pts) >= 2:
            return {"stroke": [[round(u, 6), round(v, 6), round(r, 6)] for u, v, r in pts]}
        return None

    def node_list(chain):
        out = [[round(float(nodes[n].get("x")) - ox, 6), round(oy - float(nodes[n].get("y")), 6),
                round(oz - float(nodes[n].get("z")), 6), round(float(nodes[n].get("radius")), 6)] for n in chain]
        if any(nodes[n].get("deformWidth") or nodes[n].get("deformThickness") for n in chain):
            for row, n in zip(out, chain):
                row += [round(float(nodes[n].get("deformWidth") or 1.0), 6),
                        round(float(nodes[n].get("deformThickness") or 1.0), 6)]
        return out

    used_names = set()

    group_ids: Dict[str, str] = {}

    def uniq(nm):
        base, i = nm, 2
        while nm in used_names:
            nm = "%s_%d" % (base, i)
            i += 1
        used_names.add(nm)
        return nm

    def asset_file(aid, folder, ext):
        key = "%s/%s.%s" % (folder, aid, ext)
        if key not in assets:
            warnings.append("asset %s missing from document" % key)
            return None
        fname = "%s_%s.%s" % (name, aid.strip("{}")[:8], ext)
        files[fname] = assets[key]
        return fname

    def tube(c, p, pid):
        chain, bones, loop, dropped = chain_of(pid)
        if dropped:
            warnings.append("part %s: %d nodes not on the main chain were dropped" % (pid, dropped))
        sp: Dict[str, Any] = {"name": uniq(p.get("name") or c.get("name") or "part_" + pid.strip("{}")[:8]),
                              "nodes": node_list(chain)}
        if any(bones):
            sp["bones"] = bones
        if p.get("xMirrored") == "true":
            sp["mirror"] = True
        comp_name, slot = split_component_name(c.get("name") or "")
        if slot:
            sp["slot"] = slot
            if not p.get("name"):
                sp["name"] = uniq(comp_name)
        sp["color"] = _argb_to_rgb(c.get("color", ""))
        if c.get("combineMode", "Normal") != "Normal":
            sp["combine"] = c.get("combineMode")
        if c.get("smoothCutoffDegrees"):
            sp["smooth"] = round(float(c.get("smoothCutoffDegrees")), 2)
        if c.get("colorImageId"):
            f = asset_file(c.get("colorImageId"), "images", "png")
            if f:
                sp["image"] = f
        if p.get("target") == "ImportedModel":
            f = asset_file(p.get("importedModelId", ""), "models", "glb")
            if f:
                sp["import"] = f
        cf = p.get("cutFace")
        if cf in ("Pentagon", "Hexagon", "Triangle"):
            sp["cutFace"] = cf
        elif cf and cf.startswith("{"):
            prof = profile_of(cf) if cf in parts else None
            if prof is None:
                warnings.append("part %s uses a custom cut face that could not be read; replaced with Quad" % pid)
            else:
                sp["cutFace"] = prof
        for k_xml, k_spec in (("rounded", "rounded"), ("subdived", "subdivided"), ("chamfered", "chamfered")):
            v = p.get(k_xml) == "true"
            if v != (k_spec in ("rounded", "subdivided")):
                sp[k_spec] = v
        for k in ("deformThickness", "deformWidth", "cutRotation", "metallic", "roughness", "emissive"):
            if p.get(k):
                sp[k] = round(float(p.get(k)), 6)
        if p.get("deformUnified") == "true":
            sp["deformUnified"] = True
        if p.get("interpolated") == "false":
            sp["interpolate"] = False
        if p.get("hard") == "true":
            sp["hard"] = True
        if loop:
            sp["loop"] = True
        if p.get("disabled") == "true":
            sp["disabled"] = True
        return sp

    def member(c, p, pid, kind):
        chain, bones, loop, _ = chain_of(pid)
        m: Dict[str, Any] = {"name": uniq(p.get("name") or c.get("name") or "m_" + pid.strip("{}")[:8]),
                             "nodes": node_list(chain)}
        if any(bones):
            m["bones"] = bones
        if c.get("color"):
            m["color"] = _argb_to_rgb(c.get("color"))
        if kind == "StitchingLoop":
            if loop:
                m["closed"] = True
            if p.get("fillLoopInterior") == "true":
                m["fillInterior"] = True
        if p.get("disabled") == "true":
            m["disabled"] = True
        return m

    def element(c):
        if c.get("linkDataType") == "partId":
            pid = c.get("linkData")
            p = parts.get(pid)
            if p is None:
                return None
            target = p.get("target", "Model") or "Model"
            if target == "CutFace":
                return None  # emitted inline as the cutFace of the parts that use it
            if target in ("Model", "ImportedModel"):
                return tube(c, p, pid)
            warnings.append("part %s target=%s outside a group; skipped" % (pid, target))
            return None
        kids = c.findall("component")
        kinds = []
        for k in kids:
            p = parts.get(k.get("linkData")) if k.get("linkDataType") == "partId" else None
            kinds.append(p.get("target", "Model") if p is not None else "Group")
        raw_name, gslot = split_component_name(c.get("name") or "")
        gname = uniq(raw_name or "group_" + (c.get("id") or "").strip("{}")[:8])
        common: Dict[str, Any] = {"name": gname}
        group_ids[c.get("id") or ""] = gname
        if c.get("wrap") in ("Skin", "Cloth"):
            wrap: Dict[str, Any] = {"mode": "cloth" if c.get("wrap") == "Cloth" else "creature"}
            for k, attr in WRAP_ATTRS.items():
                if c.get(attr):
                    wrap[k] = int(float(c.get(attr))) if k == "faces" else round(float(c.get(attr)), 6)
            if c.get("wrapBindTo"):
                wrap["bindTo"] = c.get("wrapBindTo")  # resolved to the group's name below
            if c.get("wrapKeep") in ("true", "false"):
                wrap["keep"] = c.get("wrapKeep") == "true"
            common["wrap"] = wrap
            if gslot:
                common["slot"] = gslot
        elif gslot:
            gname = uniq(c.get("name"))
            common["name"] = gname
        if c.get("combineMode", "Normal") != "Normal":
            common["combine"] = c.get("combineMode")
        if c.get("color"):
            common["color"] = _argb_to_rgb(c.get("color"))
        if c.get("smoothCutoffDegrees"):
            common["smooth"] = round(float(c.get("smoothCutoffDegrees")), 2)
        if c.get("colorImageId"):
            f = asset_file(c.get("colorImageId"), "images", "png")
            if f:
                common["image"] = f
        seg = int(float(c.get("targetSegments") or 0))
        for stitch_kind, key in (("StitchingLine", "lines"), ("StitchingLoop", "loops")):
            if stitch_kind in kinds:
                if any(k != stitch_kind for k in kinds):
                    warnings.append("group %s mixes %s with other children; they are split into nested groups"
                                    % (gname, stitch_kind))
                break
        else:
            stitch_kind = None
        if stitch_kind and all(k == stitch_kind for k in kinds):
            g = dict(common, stitch=key)
            g[key] = [member(k, parts[k.get("linkData")], k.get("linkData"), stitch_kind) for k in kids]
            if stitch_kind == "StitchingLine":
                for flag in ("frontClosed", "backClosed", "sideClosed"):
                    if c.get(flag) == "true":
                        g[flag] = True
            else:
                if c.get("backClosed") == "true":
                    g["backClosed"] = True
                for k in ("backCloseDepthRatio", "backCloseSharpness"):
                    if c.get(k):
                        g[k] = round(float(c.get(k)), 6)
            if seg:
                g["targetSegments"] = seg
            return g
        children = [e for e in (element(k) for k in kids) if e is not None]
        if not children:
            return None
        return dict(common, group=children)

    comps_root = root.find("components")
    out_parts = [e for e in (element(c) for c in (comps_root.findall("component") if comps_root is not None else []))
                 if e is not None]

    # a decompiled document keeps its author's exact component order
    def resolve_bind(items):
        for e in items:
            if isinstance(e, dict) and isinstance(e.get("group"), list):
                wrap = e.get("wrap")
                if wrap and wrap.get("bindTo"):
                    target = group_ids.get(wrap["bindTo"])
                    if target:
                        wrap["bindTo"] = target
                    else:
                        warnings.append("group %s: wrap bindTo %s is not a group; dropped" % (e.get("name"), wrap["bindTo"]))
                        del wrap["bindTo"]
                resolve_bind(e["group"])

    resolve_bind(out_parts)
    spec: Dict[str, Any] = {"name": name, "autoOrder": False, "parts": out_parts}
    if root.get("rigType") and root.get("rigType") != "None":
        spec["rig"] = root.get("rigType")
    if root.get("headHasEyelids") == "true":
        spec["headHasEyelids"] = True
    anims = []
    for a in root.find("animations") or []:
        params = {k: v for k, v in a.attrib.items() if k not in ("id", "name", "type")}
        anims.append({"type": a.get("type"), "name": a.get("name"), "params": params})
    if anims:
        spec["animations"] = anims
    return spec, warnings


def decompile_file(path: str, asset_dir: str = None) -> Tuple[Dict[str, Any], List[str]]:
    """Decompile a .ds3 (or raw snapshot XML). Imported meshes and images are written to
    asset_dir (the spec refers to them by file name, relative to the spec)."""
    import os
    assets: Dict[str, bytes] = {}
    if path.endswith(".ds3"):
        xml = read_ds3_model_xml(path)
        assets = read_ds3_assets(path)
    else:
        with open(path, encoding="utf-8") as f:
            xml = f.read()
    files: Dict[str, bytes] = {}
    spec, warnings = decompile_xml(xml, os.path.splitext(os.path.basename(path))[0], assets, files)
    if files:
        if asset_dir is None:
            warnings.append("model references %d asset file(s); pass asset_dir to extract them" % len(files))
        else:
            os.makedirs(asset_dir, exist_ok=True)
            for fname, data in files.items():
                with open(os.path.join(asset_dir, fname), "wb") as f:
                    f.write(data)
    return spec, warnings
