"""Tests for the agent modeling toolkit.

    python3 -m unittest discover -s tools/agent_modeling/tests -v

Integration tests run only when a Dust3D binary is available (DUST3D_BIN or a
build next to the sources). Reference-model round trips additionally need
DUST3D_TEST_MODELS pointing at a checkout of huxingyi/dust3d-test-models.
"""

import glob
import json
import os
import sys
import tempfile
import unittest
import xml.etree.ElementTree as ET

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.dirname(HERE))

from dust3d_agent import ds3, spec as S  # noqa: E402

EXAMPLES = os.path.join(os.path.dirname(HERE), "examples")


def _have_dust3d():
    try:
        from dust3d_agent import export
        export.find_dust3d()
        return True
    except FileNotFoundError:
        return False


class SpecTests(unittest.TestCase):
    @unittest.skipUnless(_have_dust3d(), "rig templates come from the Dust3D binary")
    def test_examples_parse_and_lint_clean(self):
        for f in glob.glob(os.path.join(EXAMPLES, "*.json")):
            sp = S.load_spec(f)
            bad = [w for w in S.lint_spec(sp) if not w.startswith(("info:", "advisory:"))]
            self.assertEqual(bad, [], f)

    @unittest.skipUnless(_have_dust3d(), "rig templates come from the Dust3D binary")
    def test_bad_bone_and_side(self):
        sp = S.parse_spec({"rig": "Quadruped", "parts": [
            {"name": "leg", "nodes": [[-0.1, 0.5, 0, 0.05], [-0.1, 0.1, 0, 0.04]], "bones": ["FrontLeftUpperLeg"]},
            {"name": "x", "nodes": [[0, 0.5, 0, 0.05], [0, 0.6, 0, 0.04]], "bones": ["Wing"]}]})
        w = " ".join(S.lint_spec(sp))
        self.assertIn("Left is +X", w)
        self.assertIn("'Wing' not in Quadruped", w)

    def test_bone_count_must_match_edges(self):
        with self.assertRaises(S.SpecError):
            S.parse_spec({"parts": [{"nodes": [[0, 0, 0, 0.1], [0, 1, 0, 0.1]], "bones": ["A", "B"]}]})

    def test_flatten_straight_z_chain_maps_y_to_width(self):
        # Dust3D's fallback base normal for a straight Z chain is +/-Y, so width squashes Y.
        p = S.parse_spec({"parts": [{"nodes": [[0, 0, 0, 0.1], [0, 0, 0.5, 0.1]], "flatten": {"y": 0.2}}]}).parts[0]
        self.assertEqual(S.resolve_flatten(p)[:2], (0.2, 1.0))
        p = S.parse_spec({"parts": [{"nodes": [[0, 0, 0, 0.1], [0, 0, 0.5, 0.1]], "flatten": {"x": 0.3}}]}).parts[0]
        self.assertEqual(S.resolve_flatten(p)[:2], (1.0, 0.3))

    def test_flatten_bent_chain_uses_bend_plane(self):
        # Chain bending in the YZ plane: base normal is X, so width squashes X.
        p = S.parse_spec({"parts": [{"nodes": [[0, 0, 0, 0.1], [0, 0.3, 0.1, 0.1], [0, 0.3, 0.5, 0.1]],
                                     "flatten": {"x": 0.2}}]}).parts[0]
        self.assertEqual(S.resolve_flatten(p)[:2], (0.2, 1.0))

    def test_extends_variant(self):
        with tempfile.TemporaryDirectory() as d:
            base = {"name": "wolf", "rig": "Quadruped", "defaults": {"color": "#111111"},
                    "parts": [{"name": "body", "color": "#AAAAAA", "nodes": [[0, 0.5, 0, 0.1], [0, 0.5, 0.4, 0.1]]},
                              {"name": "head", "group": [{"name": "skull", "color": "#aaaaaa", "nodes": [[0, 0.6, 0.5, 0.1]]},
                                                         {"name": "ear", "nodes": [[0, 0.7, 0.5, 0.03]]}]},
                              {"name": "mane", "nodes": [[0, 0.6, 0.1, 0.1]]}],
                    "animations": ["QuadrupedWalk"]}
            json.dump(base, open(os.path.join(d, "wolf.json"), "w"))
            variant = {"extends": "wolf.json", "name": "hound", "recolor": {"#aaaaaa": "#ff0000", "#111111": "#222222"},
                       "remove": ["mane", "ear"], "override": {"body": {"cutFace": "Hexagon"}},
                       "add": [{"name": "horn", "nodes": [[0, 0.8, 0.5, 0.02]]}], "scale": 2.0}
            json.dump(variant, open(os.path.join(d, "hound.json"), "w"))
            data = S.load_spec_dict(os.path.join(d, "hound.json"))
        self.assertEqual(data["name"], "hound")
        self.assertEqual(data["rig"], "Quadruped")
        self.assertEqual([p["name"] for p in data["parts"]], ["body", "head", "horn"])
        self.assertEqual(data["parts"][0]["color"], "#ff0000")
        self.assertEqual(data["parts"][1]["group"][0]["color"], "#ff0000")
        self.assertEqual([c["name"] for c in data["parts"][1]["group"]], ["skull"])
        self.assertEqual(data["defaults"]["color"], "#222222")
        self.assertEqual(data["parts"][0]["cutFace"], "Hexagon")
        self.assertEqual(data["parts"][0]["nodes"][1], [0, 1.0, 0.8, 0.2])
        self.assertEqual(data["parts"][2]["nodes"][0], [0, 1.6, 1.0, 0.04])

    def test_extends_add_to_group(self):
        with tempfile.TemporaryDirectory() as d:
            base = {"name": "person", "rig": "Biped",
                    "parts": [{"name": "body", "wrap": {"mode": "creature"},
                               "group": [{"name": "torso", "nodes": [[0, 0.5, 0, 0.1], [0, 0.8, 0, 0.1]]}]},
                              {"name": "tunic", "wrap": {"mode": "cloth"},
                               "group": [{"shell": "torso", "name": "tunic_torso"}]}]}
            json.dump(base, open(os.path.join(d, "person.json"), "w"))
            variant = {"extends": "person.json", "name": "woman",
                       "addTo": {"body": [{"name": "bust", "mirror": True, "nodes": [[0.03, 0.7, 0.05, 0.04]]}],
                                 "tunic": [{"shell": "bust", "name": "tunic_bust"}]}}
            json.dump(variant, open(os.path.join(d, "woman.json"), "w"))
            data = S.load_spec_dict(os.path.join(d, "woman.json"))
            self.assertEqual([c["name"] for c in data["parts"][0]["group"]], ["torso", "bust"])
            self.assertEqual([c["name"] for c in data["parts"][1]["group"]], ["tunic_torso", "tunic_bust"])
            json.dump({"extends": "person.json", "name": "x", "addTo": {"torso": [{"name": "y"}]}},
                      open(os.path.join(d, "x.json"), "w"))
            with self.assertRaisesRegex(S.SpecError, "addTo"):
                S.load_spec_dict(os.path.join(d, "x.json"))

    def test_extends_errors(self):
        with tempfile.TemporaryDirectory() as d:
            json.dump({"name": "a", "extends": "b.json"}, open(os.path.join(d, "a.json"), "w"))
            json.dump({"name": "b", "extends": "a.json", "parts": []}, open(os.path.join(d, "b.json"), "w"))
            with self.assertRaises(S.SpecError):
                S.load_spec_dict(os.path.join(d, "a.json"))
            json.dump({"name": "c", "parts": [{"name": "x", "nodes": [[0, 0, 0, 0.1]]}]}, open(os.path.join(d, "c.json"), "w"))
            json.dump({"name": "e", "extends": "c.json", "remove": ["nope"]}, open(os.path.join(d, "e.json"), "w"))
            with self.assertRaisesRegex(S.SpecError, "nope"):
                S.load_spec_dict(os.path.join(d, "e.json"))

    def test_animation_timing(self):
        sp = S.parse_spec({"name": "t", "rig": "Biped", "parts": [{"name": "b", "nodes": [[0, 0, 0, 0.1]]}],
                           "animations": ["BipedWalk", {"type": "BipedSlam", "name": "attack"},
                                          {"type": "BipedIdle", "params": {"durationSeconds": 2.5}}]})
        xml, _, _ = ds3.build_document(sp)
        anims = {a.get("name"): a for a in ET.fromstring(xml).find("animations")}
        timing = lambda a: (float(anims[a].get("durationSeconds")), float(anims[a].get("frameCount")))
        self.assertEqual(timing("walk"), (1.0, 30.0))
        self.assertEqual(timing("attack"), (0.9, 48.0))
        self.assertEqual(timing("idle")[0], 2.5)
        self.assertIn("BipedWalk", S.LOOPING_ANIMATIONS)
        self.assertNotIn("BipedSlam", S.LOOPING_ANIMATIONS)
        for rig, types in S.ANIMATION_TYPES.items():
            for t in types:
                self.assertIn(t, S.ANIMATION_TIMING, t)

    def test_inversion_in_the_middle_is_linted(self):
        sp = S.parse_spec({"name": "t", "parts": [
            {"name": "skull", "nodes": [[0, 1, 0, 0.1]]},
            {"name": "socket", "combine": "Inversion", "nodes": [[0.03, 1, 0.08, 0.02]]},
            {"name": "arm", "nodes": [[0.1, 0.8, 0, 0.03], [0.2, 0.6, 0, 0.03]]}]})
        self.assertTrue(any("socket" in w and "separate run" in w for w in S.lint_spec(sp)))
        sp = S.parse_spec({"name": "t", "parts": [
            {"name": "arm", "nodes": [[0.1, 0.8, 0, 0.03], [0.2, 0.6, 0, 0.03]]},
            {"name": "head", "group": [{"name": "skull", "nodes": [[0, 1, 0, 0.1]]},
                                       {"name": "socket", "combine": "Inversion", "nodes": [[0.03, 1, 0.08, 0.02]]}]}]})
        self.assertFalse(any("separate run" in w for w in S.lint_spec(sp)))

    def test_flatten_along_chain_is_reported(self):
        p = S.parse_spec({"parts": [{"name": "p", "nodes": [[0, 0, 0, 0.1], [0, 0, 0.5, 0.1]], "flatten": {"z": 0.2}}]}).parts[0]
        self.assertIn("cannot be applied", S.resolve_flatten(p)[2][0])


