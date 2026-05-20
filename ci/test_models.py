#!/usr/bin/env python3
"""
Cross-platform test script for dust3d.
Downloads .ds3 test models, exports .glb files, and generates a self-contained
HTML report that uses three.js (inlined) to render and play back all animations.

Requirements: Python 3 (stdlib only, no pip packages needed)

Usage:
    python test_models.py --dust3d /path/to/dust3d [--output-dir test_output]

For headless CI (Linux), run with xvfb:
    xvfb-run python test_models.py --dust3d ./dust3d
"""

import argparse
import base64
import json
import os
import platform
import shutil
import struct
import subprocess
import sys
import urllib.request
from pathlib import Path


REPO_API_URL = "https://api.github.com/repos/huxingyi/dust3d-test-models/git/trees/main?recursive=1"
RAW_BASE_URL = "https://raw.githubusercontent.com/huxingyi/dust3d-test-models/main"
THREE_JS_URL = "https://cdn.jsdelivr.net/npm/three@0.135.0/build/three.min.js"
GLTF_LOADER_URL = "https://cdn.jsdelivr.net/npm/three@0.135.0/examples/js/loaders/GLTFLoader.js"


def fetch_text(url):
    req = urllib.request.Request(url, headers={"User-Agent": "dust3d-test"})
    with urllib.request.urlopen(req, timeout=30) as resp:
        return resp.read().decode("utf-8")


def discover_ds3_files():
    print("Discovering .ds3 files from GitHub...")
    headers = {"User-Agent": "dust3d-test"}
    token = os.environ.get("GITHUB_TOKEN")
    if token:
        headers["Authorization"] = f"token {token}"
    try:
        req = urllib.request.Request(REPO_API_URL, headers=headers)
        with urllib.request.urlopen(req, timeout=30) as resp:
            data = json.loads(resp.read().decode())
        return [entry["path"] for entry in data["tree"] if entry["path"].endswith(".ds3")]
    except Exception as e:
        raise RuntimeError(f"Failed to fetch test model list: {type(e).__name__}: {e}") from None


def download_file(url, dest):
    dest.parent.mkdir(parents=True, exist_ok=True)
    req = urllib.request.Request(url, headers={"User-Agent": "dust3d-test"})
    with urllib.request.urlopen(req, timeout=60) as resp, open(dest, "wb") as f:
        shutil.copyfileobj(resp, f)


def export_glb(dust3d_bin, ds3_path, glb_path, timeout=120):
    env = os.environ.copy()
    if platform.system() == "Linux":
        env.setdefault("QT_QPA_PLATFORM", "offscreen")

    cmd = [str(dust3d_bin), str(ds3_path), "-o", str(glb_path)]
    print(f"  Running: {' '.join(cmd)}")
    result = subprocess.run(cmd, env=env, timeout=timeout, capture_output=True)
    if result.returncode != 0:
        print(f"  FAILED (exit {result.returncode})")
        if result.stderr:
            print(f"  stderr: {result.stderr.decode(errors='replace')[:500]}")
        return False
    return glb_path.exists()


def parse_glb_animations(glb_path):
    with open(glb_path, "rb") as f:
        magic, version, length = struct.unpack("<III", f.read(12))
        if magic != 0x46546C67:
            return []
        chunk_length, chunk_type = struct.unpack("<II", f.read(8))
        json_bytes = f.read(chunk_length)

    gltf = json.loads(json_bytes)
    animations = gltf.get("animations", [])
    result = []
    accessors = gltf.get("accessors", [])

    for i, anim in enumerate(animations):
        name = anim.get("name", f"animation_{i}")
        max_time = 0.0
        for sampler in anim.get("samplers", []):
            input_acc = accessors[sampler["input"]]
            max_time = max(max_time, input_acc.get("max", [0])[0])
        result.append({"name": name, "duration": max_time, "index": i})
    return result


