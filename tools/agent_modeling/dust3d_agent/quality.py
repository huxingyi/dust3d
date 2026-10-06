"""Machine-readable acceptance gate for unattended asset builds.

This gate checks reported defects, not artistic quality or engine compatibility.
"""


def evaluate(report):
    failures = []
    if not report.get("export", {}).get("ok"):
        failures.append("export failed")
    if not report.get("metrics", {}).get("triangles", 0):
        failures.append("no inspected geometry")
    failures.extend("lint: " + w for w in report.get("lint", [])
                    if not w.startswith(("info:", "advisory:")))
    failures.extend(report.get("metrics", {}).get("warnings", []))
    for seam in report.get("seams", {}).get("bad", []):
        failures.append("seam %s: %s" % (seam.get("part", "?"), "; ".join(seam.get("problems", []))))
    if report.get("game", {}).get("budget", {}).get("ok") is False:
        failures.append("triangle budget exceeded")
    return {"ok": not failures, "failures": failures,
            "scope": "reported mesh, animation, lint, seam and triangle-budget checks"}