class Ds3Tests(unittest.TestCase):
    def test_container_roundtrip_and_escaping(self):
        name = 'Tom\'s "big" ear & tail <1>'
        sp = S.parse_spec({"name": "t", "parts": [{"name": name, "nodes": [[0, 0.5, 0, 0.1], [0, 0.5, 0.3, 0.08]]}]})
        xml, _ = ds3.build_snapshot(sp)
        with tempfile.TemporaryDirectory() as d:
            p = os.path.join(d, "t.ds3")
            ds3.write_ds3(p, xml)
            with open(p, "rb") as f:
                self.assertTrue(f.read(8).startswith(b"\xd3\x3dDUST3D"))
            back = ds3.read_ds3_model_xml(p)
        root = ET.fromstring(back)
        self.assertEqual(root.find("parts")[0].get("name"), name)

    def test_world_to_canvas_mapping(self):
        sp = S.parse_spec({"parts": [{"nodes": [[0.1, 0.2, 0.3, 0.05]]}]})
        root = ET.fromstring(ds3.build_snapshot(sp)[0])
        ox, oy, oz = (float(root.get(k)) for k in ("originX", "originY", "originZ"))
        n = root.find("nodes")[0]
        self.assertAlmostEqual(float(n.get("x")) - ox, 0.1, 5)
        self.assertAlmostEqual(oy - float(n.get("y")), 0.2, 5)
        self.assertAlmostEqual(oz - float(n.get("z")), 0.3, 5)

    def test_custom_cut_face_emits_cutface_part(self):
        sp = S.parse_spec({"parts": [{"name": "b", "cutFace": [[1, 0], [0, 0.2], [-1, 0], [0, -0.2]],
                                      "nodes": [[0, 0, 0, 0.05], [0, 1, 0, 0.01]]}]})
        root = ET.fromstring(ds3.build_snapshot(sp)[0])
        parts = {p.get("name"): p for p in root.find("parts")}
        self.assertEqual(parts["cutface0"].get("target"), "CutFace")
        self.assertEqual(parts["b"].get("cutFace"), parts["cutface0"].get("id"))
        ring = [e for e in root.find("edges") if e.get("partId") == parts["cutface0"].get("id")]
        self.assertEqual(len(ring), 4)

    def test_decompile_compile_is_stable(self):
        for f in glob.glob(os.path.join(EXAMPLES, "*.json")):
            sp = S.load_spec(f)
            xml, _ = ds3.build_snapshot(sp)
            from dust3d_agent import decompile
            again, warnings = decompile.decompile_xml(xml, sp.name)
            self.assertEqual(warnings, [], f)
            xml2, _ = ds3.build_snapshot(S.parse_spec(again))
            def model_nodes(x):
                # cut-face profile parts are deduplicated by shape, so count only model nodes
                root = ET.fromstring(x)
                cut = {p.get("id") for p in root.find("parts") if p.get("target") == "CutFace"}
                return sum(1 for n in root.find("nodes") if n.get("partId") not in cut)
            self.assertEqual(model_nodes(xml), model_nodes(xml2), f)


    def test_interpolate_false_roundtrip(self):
        from dust3d_agent import decompile
        sp = S.parse_spec({"parts": [
            {"name": "post", "interpolate": False, "subdivided": False, "nodes": [[0, 0, 0, 0.05], [0, 1, 0, 0.04]]},
            {"name": "soft", "nodes": [[0.2, 0, 0, 0.05], [0.2, 1, 0, 0.04]]}]})
        xml, _ = ds3.build_snapshot(sp)
        parts = {p.get("name"): p for p in ET.fromstring(xml).find("parts")}
        self.assertEqual(parts["post"].get("interpolated"), "false")
        self.assertIsNone(parts["soft"].get("interpolated"))  # default: attribute left out
        again, _ = decompile.decompile_xml(xml, "t")
        back = {p.name: p for p in S.parse_spec(again).parts}
        self.assertFalse(back["post"].interpolate)
        self.assertTrue(back["soft"].interpolate)


