"""Run the Dust3D binary in batch mode: `dust3d model.ds3 -o model.glb`."""

from __future__ import annotations

import os
import platform
import shutil
import subprocess
from typing import Dict, List, Optional

_CANDIDATES = [
    "application/dust3d",
    "build/dust3d",
    "application/dust3d.app/Contents/MacOS/dust3d",
    "build/dust3d.app/Contents/MacOS/dust3d",
    "/Applications/dust3d.app/Contents/MacOS/dust3d",
    "/Applications/Dust3D.app/Contents/MacOS/dust3d",
]


def find_dust3d(explicit: Optional[str] = None) -> str:
    if explicit:
        return explicit
    env = os.environ.get("DUST3D_BIN")
    if env:
        return env
    repo = os.path.normpath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))
    for c in _CANDIDATES:
        p = c if os.path.isabs(c) else os.path.join(repo, c)
        if os.path.isfile(p) and os.access(p, os.X_OK):
            return p
    p = shutil.which("dust3d")
    if p:
        return p
    raise FileNotFoundError("Dust3D binary not found; pass --dust3d or set DUST3D_BIN")


def export(ds3_path: str, outputs: List[str], dust3d: Optional[str] = None, timeout: int = 300,
           full_log: bool = False) -> Dict:
    """Export a .ds3 to one or more of .glb/.fbx/.obj. Returns {ok, returncode, log, outputs}."""
    exe = find_dust3d(dust3d)
    for o in outputs:
        if os.path.exists(o):
            os.remove(o)
    cmd = [exe, os.path.abspath(ds3_path)]
    for o in outputs:
        cmd += ["-o", os.path.abspath(o)]
    env = os.environ.copy()
    if platform.system() == "Linux" and not env.get("DISPLAY"):
        env.setdefault("QT_QPA_PLATFORM", "offscreen")
    def run(command):
        try:
            res = subprocess.run(command, env=env, capture_output=True, timeout=timeout,
                                 cwd=os.path.dirname(os.path.abspath(ds3_path)))
            return (res.stdout + res.stderr).decode("utf-8", "replace"), res.returncode
        except subprocess.TimeoutExpired as e:
            return ((e.stdout or b"") + (e.stderr or b"")).decode("utf-8", "replace"), -999

    log, rc = run(cmd)
    if rc < 0 and rc != -999 and env.get("QT_QPA_PLATFORM") == "offscreen" and shutil.which("xvfb-run"):
        # Dust3D releases without the batch-mode fix show a window even when exporting and
        # crash on the offscreen platform; run them on a virtual X display instead.
        env.pop("QT_QPA_PLATFORM")
        log, rc = run(["xvfb-run", "-a", "-s", "-screen 0 1280x1024x24"] + cmd)
        log = "[retried under xvfb-run: this Dust3D build crashes on QT_QPA_PLATFORM=offscreen]\n" + log
    produced = {o: os.path.exists(o) and os.path.getsize(o) > 0 for o in outputs}
    interesting = [ln for ln in log.splitlines() if not ln.startswith("SEAM_REPORT")
                   and any(k in ln.lower() for k in ("error", "fail", "exception", "not found", "warning:", "grounding"))]
    result = {"ok": rc == 0 and all(produced.values()), "returncode": rc, "outputs": produced,
              "command": " ".join(cmd), "log_tail": log.splitlines()[-15:], "log_highlights": interesting[-30:]}
    if full_log:
        result["log"] = log
    return result
