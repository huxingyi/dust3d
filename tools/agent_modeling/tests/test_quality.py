import os
import sys
import unittest
import json
import tempfile
from unittest.mock import patch

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from dust3d_agent import quality, export


def have_dust3d():
    try:
        export.find_dust3d()
        return True
    except FileNotFoundError:
        return False


@unittest.skipUnless(have_dust3d(), "Dust3D binary not available")
class BuildGateTests(unittest.TestCase):
    def test_slam_preserves_modular_bind_offsets_at_recovery(self):
        from dust3d_agent import ds3, spec, glb, render
        import numpy as np
        parts = [{"shape": "box", "name": name, "center": center,
                  "size": size, "bones": bone, "combine": "Uncombined"}
                 for name, center, size, bone in [
                    ("hips", [0,.44,0], [.18,.10,.14], "Hips"),
                    ("spine", [0,.54,0], [.08,.16,.08], "Spine"),
                    ("chest", [0,.68,0], [.27,.25,.16], "Chest"),
                    ("neck", [0,.82,0], [.065,.075,.07], "Neck"),
                    ("head", [0,.89,.015], [.12,.11,.13], "Head")]]
        for bone, start, end in [
            ("LeftShoulder", [.11,.75,0], [.17,.75,0]),
            ("LeftUpperArm", [.17,.75,0], [.23,.57,0]),
            ("LeftLowerArm", [.23,.57,0], [.27,.40,.025]),
            ("LeftHand", [.27,.40,.025], [.28,.34,.04]),
            ("LeftUpperLeg", [.075,.44,0], [.095,.25,.015]),
            ("LeftLowerLeg", [.095,.25,.015], [.095,.06,0]),
            ("LeftFoot", [.095,.045,0], [.095,.035,.10])]:
            parts.append({"shape":"cylinder", "name":bone, "from":start,"to":end,
                          "radius":.025,"bones":bone,"mirror":True,"combine":"Uncombined"})
        with tempfile.TemporaryDirectory() as folder:
            model, output = os.path.join(folder,"robot.ds3"), os.path.join(folder,"robot.glb")
            ds3.compile_to_ds3(spec.parse_spec({"name":"robot","rig":"Biped","parts":parts,
                "animations":[{"type":"BipedSlam","name":"attack","params":{"armPosture":2}}]}), model)
            self.assertTrue(export.export(model,[output])["ok"])
            g = glb.load(output)
            a = g.animations[0]
            rest = np.concatenate([p for p,_ in render._gather(g,None)])
            end = np.concatenate([p for p,_ in render._gather(g,a,a["duration"])])
            self.assertLess(float(np.linalg.norm(end-rest,axis=1).max()), .002)

    def test_strict_budget_failure_keeps_reviewable_outputs(self):
        from dust3d_agent.__main__ import main
        with tempfile.TemporaryDirectory() as folder:
            path = os.path.join(folder, "cube.json")
            with open(path, "w") as f:
                json.dump({"name": "cube", "budget": 1, "parts": [
                    {"shape": "box", "name": "case", "size": [1, 1, 1]}]}, f)
            self.assertEqual(main(["build", path, "-o", folder, "--strict", "--no-render"]), 1)
            with open(os.path.join(folder, "cube_report.json")) as f:
                report = json.load(f)
            self.assertTrue(report["export"]["ok"])
            self.assertIn("triangle budget exceeded", report["quality"]["failures"])
            self.assertTrue(os.path.isfile(os.path.join(folder, "cube.glb")))

    def test_animated_fbx_finishes_before_batch_exit(self):
        from dust3d_agent import ds3, spec
        with tempfile.TemporaryDirectory() as folder:
            source = os.path.join(os.path.dirname(os.path.dirname(__file__)), "examples", "goblin.json")
            model = os.path.join(folder, "animated.ds3")
            ds3.compile_to_ds3(spec.load_spec(source), model)
            outputs = [os.path.join(folder, "animated." + ext) for ext in ("glb", "fbx", "obj")]
            result = export.export(model, outputs)
            self.assertTrue(result["ok"], result)
            with open(outputs[1], "rb") as f:
                self.assertEqual(f.read(22), b"Kaydara FBX Binary  \x00\x1a")


class QualityTests(unittest.TestCase):
    def test_one_shot_preview_includes_recovery_endpoint(self):
        from dust3d_agent.render import preview_times
        self.assertEqual(preview_times({"duration":1,"loop":False},6)[-1],1)
        self.assertLess(preview_times({"duration":1,"loop":True},6)[-1],1)
        self.assertLess(preview_times({"duration":1},6)[-1],1)

    def test_gate_preserves_all_defects(self):
        result = quality.evaluate({"export": {"ok": True},
            "lint": ["info: separate piece", "bad bone"],
            "metrics": {"triangles": 12, "warnings": ["open boundary"]},
            "seams": {"bad": [{"part": "arm", "problems": ["failed boolean"]}]},
            "game": {"budget": {"ok": False}}})
        self.assertFalse(result["ok"])
        self.assertEqual(len(result["failures"]), 4)

    def test_missing_geometry_cannot_pass(self):
        self.assertFalse(quality.evaluate({"export": {"ok": True}})["ok"])

    def test_clean_asset_passes(self):
        self.assertTrue(quality.evaluate({"export": {"ok": True},
            "metrics": {"triangles": 12}, "lint": ["advisory: check silhouette"]})["ok"])

    def test_macos_batch_platform_and_user_override(self):
        with patch.object(export.platform, "system", return_value="Darwin"), \
             patch.object(export.subprocess, "run") as run, \
             patch.object(export.os.path, "exists", return_value=False):
            run.return_value.stdout = run.return_value.stderr = b""
            run.return_value.returncode = 0
            with patch.dict(os.environ, {}, clear=True):
                export.export("model.ds3", ["model.glb"], dust3d="dust3d")
                self.assertEqual(run.call_args.kwargs["env"]["QT_QPA_PLATFORM"], "offscreen")
            with patch.dict(os.environ, {"QT_QPA_PLATFORM": "cocoa"}, clear=True):
                export.export("model.ds3", ["model.glb"], dust3d="dust3d")
                self.assertEqual(run.call_args.kwargs["env"]["QT_QPA_PLATFORM"], "cocoa")