class LintAdvisoryTests(unittest.TestCase):
    def test_uncombined_mirrored_part_on_midline(self):
        sp = S.parse_spec({"parts": [
            {"name": "spot", "mirror": True, "combine": "Uncombined", "nodes": [[0.02, 0.3, 0, 0.02], [0.02, 0.3, 0.03, 0.02]]},
            {"name": "ok", "mirror": True, "combine": "Uncombined", "nodes": [[0.1, 0.3, 0, 0.02], [0.1, 0.3, 0.03, 0.02]]},
            {"name": "leg", "mirror": True, "nodes": [[0.02, 0.3, 0, 0.05], [0.1, 0, 0, 0.03]]}]})
        w = [x for x in S.lint_spec(sp) if "mirrored uncombined" in x]
        self.assertEqual(len(w), 1)
        self.assertIn("'spot'", w[0])

    def test_interpolate_false_across_bones(self):
        sp = S.parse_spec({"parts": [{"name": "tail", "interpolate": False, "bones": ["A", "B"],
                                      "nodes": [[0, 0, 0, 0.05], [0, 0, 0.3, 0.04], [0, 0, 0.6, 0.02]]}]})
        self.assertTrue(any("interpolate false" in x for x in S.lint_spec(sp)))

    def test_new_animation_types_registered(self):
        self.assertIn("SnakeStrike", S.ANIMATION_TYPES.get("Snake", []))
        self.assertIn("BipedHop", S.ANIMATION_TYPES.get("Biped", []))
        self.assertIn("BipedHop", S.LOOPING_ANIMATIONS)

    def test_biped_throw_is_a_one_shot_attack(self):
        from dust3d_agent import gamekit
        self.assertIn("BipedThrow", S.ANIMATION_TYPES["Biped"])
        self.assertNotIn("BipedThrow", S.LOOPING_ANIMATIONS)
        self.assertEqual(S.ANIMATION_TIMING["BipedThrow"], (0.9, 36))
        # attack clips get a hit event: throws, kicks and bites too
        for t in ("BipedThrow", "BipedKick", "InsectBite"):
            self.assertTrue(any(w in t for w in gamekit.ATTACK_WORDS), t)

    def test_every_rig_has_a_game_clip_set(self):
        # a creature in a game needs idle, a way to move, an attack, a hurt and a death
        for rig, types in S.ANIMATION_TYPES.items():
            for kind in ("Idle", "Attack|Strike|Bite|Slam|Stab|Kick", "Hurt", "Die"):
                self.assertTrue(any(any(t.endswith(k) for k in kind.split("|")) for t in types),
                                "%s has no %s clip" % (rig, kind))
            self.assertTrue(any(t in S.LOOPING_ANIMATIONS and not t.endswith("Idle") for t in types),
                            "%s has no movement loop" % rig)
            for t in types:
                if t.endswith(("Hurt", "Die", "Attack", "Strike", "Bite", "Kick")):
                    self.assertNotIn(t, S.LOOPING_ANIMATIONS, t)
                self.assertIn(t, S.ANIMATION_TIMING, t)

    def test_stitched_surfaces_take_materials(self):
        sp = S.parse_spec({"parts": [{"name": "fin", "stitch": "lines", "metallic": 0.5, "emissive": 1.0, "lines": [
            [[0, 0, 0, 0.02], [0, 0.2, 0, 0.01]], [[0, 0, 0.1, 0.02], [0, 0.2, 0.1, 0.01]]]}]})
        self.assertTrue(all(p.metallic == 0.5 and p.emissive == 1.0 for p in sp.parts))


class ShapeTests(unittest.TestCase):
    """Hard-surface shapes expand into plain tube parts with the right size and frame."""

    def _bbox(self, part):
        # the swept prism's corners: every node x every cut-face point (u, v frame from the spec)
        axes = S.tube_axes(part.nodes)
        u, v = axes
        pts = []
        for i, n in enumerate(part.nodes):
            w, t = part.node_deform[i] if part.node_deform else (1.0, 1.0)
            for a, b in part.cutFace:
                pts.append([n[k] + (u[k] * a * w + v[k] * b * t) * n[3] for k in range(3)])
        lo = [min(p[k] for p in pts) for k in range(3)]
        hi = [max(p[k] for p in pts) for k in range(3)]
        return [round(hi[k] - lo[k], 4) for k in range(3)], [round((hi[k] + lo[k]) / 2, 4) for k in range(3)]

    def test_box_size_and_center(self):
        for axis in ("x", "y", "z", None):
            sp = S.parse_spec({"parts": [{"shape": "box", "name": "b", "center": [0.1, 0.5, -0.2],
                                          "size": [0.6, 0.3, 0.2], **({"axis": axis} if axis else {})}]})
            p = sp.parts[0]
            size, center = self._bbox(p)
            self.assertEqual(size, [0.6, 0.3, 0.2], axis)
            self.assertEqual(center, [0.1, 0.5, -0.2], axis)
            self.assertFalse(p.rounded)
            self.assertFalse(p.interpolate)
            self.assertTrue(p.hard)
            self.assertEqual(p.combine, "Uncombined")  # kitbashed by default

    def test_offcentre_profile_is_recentred(self):
        # an L section's bounding box is not centred on the path: the nodes move instead
        sp = S.parse_spec({"parts": [{"shape": "beam", "name": "l", "path": [[0, 0, 0], [0, 1, 0]],
                                      "profile": "polygon", "points": [[0, 0], [0.2, 0], [0.2, 0.05], [0, 0.05]]}]})
        size, center = self._bbox(sp.parts[0])
        self.assertAlmostEqual(size[1], 1.0, 4)
        self.assertEqual(sorted(size[i] for i in (0, 2)), [0.05, 0.2])

    def test_taper_per_direction_uses_node_scale(self):
        sp = S.parse_spec({"parts": [{"shape": "box", "name": "w", "center": [0, 0, 0], "size": [1.0, 0.4, 0.2],
                                      "axis": "x", "taper": [0.5, 1.0]}]})
        p = sp.parts[0]
        self.assertEqual(len(p.node_deform), 2)
        self.assertEqual(p.node_deform[0], [1.0, 1.0])
        self.assertIn(0.5, p.node_deform[1])
        xml, _ = ds3.build_snapshot(sp)
        self.assertEqual(sum(1 for n in ET.fromstring(xml).find("nodes") if n.get("deformWidth") or n.get("deformThickness")), 1)

    def test_mirror_emits_explicit_copy_with_swapped_bones(self):
        sp = S.parse_spec({"rig": "", "parts": [{"shape": "box", "name": "pad", "center": [0.2, 0.1, 0], "size": [0.1, 0.1, 0.1],
                                                 "mirror": True, "bones": "LeftFoot"}]})
        names = [p.name for p in sp.parts]
        self.assertEqual(names, ["pad", "pad_mirror"])
        self.assertEqual(sp.parts[1].bones, ["RightFoot"])
        self.assertAlmostEqual(self._bbox(sp.parts[1])[1][0], -0.2, 4)

    def test_cutters_make_their_list_union(self):
        sp = S.parse_spec({"parts": [{"name": "g", "group": [
            {"shape": "box", "name": "body", "center": [0, 0, 0], "size": [0.5, 0.3, 0.3]},
            {"shape": "groove", "name": "seam", "from": [-0.1, 0, 0.15], "to": [0.1, 0, 0.15], "normal": [0, 0, 1]}]}]})
        byname = {p.name: p for p in sp.parts}
        self.assertEqual(byname["body"].combine, "Normal")
        self.assertEqual(byname["seam"].combine, "Inversion")

    def test_hard_and_node_scale_roundtrip(self):
        from dust3d_agent import decompile
        sp = S.parse_spec({"parts": [{"name": "h", "hard": True, "nodes": [[0, 0, 0, 0.1, 1, 1], [0, 1, 0, 0.1, 0.5, 2]]}]})
        xml, _ = ds3.build_snapshot(sp)
        again, _ = decompile.decompile_xml(xml, "t")
        p = S.parse_spec(again).parts[0]
        self.assertTrue(p.hard)
        self.assertEqual(p.node_deform, [[1.0, 1.0], [0.5, 2.0]])

    def test_unknown_shape_key_is_an_error(self):
        with self.assertRaises(S.SpecError):
            S.parse_spec({"parts": [{"shape": "box", "size": [1, 1, 1], "sise": 2}]})