def generate_html_report(results, output_dir, glb_dir):
    html_path = output_dir / "report.html"

    total = len(results)
    passed = sum(1 for r in results if r["success"])
    failed = total - passed

    print("Downloading three.js for embedding...")
    three_js = fetch_text(THREE_JS_URL)
    gltf_loader_js = fetch_text(GLTF_LOADER_URL)

    models_js = []
    for r in results:
        entry = {
            "name": r["model"],
            "success": r["success"],
            "error": r.get("error", ""),
            "animations": r.get("animations", []),
            "glbData": None,
        }
        if r["success"]:
            glb_path = glb_dir / f"{Path(r['model']).stem}.glb"
            if glb_path.exists():
                entry["glbData"] = base64.b64encode(glb_path.read_bytes()).decode("ascii")
        models_js.append(entry)

    models_json = json.dumps(models_js)

    html = (
        '<!DOCTYPE html>\n'
        '<html lang="en">\n'
        '<head>\n'
        '<meta charset="UTF-8">\n'
        '<meta name="viewport" content="width=device-width, initial-scale=1.0">\n'
        '<title>Dust3D Test Models Report</title>\n'
        '<link rel="preconnect" href="https://fonts.googleapis.com">\n'
        '<link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>\n'
        '<link href="https://fonts.googleapis.com/css2?family=Inter:wght@300;400;500;600;700&display=swap" rel="stylesheet">\n'
        '<style>\n'
        ':root {\n'
        '  --bg: #ffffff; --bg-alt: #f5f3ef; --text: #1a1a1a;\n'
        '  --text-muted: #6b6860; --accent: #aaebc4; --border: #e0ddd8;\n'
        '  --pass: #2e7d32; --fail: #c62828;\n'
        '}\n'
        '*, *::before, *::after { box-sizing: border-box; }\n'
        'body { font-family: "Inter", "Helvetica Neue", Arial, sans-serif; font-size: 14px;\n'
        '       font-weight: 400; color: var(--text); background: var(--bg);\n'
        '       margin: 0; padding: 0; line-height: 1.5; }\n'
        '.topbar { background: #1a1a1a; padding: 0 32px; height: 28px;\n'
        '          display: flex; align-items: center; }\n'
        '.topbar span { font-size: 12px; font-weight: 600; color: #fff; letter-spacing: 0.04em; }\n'
        '.page { max-width: 960px; margin: 0 auto; padding: 48px 32px 64px; }\n'
        'h1 { font-size: 26px; font-weight: 300; margin: 0 0 8px 0; }\n'
        'h1 strong { font-weight: 600; }\n'
        '.subtitle { font-size: 14px; color: var(--text-muted); margin: 0 0 32px 0; }\n'
        '.summary { display: flex; gap: 24px; margin: 0 0 32px 0;\n'
        '           font-size: 12px; color: var(--text-muted); }\n'
        '.summary span { font-weight: 600; color: var(--text); }\n'
        '.summary .pass { color: var(--pass); }\n'
        '.summary .fail { color: var(--fail); }\n'
        '.model-section { margin-bottom: 40px; }\n'
        '.model-title { font-size: 14px; font-weight: 600; color: var(--text);\n'
        '               margin: 0 0 2px 0; display: flex; align-items: center; gap: 8px; }\n'
        '.badge { font-size: 10px; font-weight: 700; letter-spacing: 0.05em;\n'
        '         text-transform: uppercase; padding: 1px 6px; border-radius: 3px; }\n'
        '.badge.success { background: var(--accent); color: #1a1a1a; }\n'
        '.badge.failure { background: var(--fail); color: #fff; }\n'
        '.model-meta { font-size: 12px; color: var(--text-muted); margin: 0 0 10px 0; }\n'
        '.anim-grid { display: grid; grid-template-columns: repeat(auto-fill, minmax(200px, 1fr)); gap: 12px; }\n'
        '.anim-card { border: 1px solid var(--border); border-radius: 6px; overflow: hidden; }\n'
        '.anim-card canvas { display: block; width: 100%; height: auto; background: var(--bg-alt); }\n'
        '.anim-info { padding: 6px 10px; }\n'
        '.anim-name { font-size: 12px; font-weight: 600; color: var(--text);\n'
        '             letter-spacing: 0.03em; text-transform: uppercase; margin: 0; }\n'
        '.anim-duration { font-size: 11px; color: var(--text-muted); margin: 0; }\n'
        '.no-anim { font-size: 12px; color: var(--text-muted); }\n'
        '.error-msg { font-size: 12px; color: var(--fail); }\n'
        'hr { border: none; border-top: 1px solid var(--border); margin: 0 0 40px 0; }\n'
        'footer { max-width: 960px; margin: 0 auto; padding: 16px 32px;\n'
        '         border-top: 1px solid var(--border); font-size: 11px; color: var(--text-muted); }\n'
        '</style>\n'
        '</head>\n'
        '<body>\n'
        '<div class="topbar"><span>DUST3D</span></div>\n'
        '<div class="page">\n'
        '<h1>Test Models <strong>Report</strong></h1>\n'
        f'<div class="summary"><span class="pass">{passed}</span> passed &middot; <span class="fail">{failed}</span> failed &middot; <span>{total}</span> total</div>\n'
        '<div id="cards"></div>\n'
        '</div>\n'
        '<footer>Generated by dust3d test suite</footer>\n'
        '\n'
        '<script>\n'
        + three_js + '\n'
        '</script>\n'
        '<script>\n'
        + gltf_loader_js + '\n'
        '</script>\n'
        '<script>\n'
        'var MODELS = ' + models_json + ';\n'
        'var cardsEl = document.getElementById("cards");\n'
        'var allViewers = [];\n'
        '\n'
        'var sharedRenderer = new THREE.WebGLRenderer({ antialias: true });\n'
        'sharedRenderer.setSize(320, 240, false);\n'
        'sharedRenderer.setClearColor(0xf5f3ef);\n'
        'sharedRenderer.setPixelRatio(1);\n'
        '\n'
        'MODELS.forEach(function(model) {\n'
        '    var section = document.createElement("div");\n'
        '    section.className = "model-section";\n'
        '    var title = document.createElement("p");\n'
        '    title.className = "model-title";\n'
        '    var nameText = document.createTextNode(model.name.replace(".ds3",""));\n'
        '    title.appendChild(nameText);\n'
        '    var badge = document.createElement("span");\n'
        '    badge.className = "badge " + (model.success ? "success" : "failure");\n'
        '    badge.textContent = model.success ? "pass" : "fail";\n'
        '    title.appendChild(badge);\n'
        '    section.appendChild(title);\n'
        '    if (!model.success) {\n'
        '        var msg = document.createElement("p");\n'
        '        msg.className = "error-msg";\n'
        '        msg.textContent = model.error || "Export failed";\n'
        '        section.appendChild(msg);\n'
        '    } else if (!model.glbData || model.animations.length === 0) {\n'
        '        var msg = document.createElement("p");\n'
        '        msg.className = "no-anim";\n'
        '        msg.textContent = "No animations";\n'
        '        section.appendChild(msg);\n'
        '    } else {\n'
        '        var meta = document.createElement("p");\n'
        '        meta.className = "model-meta";\n'
        '        meta.textContent = model.animations.length + " animation" + (model.animations.length > 1 ? "s" : "");\n'
        '        section.appendChild(meta);\n'
        '        var grid = document.createElement("div");\n'
        '        grid.className = "anim-grid";\n'
        '        var raw = atob(model.glbData);\n'
        '        var binary = new Uint8Array(raw.length);\n'
        '        for (var i = 0; i < raw.length; i++) binary[i] = raw.charCodeAt(i);\n'
        '        model.animations.forEach(function(anim) {\n'
        '            var card = document.createElement("div");\n'
        '            card.className = "anim-card";\n'
        '            var canvas = document.createElement("canvas");\n'
        '            canvas.width = 320; canvas.height = 240;\n'
        '            card.appendChild(canvas);\n'
        '            var info = document.createElement("div");\n'
        '            info.className = "anim-info";\n'
        '            var name = document.createElement("p");\n'
        '            name.className = "anim-name";\n'
        '            name.textContent = anim.name;\n'
        '            info.appendChild(name);\n'
        '            var dur = document.createElement("p");\n'
        '            dur.className = "anim-duration";\n'
        '            dur.textContent = anim.duration.toFixed(2) + "s";\n'
        '            info.appendChild(dur);\n'
        '            card.appendChild(info);\n'
        '            grid.appendChild(card);\n'
        '            allViewers.push({ canvas: canvas, glbBytes: binary.buffer, animIndex: anim.index, scene: null, camera: null, mixer: null, loaded: false, visible: false });\n'
        '        });\n'
        '        section.appendChild(grid);\n'
        '    }\n'
        '    cardsEl.appendChild(section);\n'
        '    var hr = document.createElement("hr");\n'
        '    cardsEl.appendChild(hr);\n'
        '});\n'
        '\n'
        'var observer = new IntersectionObserver(function(entries) {\n'
        '    entries.forEach(function(entry) {\n'
        '        var v = allViewers.find(function(vv) { return vv.canvas === entry.target; });\n'
        '        if (v) v.visible = entry.isIntersecting;\n'
        '    });\n'
        '}, { rootMargin: "200px" });\n'
        'allViewers.forEach(function(v) { observer.observe(v.canvas); });\n'
        '\n'
        'function loadViewer(v) {\n'
        '    v.loaded = true;\n'
        '    v.scene = new THREE.Scene();\n'
        '    v.camera = new THREE.PerspectiveCamera(45, 320/240, 0.01, 100);\n'
        '    v.scene.add(new THREE.AmbientLight(0xffffff, 1.0));\n'
        '    var dl = new THREE.DirectionalLight(0xffffff, 0.8);\n'
        '    dl.position.set(2, 4, 5); v.scene.add(dl);\n'
        '    var dl2 = new THREE.DirectionalLight(0xffffff, 0.4);\n'
        '    dl2.position.set(-3, 2, -2); v.scene.add(dl2);\n'
        '    var loader = new THREE.GLTFLoader();\n'
        '    loader.parse(v.glbBytes.slice(0), "", function(gltf) {\n'
        '        v.scene.add(gltf.scene);\n'
        '        var box = new THREE.Box3().setFromObject(gltf.scene);\n'
        '        var center = box.getCenter(new THREE.Vector3());\n'
        '        var size = box.getSize(new THREE.Vector3());\n'
        '        var maxDim = Math.max(size.x, size.y, size.z);\n'
        '        var dist = maxDim * 1.8;\n'
        '        v.camera.position.set(center.x+dist*0.5, center.y+dist*0.3, center.z+dist);\n'
        '        v.camera.lookAt(center);\n'
        '        if (gltf.animations && gltf.animations.length > v.animIndex) {\n'
        '            v.mixer = new THREE.AnimationMixer(gltf.scene);\n'
        '            v.mixer.clipAction(gltf.animations[v.animIndex]).play();\n'
        '        }\n'
        '    });\n'
        '}\n'
        '\n'
        'var globalClock = new THREE.Clock();\n'
        'function animateAll() {\n'
        '    requestAnimationFrame(animateAll);\n'
        '    var dt = globalClock.getDelta();\n'
        '    allViewers.forEach(function(v) {\n'
        '        if (!v.visible) return;\n'
        '        if (!v.loaded) loadViewer(v);\n'
        '        if (!v.scene) return;\n'
        '        if (v.mixer) v.mixer.update(dt);\n'
        '        sharedRenderer.setSize(320, 240, false);\n'
        '        sharedRenderer.render(v.scene, v.camera);\n'
        '        var ctx = v.canvas.getContext("2d");\n'
        '        ctx.drawImage(sharedRenderer.domElement, 0, 0);\n'
        '    });\n'
        '}\n'
        'animateAll();\n'
        '</script>\n'
        '</body>\n'
        '</html>\n'
    )

    html_path.write_text(html)
    return html_path


