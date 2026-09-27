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
            n1 = len(ET.fromstring(xml).find("nodes"))
            n2 = len(ET.fromstring(xml2).find("nodes"))
            self.assertEqual(n1, n2, f)


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

    def test_tuner_locates_part_and_joint_node(self):
        from dust3d_agent import tune
        spec = {"parts": [{"name": "body", "nodes": [[0, 0.5, -0.3, 0.12], [0, 0.5, 0.3, 0.12]]},
                          {"name": "leg", "mirror": True, "nodes": [[0.06, 0.45, 0.2, 0.06], [0.09, 0.2, 0.2, 0.04], [0.09, 0.0, 0.2, 0.03]]}]}
        loc = tune._locate(spec, {"part": "leg~mirror", "center": [-0.08, 0.4, 0.2]})
        self.assertEqual(loc[0]["name"], "leg")
        self.assertEqual(loc[1], 0)


@unittest.skipUnless(_have_dust3d(), "Dust3D binary not available")
class IntegrationTests(unittest.TestCase):
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