class FeatureCompileTests(unittest.TestCase):
    def _root(self, spec_dict, base_dir=""):
        xml, assets, _ = ds3.build_document(S.parse_spec(spec_dict, base_dir))
        return ET.fromstring(xml), assets

    def test_stitch_lines_group_and_mirror(self):
        root, _ = self._root({"rig": "Fish", "parts": [
            {"name": "body", "nodes": [[0, 0, 0.3, 0.1], [0, 0, -0.3, 0.1]]},
            {"name": "fin", "stitch": "lines", "mirror": True, "backClosed": True, "targetSegments": 4,
             "lines": [{"nodes": [[0.1, 0, 0.1, 0.02], [0.2, -0.05, 0.05, 0.004]], "bones": "LeftPectoralFin"},
                       {"nodes": [[0.1, 0, 0.0, 0.02], [0.2, -0.05, -0.05, 0.004]], "bones": "LeftPectoralFin"}]}]})
        groups = [c for c in root.find("components") if c.get("linkDataType") != "partId"]
        self.assertEqual([g.get("name") for g in groups], ["fin", "fin~mirror"])
        self.assertEqual(groups[0].get("backClosed"), "true")
        self.assertEqual(groups[0].get("targetSegments"), "4")
        parts = {p.get("id"): p for p in root.find("parts")}
        members = [parts[c.get("linkData")] for c in groups[1].findall("component")]
        self.assertTrue(all(m.get("target") == "StitchingLine" for m in members))
        bones = {e.get("boneName") for e in root.find("edges") if e.get("partId") in {m.get("id") for m in members}}
        self.assertEqual(bones, {"RightPectoralFin"})

    def test_stitch_line_on_center_plane_is_nudged(self):
        root, _ = self._root({"parts": [{"name": "fin", "stitch": "lines", "lines": [
            [[0, 0, 0, 0.02], [0, 0.2, 0, 0.004]], [[0, 0, -0.1, 0.02], [0, 0.2, -0.1, 0.004]]]}]})
        ox = float(root.get("originX"))
        xs = {round(float(n.get("x")) - ox, 6) for n in root.find("nodes")}
        self.assertEqual(xs, {0.001})

    def test_stitch_loops_inherit_group_colour(self):
        root, _ = self._root({"parts": [{"name": "face", "stitch": "loops", "color": "#112233", "backClosed": True,
                                         "loops": [{"nodes": [[0, 0.3, 0, 0.01], [0.3, 0, 0, 0.01], [0, -0.3, 0, 0.01]]},
                                                   {"closed": True, "fillInterior": True, "color": "#00ff00",
                                                    "nodes": [[0.1, 0.1, 0.1, 0.01], [0.15, 0.1, 0.1, 0.01], [0.12, 0.15, 0.1, 0.01]]}]}]})
        group = [c for c in root.find("components")][0]
        colors = [c.get("color") for c in group.findall("component")]
        self.assertEqual(colors, ["#ff112233", "#ff00ff00"])
        fills = [p.get("fillLoopInterior") for p in root.find("parts")]
        self.assertIn("true", fills)

    def test_auto_order_moves_uncombined_parts_last(self):
        spec = {"parts": [{"name": "body", "nodes": [[0, 0, 0, 0.2]]},
                          {"name": "eye", "combine": "Uncombined", "nodes": [[0.1, 0.1, 0.1, 0.02]]},
                          {"name": "leg", "nodes": [[0.1, 0, 0, 0.05], [0.1, -0.3, 0, 0.04]]}]}
        root, _ = self._root(spec)
        self.assertEqual([c.get("name") for c in root.find("components")][:3], ["body", "leg", "model_uncombined"])
        root, _ = self._root(dict(spec, autoOrder=False))
        self.assertEqual([c.get("name") for c in root.find("components")][:3], ["body", "eye", "leg"])

    def test_auto_order_moves_uncombined_groups_last(self):
        spec = {"parts": [{"name": "body", "nodes": [[0, 0, 0, 0.2]]},
                          {"name": "wing", "stitch": "lines", "combine": "Uncombined", "mirror": True, "lines": [
                              {"nodes": [[0.1, 0, 0, 0.01], [0.4, 0, 0, 0.005]]}, {"nodes": [[0.1, 0, -0.1, 0.01], [0.4, 0, -0.2, 0.005]]}]},
                          {"name": "leg", "mirror": True, "nodes": [[0.1, 0, 0, 0.05], [0.1, -0.3, 0, 0.04]]}]}
        root, _ = self._root(spec)
        names = [c.get("name") for c in root.find("components")]
        self.assertEqual(names[:2], ["body", "leg"])
        self.assertEqual(names[2], "model_uncombined")

    def test_imported_mesh_is_embedded(self):
        with tempfile.TemporaryDirectory() as d:
            with open(os.path.join(d, "horn.glb"), "wb") as f:
                f.write(b"glTF-fake")
            root, assets = self._root({"parts": [{"name": "horn", "import": "horn.glb",
                                                  "nodes": [[0, 0, 0, 0.05], [0, 0.3, 0, 0.05]]}]}, d)
        part = root.find("parts")[0]
        self.assertEqual(part.get("target"), "ImportedModel")
        self.assertIn("models/%s.glb" % part.get("importedModelId"), assets)

    def test_nested_groups(self):
        root, _ = self._root({"parts": [{"name": "head", "combine": "Inversion", "group": [
            {"name": "a", "nodes": [[0, 0, 0, 0.1]]}, {"name": "inner", "group": [{"name": "b", "nodes": [[0, 0.1, 0, 0.1]]}]}]}]})
        head = root.find("components")[0]
        self.assertEqual(head.get("combineMode"), "Inversion")
        self.assertEqual([c.get("name") for c in head.findall("component")], ["a", "inner"])


class SeamTests(unittest.TestCase):
    LOG = ("noise\n"
           "SEAM_REPORT + tail 1 1,1,1,12,8,0,0.5,-0.47,0.145,20,8.17,10,0.262,0.052,0.071\n"
           "SEAM_REPORT + head 1 1,1,1,12,12,0,0.75,0.43,0.13,24,25.7,4,0.117,0.046,0.059\n"
           "SEAM_REPORT + fin 2 1,1,1,8,8,0,0,0,0.1,16,12,4,0.1,0.05,0.05 0,2,1,8,8,0,0,0,0.1,0,0,0,0,0.05,0.05\n"
           "SEAM_REPORT + floating 0\n")

    def test_parse_and_judge(self):
        from dust3d_agent import seams
        r = seams.seams_from_log(self.LOG)
        by = {}
        for x in r["seams"]:
            by.setdefault(x["part"], []).append(x)
        self.assertIn("fan", " ".join(by["tail"][0]["problems"]))
        self.assertIn("wide gap", " ".join(by["tail"][0]["problems"]))
        self.assertEqual(by["head"][0]["problems"], [])
        self.assertEqual(by["head"][0]["penalty"], 0.0)
        self.assertTrue(any("not bridged" in p for p in by["fin"][1]["problems"]))
        self.assertTrue(any("separate places" in p for p in by["fin"][0]["problems"]))
        self.assertIn("no seam", by["floating"][0]["problems"][0])
        self.assertGreater(r["total_penalty"], 10)

    def test_failed_combine_and_unbridged_scores(self):
        from dust3d_agent import seams
        r = seams.seams_from_log("SEAM_REPORT + horn failed\n"
                                 "SEAM_REPORT + belly 1 0,1,0,16,0,0,0.4,0,0.44,0,0,0,0,0.05,0\n")
        by = {x["part"]: x for x in r["seams"]}
        self.assertTrue(by["horn"]["failed"])
        self.assertEqual(by["horn"]["penalty"], seams.FAILED_PENALTY)
        self.assertIn("boolean failed", by["horn"]["problems"][0])
        # an unbridged join scores the flat penalty (a missing loop has edge length 0, no blow-up)
        self.assertEqual(by["belly"]["penalty"], 10.0)

    def test_tuner_locates_part_and_joint_node(self):
        from dust3d_agent import tune
        spec = {"parts": [{"name": "body", "nodes": [[0, 0.5, -0.3, 0.12], [0, 0.5, 0.3, 0.12]]},
                          {"name": "leg", "mirror": True, "nodes": [[0.06, 0.45, 0.2, 0.06], [0.09, 0.2, 0.2, 0.04], [0.09, 0.0, 0.2, 0.03]]}]}
        loc = tune._locate(spec, {"part": "leg~mirror", "center": [-0.08, 0.4, 0.2]})
        self.assertEqual(loc[0]["name"], "leg")
        self.assertEqual(loc[1], 0)