def main():
    parser = argparse.ArgumentParser(description="Dust3D test model regression suite")
    parser.add_argument("--dust3d", required=True, help="Path to dust3d executable")
    parser.add_argument("--output-dir", default="test_output", help="Output directory")
    parser.add_argument("--models-dir", default=None, help="Pre-downloaded models directory (skip download)")
    args = parser.parse_args()

    dust3d_bin = Path(args.dust3d).resolve()
    if not dust3d_bin.exists():
        print(f"Error: dust3d binary not found at {dust3d_bin}")
        sys.exit(1)

    output_dir = Path(args.output_dir).resolve()
    output_dir.mkdir(parents=True, exist_ok=True)
    glb_dir = output_dir / "glb"
    glb_dir.mkdir(exist_ok=True)

    ds3_files = discover_ds3_files()
    print(f"Found {len(ds3_files)} test models\n")

    if args.models_dir:
        models_dir = Path(args.models_dir).resolve()
    else:
        models_dir = output_dir / "models"
        models_dir.mkdir(exist_ok=True)
        for ds3_path in ds3_files:
            dest = models_dir / ds3_path
            if dest.exists():
                print(f"  Cached: {ds3_path}")
                continue
            url = f"{RAW_BASE_URL}/{ds3_path}"
            print(f"  Downloading: {ds3_path}")
            download_file(url, dest)

    results = []
    for ds3_rel in ds3_files:
        ds3_path = models_dir / ds3_rel
        model_name = ds3_rel
        print(f"\nProcessing: {model_name}")

        if not ds3_path.exists():
            print("  SKIP: file not found")
            results.append({"model": model_name, "success": False, "error": "File not found", "animations": []})
            continue

        glb_path = glb_dir / f"{Path(ds3_rel).stem}.glb"
        try:
            ok = export_glb(dust3d_bin, ds3_path, glb_path)
        except subprocess.TimeoutExpired:
            print("  TIMEOUT")
            results.append({"model": model_name, "success": False, "error": "Export timeout", "animations": []})
            continue
        except Exception as e:
            print(f"  ERROR: {e}")
            results.append({"model": model_name, "success": False, "error": str(e), "animations": []})
            continue

        if not ok:
            results.append({"model": model_name, "success": False, "error": "Export failed", "animations": []})
            continue

        animations = parse_glb_animations(glb_path)
        print(f"  Exported OK, {len(animations)} animation(s)")
        for a in animations:
            print(f"    - {a['name']} ({a['duration']:.2f}s)")

        results.append({"model": model_name, "success": True, "animations": animations})

    print("\n" + "=" * 60)
    report_path = generate_html_report(results, output_dir, glb_dir)
    print(f"Report: {report_path}")

    passed = sum(1 for r in results if r["success"])
    failed = len(results) - passed
    print(f"Results: {passed} passed, {failed} failed, {len(results)} total")

    json_path = output_dir / "results.json"
    json_path.write_text(json.dumps(results, indent=2))

    sys.exit(1 if failed > 0 else 0)


if __name__ == "__main__":
    main()