class GameReadyTests(unittest.TestCase):
    """Garments, equipment slots, posed clips (no Dust3D binary needed)."""

    BODY = {"name": "torso", "nodes": [[0, 0.4, 0, 0.1], [0, 0.6, 0, 0.12], [0, 0.8, 0, 0.08]],
            "bones": ["Spine", "Chest"], "cutFace": "Hexagon"}

    def test_wrap_follows_the_body_chain(self):
        sp = S.parse_spec({"name": "w", "parts": [self.BODY, {"shell": "torso", "name": "vest", "offset": 0.02,
                                                                   "range": [0.25, 1.0], "color": "#664422"}]})
        vest = next(p for p in sp.parts if p.name == "vest")
        self.assertEqual(vest.combine, "Uncombined")
        self.assertEqual(vest.cutFace, "Hexagon")
        self.assertEqual(len(vest.nodes), 3)                      # cut start, middle node, end
        self.assertAlmostEqual(vest.nodes[0][1], 0.5, places=5)  # 25% along the chain
        self.assertAlmostEqual(vest.nodes[0][3], 0.11 + 0.02, places=5)
        self.assertAlmostEqual(vest.nodes[-1][3], 0.08 + 0.02, places=5)
        self.assertEqual(vest.bones, ["Spine", "Chest"])
        with self.assertRaises(S.SpecError):
            S.parse_spec({"name": "w", "parts": [self.BODY, {"shell": "nope", "name": "x"}]})

    def test_slot_rides_on_the_component_name(self):
        sp = S.parse_spec({"name": "s", "parts": [self.BODY, {"shell": "torso", "name": "plate", "slot": "armor/3"},
                                                   {"name": "gear", "slot": "helmet/1", "group": [
                                                       {"name": "hat", "nodes": [[0, 0.9, 0, 0.05]]}]}]})
        self.assertEqual({p.name: p.slot for p in sp.parts}, {"torso": "", "plate": "armor/3", "hat": "helmet/1"})
        xml, _, _ = ds3.build_document(sp)
        names = [c.get("name") for c in ET.fromstring(xml).iter("component")]
        self.assertIn("plate @armor/3", names)
        self.assertEqual(ds3.split_component_name("plate @armor/3"), ("plate", "armor/3"))
        from dust3d_agent import decompile
        back, _ = decompile.decompile_xml(xml, "s")
        def walk(items):
            for e in items:
                yield e
                yield from walk(e.get("group", []))
        self.assertEqual({p["name"]: p.get("slot", "") for p in walk(back["parts"])}.get("plate"), "armor/3")
        with self.assertRaises(S.SpecError):
            S.parse_spec({"name": "s", "parts": [dict(self.BODY, slot="Armor 3")]})

    def test_pose_clips_are_not_dust3d_animations(self):
        sp = S.parse_spec({"name": "p", "rig": "Biped", "parts": [self.BODY], "animations": [
            "BipedIdle", {"type": "Pose", "name": "wave", "base": "idle", "durationSeconds": 1.0,
                          "keys": [{"t": 0, "pose": {}}, {"t": 0.5, "pose": {"LeftUpperArm": [0, 0, 120]}}, {"t": 1, "pose": {}}],
                          "events": {"wave": 0.5}}]})
        wave = sp.animations[1]
        self.assertEqual(S.animation_timing(wave), (1.0, 30))
        self.assertEqual(wave.events, {"wave": 0.5})
        xml, _, _ = ds3.build_document(sp)
        self.assertEqual([a.get("type") for a in ET.fromstring(xml).iter("animation")], ["BipedIdle"])


class WrapModifierTests(unittest.TestCase):
    """Wrap modifier groups: spec, document attributes, decompile (no Dust3D binary needed)."""

    BODY = [{"name": "torso", "nodes": [[0, 0.4, 0, 0.1], [0, 0.6, 0, 0.12], [0, 0.8, 0, 0.08]],
             "bones": ["Spine", "Chest"]},
            {"name": "bust", "mirror": True, "nodes": [[-0.04, 0.68, 0.05, 0.05], [-0.05, 0.67, 0.1, 0.03]],
             "bones": ["Chest"]}]

    def spec(self):
        return {"name": "sk", "rig": "Biped", "parts": [
            {"name": "body", "wrap": {"mode": "creature", "smoothness": 0.015, "faces": 900}, "group": self.BODY},
            {"name": "top", "slot": "chest/1", "combine": "Uncombined",
             "wrap": {"mode": "cloth", "keep": False, "bindTo": "body", "offset": 0.008, "drape": 0.6,
                      "drapeLength": 0.1, "thickness": 0.002, "faces": 500},
             "group": [{"shell": "torso", "name": "top_torso", "offset": 0, "combine": "Normal"},
                       {"shell": "bust", "name": "top_bust", "offset": 0, "combine": "Normal"},
                       {"shell": "torso", "name": "top_neck", "range": [0.8, 1.0], "offset": 0.01,
                        "combine": "Inversion"}]}]}

    def test_wrap_group_document_attributes(self):
        sp = S.parse_spec(self.spec())
        top = next(e for e in sp.elements if e.name == "top")
        self.assertEqual(top.wrap["mode"], "cloth")
        self.assertEqual(top.slot, "chest/1")
        # the slot stays on the garment (one surface), the guides do not get it
        self.assertTrue(all(not c.slot for c in top.children))
        xml, _, _ = ds3.build_document(sp)
        comps = {c.get("name"): c for c in ET.fromstring(xml).iter("component")}
        body, garment = comps["body"], comps["top @chest/1"]
        self.assertEqual(body.get("wrap"), "Skin")
        self.assertEqual(body.get("wrapFaces"), "900")
        self.assertEqual(garment.get("wrap"), "Cloth")
        self.assertEqual(garment.get("wrapKeep"), "false")
        self.assertEqual(garment.get("wrapBindTo"), body.get("id"))
        self.assertAlmostEqual(float(garment.get("wrapDrapeLength")), 0.1)

    def test_wrap_group_decompiles(self):
        from dust3d_agent import decompile
        xml, _, _ = ds3.build_document(S.parse_spec(self.spec()))
        back, _ = decompile.decompile_xml(xml, "sk")
        def walk(items):
            for e in items:
                if "group" in e:
                    yield e
                    yield from walk(e["group"])
        groups = {e["name"]: e for e in walk(back["parts"])}
        self.assertEqual(groups["body"]["wrap"]["mode"], "creature")
        top = groups["top"]
        self.assertEqual(top["slot"], "chest/1")
        self.assertEqual(top["wrap"]["bindTo"], "body")
        self.assertEqual(top["wrap"]["keep"], False)
        self.assertAlmostEqual(top["wrap"]["offset"], 0.008)

    def test_wrap_errors(self):
        bad = [{"mode": "fur"}, {"mode": "cloth", "drape": 2.0}, {"mode": "cloth", "openTop": 0.9},
               {"mode": "creature", "faces": 10}, {"mode": "cloth", "colour": 1}, {"mode": "cloth", "keep": 1}]
        for wrap in bad:
            with self.assertRaises(S.SpecError, msg=str(wrap)):
                S.parse_spec({"name": "x", "parts": [{"name": "g", "wrap": wrap, "group": self.BODY}]})
        with self.assertRaises(ValueError):
            sp = S.parse_spec({"name": "x", "parts": [{"name": "g", "wrap": {"mode": "cloth", "bindTo": "nobody"},
                                                        "group": self.BODY}]})
            ds3.build_document(sp)

    def test_wrap_pattern(self):
        """An animal coat on a creature wrap: document attributes, decompile, errors."""
        spec = self.spec()
        spec["parts"][0]["wrap"].update({"pattern": "rosettes", "patternColor": "#2a1c10", "patternScale": 0.05,
                                         "belly": 0.6})
        xml, _, _ = ds3.build_document(S.parse_spec(spec))
        body = {c.get("name"): c for c in ET.fromstring(xml).iter("component")}["body"]
        self.assertEqual(body.get("wrapPattern"), "Rosettes")
        self.assertEqual(body.get("wrapPatternColor"), "#ff2a1c10")
        self.assertAlmostEqual(float(body.get("wrapPatternScale")), 0.05)
        self.assertAlmostEqual(float(body.get("wrapBelly")), 0.6)
        from dust3d_agent import decompile
        back, _ = decompile.decompile_xml(xml, "sk")
        wrap = next(e for e in back["parts"] if e.get("name") == "body")["wrap"]
        self.assertEqual(wrap["pattern"], "rosettes")
        self.assertAlmostEqual(wrap["patternScale"], 0.05)
        self.assertAlmostEqual(wrap["belly"], 0.6)
        self.assertEqual(S.parse_spec(back).elements[0].wrap["patternColor"], "#ff2a1c10")
        for bad in [{"pattern": "plaid"}, {"pattern": "spots", "patternScale": 0}, {"belly": 1.5},
                    {"pattern": "spots", "patternColor": "dark"}]:
            with self.assertRaises(S.SpecError, msg=str(bad)):
                S.parse_spec({"name": "x", "parts": [{"name": "g", "wrap": dict({"mode": "creature"}, **bad),
                                                      "group": self.BODY}]})

    def test_wrap_wrinkles(self):
        """Folds and creases on a garment: document attributes, decompile, errors."""
        spec = self.spec()
        spec["parts"][1]["wrap"].update({"wrinkles": 0.7, "wrinkleSize": 1.5})
        xml, _, _ = ds3.build_document(S.parse_spec(spec))
        top = {c.get("name"): c for c in ET.fromstring(xml).iter("component")}["top @chest/1"]
        self.assertAlmostEqual(float(top.get("wrapWrinkles")), 0.7)
        self.assertAlmostEqual(float(top.get("wrapWrinkleSize")), 1.5)
        from dust3d_agent import decompile
        back, _ = decompile.decompile_xml(xml, "sk")
        def walk(items):
            for e in items:
                if "group" in e:
                    yield e
                    yield from walk(e["group"])
        wrap = next(e for e in walk(back["parts"]) if e.get("name") == "top")["wrap"]
        self.assertAlmostEqual(wrap["wrinkles"], 0.7)
        self.assertAlmostEqual(wrap["wrinkleSize"], 1.5)
        for bad in [{"wrinkles": 1.5}, {"wrinkles": 0.5, "wrinkleSize": 0}, {"wrinkles": "deep"}]:
            with self.assertRaises(S.SpecError, msg=str(bad)):
                S.parse_spec({"name": "x", "parts": [{"name": "g", "wrap": dict({"mode": "cloth"}, **bad),
                                                      "group": self.BODY}]})

    def test_old_key_names(self):
        """Specs written before the rename: a group's "skin" and a part's "wrap"."""
        new = self.spec()
        old = json.loads(json.dumps(new).replace('"shell": ', '"wrap": ').replace('"wrap": {', '"skin": {'))
        self.assertIn('"skin": {', json.dumps(old))
        a, b = S.parse_spec(new), S.parse_spec(old)
        self.assertEqual(ds3.build_document(a)[0], ds3.build_document(b)[0])
        with self.assertRaises(S.SpecError):
            S.parse_spec({"name": "x", "parts": [{"name": "g", "wrap": {"mode": "creature"}, "skin": {"mode": "creature"},
                                                  "group": self.BODY}]})
        with self.assertRaises(S.SpecError):
            S.parse_spec({"name": "x", "parts": self.BODY + [{"shell": "torso", "wrap": "torso", "name": "vest"}]})

    def test_override_across_names(self):
        """An override in the new name replaces the old key of a base spec, and the other way."""
        with tempfile.TemporaryDirectory() as d:
            base = {"name": "p", "parts": [{"name": "body", "skin": {"mode": "creature"}, "group": self.BODY},
                                           {"wrap": "torso", "name": "vest", "offset": 0.01}]}
            json.dump(base, open(os.path.join(d, "p.json"), "w"))
            variant = {"extends": "p.json", "name": "q",
                       "override": {"body": {"wrap": {"mode": "creature", "faces": 700}},
                                    "vest": {"shell": "torso", "offset": 0.02}}}
            json.dump(variant, open(os.path.join(d, "q.json"), "w"))
            sp = S.load_spec(os.path.join(d, "q.json"))
        body = next(e for e in sp.elements if e.name == "body")
        self.assertEqual(body.wrap["faces"], 700)
        vest = next(p for p in sp.parts if p.name == "vest")
        self.assertAlmostEqual(vest.nodes[0][3], 0.1 + 0.02)


@unittest.skipUnless(_have_dust3d(), "Dust3D binary not available")
class IntegrationTests(unittest.TestCase):
    def test_wrap_modifier_outfit(self):
        """A creature skin and a garment bound to it: one watertight body, four-bone weights, a slot."""
        from dust3d_agent.__main__ import main
        from dust3d_agent import glb
        import numpy as np
        spec = WrapModifierTests().spec()
        spec["parts"][0]["group"] = spec["parts"][0]["group"] + [
            {"name": "leg", "mirror": True, "nodes": [[-0.05, 0.42, 0, 0.06], [-0.06, 0.2, 0, 0.045],
                                                      [-0.06, 0.02, 0, 0.035]],
             "bones": ["LeftUpperLeg", "LeftLowerLeg"]}]
        spec["animations"] = ["BipedWalk"]
        with tempfile.TemporaryDirectory() as d:
            path = os.path.join(d, "sk.json")
            json.dump(spec, open(path, "w"))
            self.assertEqual(main(["build", path, "-o", d, "--no-render"]), 0)
            rep = json.load(open(os.path.join(d, "sk_report.json")))
            self.assertEqual(rep["metrics"]["nonmanifold_edges"], 0)
            self.assertIn("1", rep["game"]["slots"]["slots"]["chest"])
            g = glb.load(os.path.join(d, "sk.glb"))
            body = next(p for p in g.primitives if p.mesh_name in ("", "body"))
            top = next(p for p in g.primitives if p.mesh_name == "slot_chest_1")
            # the body is one closed surface: every welded edge is shared by two triangles
            key = np.round(body.positions / 1e-5).astype(np.int64)
            _, inverse = np.unique(key, axis=0, return_inverse=True)
            tri = inverse.reshape(-1)[body.indices.reshape(-1, 3)]
            edges = np.sort(np.concatenate([tri[:, [0, 1]], tri[:, [1, 2]], tri[:, [2, 0]]]), axis=1)
            _, counts = np.unique(edges, axis=0, return_counts=True)
            self.assertTrue((counts == 2).all())
            # blended joints use more than two bones somewhere, and weights sum to one
            self.assertTrue(((body.weights > 0.01).sum(1) > 2).any())
            self.assertTrue(np.allclose(body.weights.sum(1), 1.0, atol=1e-3))
            self.assertTrue(np.allclose(top.weights.sum(1), 1.0, atol=1e-3))
            # the hem: the garment has open rims (a neckline cut by the Inversion guide)
            key = np.round(top.positions / 1e-5).astype(np.int64)
            _, inverse = np.unique(key, axis=0, return_inverse=True)
            tri = inverse.reshape(-1)[top.indices.reshape(-1, 3)]
            edges = np.sort(np.concatenate([tri[:, [0, 1]], tri[:, [1, 2]], tri[:, [2, 0]]]), axis=1)
            _, counts = np.unique(edges, axis=0, return_counts=True)
            self.assertTrue((counts == 1).any())

    def test_build_examples(self):
        from dust3d_agent.__main__ import main
        for f in sorted(glob.glob(os.path.join(EXAMPLES, "*.json"))):
            with tempfile.TemporaryDirectory() as d:
                rc = main(["build", f, "-o", d, "--no-render"])
                self.assertEqual(rc, 0, f)
                name = S.load_spec(f).name
                rep = json.load(open(os.path.join(d, name + "_report.json")))
                m = rep["metrics"]
                self.assertGreater(m["triangles"], 50, f)
                self.assertEqual(m.get("unweighted_vertices", 0), 0, f)
                sp = S.load_spec(f)
                self.assertEqual(len(m["animations"]), len(sp.animations), f)

    def test_hard_surface_shapes_build_clean(self):
        from dust3d_agent.__main__ import main
        spec = {"name": "hs", "parts": [
            {"name": "hull", "group": [
                {"shape": "box", "name": "body", "center": [0, 0.3, 0], "size": [0.8, 0.5, 0.6], "axis": "x", "bevel": 0.04},
                {"shape": "box", "name": "nose", "center": [0.45, 0.3, 0], "size": [0.3, 0.3, 0.4], "axis": "x", "taper": [0.5, 1.0]},
                {"shape": "groove", "name": "seam", "from": [-0.1, 0.1, 0.3], "to": [-0.1, 0.5, 0.3], "normal": [0, 0, 1], "width": 0.015}]},
            {"shape": "beam", "name": "leg", "path": [[0.25, 0.1, 0.25], [0.35, 0.0, 0.4]], "profile": "I", "width": 0.06, "height": 0.08},
            {"shape": "beam", "name": "strut", "path": [[-0.25, 0.1, 0.25], [-0.35, 0.0, 0.4]], "profile": "T", "width": 0.06, "height": 0.08},
            {"shape": "bolts", "name": "bolt", "points": [[0, 0.55, 0.1], [0.1, 0.55, 0.1]], "normal": [0, 1, 0], "radius": 0.012}]}
        with tempfile.TemporaryDirectory() as d:
            path = os.path.join(d, "hs.json")
            json.dump(spec, open(path, "w"))
            self.assertEqual(main(["build", path, "-o", d, "--no-render"]), 0)
            rep = json.load(open(os.path.join(d, "hs_report.json")))
            m = rep["metrics"]
            self.assertEqual(m["open_edges"], 0)          # concave (I, T) caps triangulate
            self.assertEqual(m["nonmanifold_edges"], 0)
            self.assertEqual(rep["seams"]["total_penalty"], 0.0)   # hard joins: plain booleans
            self.assertAlmostEqual(m["bbox_max"][1], 0.55 + 0.012 * 0.8, delta=0.005)   # bolt tops

    def test_game_ready_export(self):
        """Materials, part labels, equipment slots, posed clips, events and budget, end to end."""
        from dust3d_agent.__main__ import main
        from dust3d_agent import glb
        spec = {"extends": os.path.join(EXAMPLES, "goblin.json"), "name": "kit", "smoothWeights": 2, "budget": 20000,
                "add": [
                    {"shell": "torso", "name": "vest", "offset": 0.02, "color": "#664422", "slot": "armor/1"},
                    # the same shape again as another variant: overlapping variants must stay apart
                    {"shell": "torso", "name": "mail", "offset": 0.02, "color": "#888888", "metallic": 0.8,
                     "roughness": 0.4, "slot": "armor/2"},
                    {"name": "lamp", "nodes": [[0, 1.2, 0.2, 0.03]], "color": "#6ff3ff", "emissive": 1.5,
                     "combine": "Uncombined"}],
                "animations": ["BipedWalk", "BipedIdle", {"type": "BipedSlam", "name": "slam"},
                               {"type": "Pose", "name": "cheer", "base": "idle", "durationSeconds": 1.0,
                                "keys": [{"t": 0, "pose": {}}, {"t": 0.5, "pose": {"LeftUpperArm": [0, 0, 140]}},
                                         {"t": 1.0, "pose": {}}]}]}
        with tempfile.TemporaryDirectory() as d:
            path = os.path.join(d, "kit.json")
            json.dump(spec, open(path, "w"))
            self.assertEqual(main(["build", path, "-o", d, "--no-render"]), 0)
            rep = json.load(open(os.path.join(d, "kit_report.json")))
            game = rep["game"]
            self.assertEqual(sorted(game["slots"]["slots"]["armor"]), ["1", "2"])
            self.assertEqual(game["slots"]["slots"]["armor"]["1"], game["slots"]["slots"]["armor"]["2"])
            self.assertTrue(game["budget"]["ok"])
            self.assertGreater(game["weights"]["vertices_changed"], 0)
            self.assertEqual(game["pose_clips"], ["cheer"])
            self.assertEqual(rep["metrics"]["nonmanifold_edges"], 0)
            g = glb.load(os.path.join(d, "kit.glb"))
            mat = g.json["materials"][0]
            self.assertIn("emissiveTexture", mat)
            self.assertIn("metallicRoughnessTexture", mat["pbrMetallicRoughness"])
            names = {p.mesh_name for p in g.primitives}
            self.assertTrue({"slot_armor_1", "slot_armor_2"} <= names)
            parts = [p["name"] for p in g.json["meshes"][0]["extras"]["dust3dParts"]]
            self.assertIn("lamp", parts)
            self.assertNotIn("", parts)                          # every triangle has its part
            clips = {c["name"]: c for c in json.load(open(os.path.join(d, "kit_clips.json")))["clips"]}
            self.assertIn("cheer", clips)
            self.assertEqual({e["name"] for e in clips["slam"]["events"]}, {"hit"})
            self.assertGreaterEqual(sum(e["name"] == "step" for e in clips["walk"]["events"]), 2)

    def test_clip_manifest_and_timing(self):
        from dust3d_agent.__main__ import main
        from dust3d_agent import glb
        with tempfile.TemporaryDirectory() as d:
            self.assertEqual(main(["build", os.path.join(EXAMPLES, "goblin.json"), "-o", d, "--no-render"]), 0)
            clips = {c["name"]: c for c in json.load(open(os.path.join(d, "goblin_clips.json")))["clips"]}
            self.assertTrue(clips["walk"]["loop"])
            self.assertFalse(clips["slam"]["loop"])
            g = glb.load(os.path.join(d, "goblin.glb"))
            for a in g.animations:
                c = clips[a["name"]]
                n, dur = c["frameCount"], c["durationSeconds"]
                self.assertAlmostEqual(a["duration"], dur * (n - 1) / n, places=3)

    def test_spider_attack(self):
        from dust3d_agent.__main__ import main
        spider = {"name": "sp", "rig": "Spider", "parts": [
            {"name": "body", "nodes": [[0, 0.3, 0.3, 0.1], [0, 0.3, 0.0, 0.12], [0, 0.3, -0.3, 0.14]],
             "bones": ["Cephalothorax", "Abdomen"]},
            {"name": "head", "nodes": [[0, 0.3, 0.35, 0.08], [0, 0.3, 0.45, 0.06]], "bones": "Head"}] + [
            {"name": "leg%d" % i, "mirror": True,
             "nodes": [[0.08, 0.3, z, 0.03], [0.25, 0.35, z, 0.025], [0.4, 0.15, z, 0.02], [0.5, 0.0, z, 0.01]],
             "bones": [pre + "Coxa", pre + "Femur", pre + "Tibia"]}
            for i, (pre, z) in enumerate([("FrontLeft", 0.3), ("MidFrontLeft", 0.2), ("MidBackLeft", 0.1), ("BackLeft", 0.0)])],
            "animations": [{"type": "SpiderAttack", "name": "attack"}]}
        with tempfile.TemporaryDirectory() as d:
            p = os.path.join(d, "sp.json")
            json.dump(spider, open(p, "w"))
            self.assertEqual(main(["build", p, "-o", d, "--no-render"]), 0)
            rep = json.load(open(os.path.join(d, "sp_report.json")))
            anim = rep["metrics"]["animations"][0]
            self.assertEqual(anim["name"], "attack")
            self.assertGreater(anim["max_vertex_motion_rel"], 0.1)

    def _build_and_check_clips(self, spec):
        from dust3d_agent import glb
        from dust3d_agent.__main__ import main
        with tempfile.TemporaryDirectory() as d:
            p = os.path.join(d, spec["name"] + ".json")
            json.dump(spec, open(p, "w"))
            self.assertEqual(main(["build", p, "-o", d, "--no-render"]), 0)
            rep = json.load(open(os.path.join(d, spec["name"] + "_report.json")))
            m = rep["metrics"]
            clip_warnings = [w for w in m["warnings"] if "animation" in w]
            self.assertEqual(clip_warnings, [])
            anims = {a["name"]: a for a in m["animations"]}
            self.assertEqual(set(anims), {a["name"] for a in spec["animations"]})
            for name, a in anims.items():
                self.assertGreater(a["max_vertex_motion_rel"], 0.005, name)
            # the death ends lying on the ground: nothing below it, nothing left standing
            g = glb.load(os.path.join(d, spec["name"] + ".glb"))
            from dust3d_agent import render
            import numpy as np
            rest = np.concatenate([q for q, _ in render._gather(g, None)])
            die = next(a for a in g.animations if a["name"] == "die")
            end = np.concatenate([q for q, _ in render._gather(g, die, die["duration"])])
            height = np.ptp(rest[:, 1])
            self.assertGreater(end[:, 1].min(), rest[:, 1].min() - 0.08 * height)
            self.assertLess(end[:, 1].max(), rest[:, 1].max() - 0.1 * height)
            return anims

    def test_spider_game_clips(self):
        spider = {"name": "sp2", "rig": "Spider", "parts": [
            {"name": "body", "nodes": [[0, 0.3, 0.3, 0.1], [0, 0.3, 0.0, 0.12], [0, 0.3, -0.3, 0.14]],
             "bones": ["Cephalothorax", "Abdomen"]},
            {"name": "head", "nodes": [[0, 0.3, 0.35, 0.08], [0, 0.3, 0.45, 0.06]], "bones": "Head"}] + [
            {"name": "leg%d" % i, "mirror": True,
             "nodes": [[0.08, 0.3, z, 0.03], [0.25, 0.35, z, 0.025], [0.4, 0.15, z, 0.02], [0.5, 0.0, z, 0.01]],
             "bones": [pre + "Coxa", pre + "Femur", pre + "Tibia"]}
            for i, (pre, z) in enumerate([("FrontLeft", 0.3), ("MidFrontLeft", 0.2), ("MidBackLeft", 0.1), ("BackLeft", 0.0)])],
            "animations": [{"type": "SpiderIdle", "name": "idle"}, {"type": "SpiderWalk", "name": "walk"},
                           {"type": "SpiderRun", "name": "run"}, {"type": "SpiderAttack", "name": "attack"},
                           {"type": "SpiderHurt", "name": "hurt"}, {"type": "SpiderDie", "name": "die"}]}
        self._build_and_check_clips(spider)

    def test_wingless_insect_game_clips(self):
        leg = lambda pre, z, x: {"name": pre.lower(), "mirror": True,
                                 "nodes": [[0.04, 0.3, z, 0.035], [0.1, 0.31, z, 0.03], [x * 0.6, 0.42, z, 0.024], [x, 0.02, z, 0.013]],
                                 "bones": [pre + "Coxa", pre + "Femur", pre + "Tibia"]}
        ant = {"name": "ant", "rig": "Insect", "parts": [
            {"name": "head", "nodes": [[0, 0.36, 0.43, 0.08], [0, 0.37, 0.55, 0.1], [0, 0.34, 0.66, 0.06]], "bones": "Head"},
            {"name": "thorax", "nodes": [[0, 0.34, 0.41, 0.07], [0, 0.36, 0.28, 0.08], [0, 0.3, 0.13, 0.05]], "bones": "Thorax"},
            {"name": "gaster", "nodes": [[0, 0.28, 0.11, 0.05], [0, 0.31, -0.08, 0.14], [0, 0.26, -0.36, 0.07]], "bones": "Abdomen"},
            leg("FrontLeft", 0.38, 0.34), leg("MiddleLeft", 0.28, 0.5), leg("BackLeft", 0.18, 0.48)],
            "animations": [{"type": "InsectIdle", "name": "idle"}, {"type": "InsectWalk", "name": "walk"},
                           {"type": "InsectBite", "name": "attack"}, {"type": "InsectHurt", "name": "hurt"},
                           {"type": "InsectDie", "name": "die", "params": {"flipOver": 1}}]}
        self._build_and_check_clips(ant)

    def test_rig_templates_come_from_binary(self):
        types = S.rig_types()
        self.assertEqual(sorted(types), sorted(S.ANIMATION_TYPES))
        quad = S.load_rig_template("Quadruped")
        self.assertIn("FrontLeftUpperLeg", quad["bones"])
        self.assertEqual(quad["bones"]["Pelvis"]["parent"], "Root")
        with self.assertRaises(S.SpecError):
            S.load_rig_template("Dragon")

    def test_engine_reports_seams(self):
        from dust3d_agent import seams
        with tempfile.TemporaryDirectory() as d:
            p = os.path.join(d, "fox.ds3")
            ds3.compile_to_ds3(S.load_spec(os.path.join(EXAMPLES, "fox.json")), p)
            r = seams.analyze_seams(p, d)
        self.assertTrue(r["reports_found"], "Dust3D build lacks SEAM_REPORT support")
        parts = {x["part"] for x in r["seams"]}
        self.assertTrue({"tail", "front_leg", "front_leg~mirror"} <= parts)

    @unittest.skipUnless(os.environ.get("DUST3D_TEST_MODELS"), "DUST3D_TEST_MODELS not set")
    def test_reference_models_roundtrip(self):
        """Decompile -> compile -> export must match the direct export for every reference model."""
        from dust3d_agent import decompile, export, metrics
        root = os.environ["DUST3D_TEST_MODELS"]
        files = sorted(glob.glob(os.path.join(root, "*", "*.ds3")))
        self.assertTrue(files)
        for f in files:
            with tempfile.TemporaryDirectory() as d:
                spec_dict, warnings = decompile.decompile_file(f, asset_dir=d)
                self.assertEqual(warnings, [], f)
                ref = os.path.join(d, "ref.glb")
                self.assertTrue(export.export(f, [ref])["ok"], f)
                p = os.path.join(d, "rt.ds3")
                ds3.compile_to_ds3(S.parse_spec(spec_dict, d), p)
                rt = os.path.join(d, "rt.glb")
                self.assertTrue(export.export(p, [rt])["ok"], f)
                a, b = metrics.analyze(ref), metrics.analyze(rt)
                self.assertEqual(a["triangles"], b["triangles"], f)
                self.assertEqual(a["size_xyz"], b["size_xyz"], f)
                self.assertEqual(len(a["animations"]), len(b["animations"]), f)

if __name__ == "__main__":
    unittest.main()
