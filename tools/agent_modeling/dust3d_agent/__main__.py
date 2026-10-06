"""Command line entry point.

  python3 -m dust3d_agent build  SPEC.json [-o OUTDIR] [--dust3d BIN] [--no-render] [--gif] [--tune-seams]
  python3 -m dust3d_agent lint   SPEC.json
  python3 -m dust3d_agent inspect MODEL.glb [--render OUT.png]
  python3 -m dust3d_agent decompile MODEL.ds3 [-o SPEC.json]
  python3 -m dust3d_agent seams  SPEC.json|MODEL.ds3   # seam (join) quality + wireframe close-ups
  python3 -m dust3d_agent rigs   [RIG]          # list rig bones / animation types

`build` writes OUTDIR/<name>.ds3, <name>.glb, <name>_turnaround.png,
<name>_anim_<clip>.png and <name>_report.json. The report is the thing an
agent should read after every iteration.
"""

from __future__ import annotations

import argparse
import json
import os
import sys
import time

from . import ds3, export, spec as specmod
from .spec import animation_timing


def _render_outputs(glb_path, outdir, name, gif=False, anim_frames=6):
    from . import glb, render
    g = glb.load(glb_path)
    files = {}
    looks = glb.outfits(g)
    if looks:
        # equipment slots: one front view per outfit, then preview the heaviest one
        from PIL import Image
        shots = [render.turnaround(glb.dressed(g, o), 240, skeleton=False, views=("three_quarter",)) for o in looks]
        sheet = Image.new("RGB", (sum(s.width for s in shots), max(s.height for s in shots)), "white")
        x = 0
        for s in shots:
            sheet.paste(s, (x, 0))
            x += s.width
        p = os.path.join(outdir, name + "_outfits.png")
        sheet.save(p)
        files["outfits"] = p
        g = glb.dressed(g, looks[-1])
    p = os.path.join(outdir, name + "_turnaround.png")
    render.turnaround(g, 320, skeleton=False).save(p)
    files["turnaround"] = p
    if g.skin is not None:
        p = os.path.join(outdir, name + "_skeleton.png")
        render.turnaround(g, 320, skeleton=True, views=("front", "left")).save(p)
        files["skeleton"] = p
    for a in g.animations:
        p = os.path.join(outdir, "%s_anim_%s.png" % (name, a["name"]))
        render.animation_strip(g, a, frames=anim_frames, size=200).save(p)
        files["anim_" + a["name"]] = p
        if gif:
            p = os.path.join(outdir, "%s_anim_%s.gif" % (name, a["name"]))
            render.animation_gif(g, a, p)
            files["gif_" + a["name"]] = p
    return files


def cmd_build(args):
    t0 = time.time()
    tuning = None
    if args.tune_seams:
        from . import tune
        print("tuning seams (each step rebuilds the model)...", file=sys.stderr)
        tuning = tune.tune_file(args.spec, dust3d=args.dust3d, max_steps=args.tune_steps,
                                log=lambda s: print(s, file=sys.stderr))
        args.spec = tuning["path"]
    sp = specmod.load_spec(args.spec)
    name = sp.name
    outdir = args.out or os.path.join(os.path.dirname(os.path.abspath(args.spec)), "out", name)
    os.makedirs(outdir, exist_ok=True)
    report = {"name": name, "spec": os.path.abspath(args.spec), "lint": specmod.lint_spec(sp)}
    if tuning:
        report["seam_tuning"] = {k: tuning[k] for k in ("path", "changes", "penalty_before", "penalty_after", "evaluations")}
    xml, assets, _ = ds3.build_document(sp)
    ds3_path = os.path.join(outdir, name + ".ds3")
    ds3.write_ds3(ds3_path, xml, assets)
    report["ds3"] = ds3_path
    clips = [{"name": a.name, "type": a.type, "durationSeconds": animation_timing(a)[0],
              "frameCount": animation_timing(a)[1],
              "loop": bool(a.pose.get("loop")) if a.type == "Pose" else a.type in specmod.LOOPING_ANIMATIONS}
             for a in sp.animations] if sp.rig else []
    clips_path = os.path.join(outdir, name + "_clips.json")

    def write_clips():
        # For game engines: which clip loops, its true length and its events (hit, step).
        # Biped clips include a terminal key at the full duration (loops repeat the first
        # pose there). Other rigs retain their legacy sampling convention; the manifest
        # supplies the full loop duration for importers.
        with open(clips_path, "w") as f:
            json.dump({"model": name + ".glb", "rig": sp.rig, "clips": clips}, f, indent=2)
        report["clips"] = clips_path
    if clips:
        write_clips()
    glb_path = os.path.join(outdir, name + ".glb")
    obj_path = os.path.join(outdir, name + "_topology.obj")
    outputs = [glb_path, obj_path] + [os.path.join(outdir, name + "." + e) for e in (args.extra or [])]
    from . import seams
    ex = seams.export_with_seams(ds3_path, outputs, args.dust3d, timeout=args.timeout)
    seam_result = seams.seams_from_log(ex.pop("log", ""), [g.name for g in sp.groups() if g.stitch])
    report["export"] = ex
    report["seams"] = {"total_penalty": seam_result["total_penalty"],
                       "bad": [{k: v for k, v in s.items()} for s in seam_result["bad"]],
                       "all": seam_result["seams"]}
    if not seam_result["reports_found"] and ex["ok"]:
        report["seams"]["note"] = "this Dust3D build does not print SEAM_REPORT lines; seam checks unavailable"
    if ex["outputs"].get(glb_path):
        from . import gamekit
        report["game"] = gamekit.finish(glb_path, sp, clips)
        if clips:
            write_clips()
        from . import metrics
        cloth = {g.name for g in sp.groups() if (g.wrap or {}).get("mode") == "cloth"}
        # open by design: cloth rims and openings, and an imported mesh's own openings (eyes
        # left open for the eyelids to blink over)
        imported = {p.name for p in sp.parts if p.kind == "ImportedModel"}
        report["metrics"] = metrics.analyze(glb_path, clips, open_parts=cloth | imported)
        if not args.no_render:
            report["images"] = _render_outputs(glb_path, outdir, name, gif=args.gif)
            if ex["outputs"].get(obj_path) and seam_result["bad"]:
                for p in seams.closeups(obj_path, seam_result["bad"], os.path.join(outdir, name)):
                    report["images"]["seam_" + os.path.basename(p)[len(name) + 6:-4]] = p
    from . import quality
    report["quality"] = quality.evaluate(report)
    report["seconds"] = round(time.time() - t0, 2)
    rp = os.path.join(outdir, name + "_report.json")
    with open(rp, "w") as f:
        json.dump(report, f, indent=2)
    summary = {
        "ok": ex["ok"] and (not args.strict or report["quality"]["ok"]), "report": rp,
        "quality": report["quality"],
        "lint": report["lint"],
        "warnings": report.get("metrics", {}).get("warnings", []),
        "seam_penalty": report["seams"]["total_penalty"] if seam_result["reports_found"] else "unavailable (Dust3D build has no seam report)",
        "bad_seams": ["%s: %s" % (s["part"], "; ".join(s["problems"])) for s in report["seams"]["bad"]],
        "size_xyz": report.get("metrics", {}).get("size_xyz"),
        "triangles": report.get("metrics", {}).get("triangles"),
        "islands": report.get("metrics", {}).get("islands"),
        "bones": len(report.get("metrics", {}).get("bones", [])),
        "animations": [a["name"] for a in report.get("metrics", {}).get("animations", [])],
        "game": report.get("game", {}),
        "images": report.get("images", {}),
    }
    print(json.dumps(summary, indent=2))
    return 0 if summary["ok"] else 1


def cmd_lint(args):
    sp = specmod.load_spec(args.spec)
    w = specmod.lint_spec(sp)
    print(json.dumps({"parts": len(sp.parts), "warnings": w}, indent=2))
    return 0


def cmd_inspect(args):
    from . import metrics
    r = metrics.analyze(args.glb)
    print(json.dumps(r, indent=2))
    if args.render:
        from . import glb, render
        render.turnaround(glb.load(args.glb), 320, skeleton=args.skeleton).save(args.render)
    return 0


def cmd_decompile(args):
    from . import decompile
    out_dir = os.path.dirname(os.path.abspath(args.out)) if args.out else os.getcwd()
    sp, warnings = decompile.decompile_file(args.ds3, asset_dir=out_dir)
    text = json.dumps(sp, indent=1)
    if args.out:
        with open(args.out, "w") as f:
            f.write(text)
    else:
        print(text)
    for w in warnings:
        print("warning:", w, file=sys.stderr)
    return 0


def cmd_seams(args):
    from . import seams
    src = args.model
    work = args.out or os.path.join(os.path.dirname(os.path.abspath(src)), "out", "seams")
    os.makedirs(work, exist_ok=True)
    shells = []
    if src.endswith(".json"):
        sp = specmod.load_spec(src)
        shells = [g.name for g in sp.groups() if g.stitch]
        ds3_path = os.path.join(work, sp.name + ".ds3")
        xml, assets, _ = ds3.build_document(sp)
        ds3.write_ds3(ds3_path, xml, assets)
    else:
        ds3_path = src
    r = seams.analyze_seams(ds3_path, work, args.dust3d, shells=shells)
    images = seams.closeups(r["obj"], r["bad"], os.path.join(work, os.path.splitext(os.path.basename(ds3_path))[0])) if r.get("obj") else []
    print(json.dumps({"total_penalty": r["total_penalty"],
                      "seams": [{k: s.get(k) for k in ("part", "penalty", "problems", "max_fan", "min_angle", "loop_vertices")}
                                for s in r["seams"]], "images": images}, indent=1))
    return 0


def cmd_rigs(args):
    rigs = [args.rig] if args.rig else specmod.rig_types()
    out = {}
    for r in rigs:
        t = specmod.load_rig_template(r)
        out[r] = {"description": t["description"],
                  "bones": {b: {"parent": t["bones"][b]["parent"],
                                "template_start": t["bones"][b]["pos"],
                                "template_end": t["bones"][b]["end"]} for b in t["order"]} if args.rig else t["order"],
                  "animations": specmod.ANIMATION_TYPES.get(r, [])}
    print(json.dumps(out, indent=1))
    return 0


def main(argv=None):
    ap = argparse.ArgumentParser(prog="dust3d_agent")
    sub = ap.add_subparsers(dest="cmd", required=True)
    b = sub.add_parser("build")
    b.add_argument("spec")
    b.add_argument("-o", "--out")
    b.add_argument("--dust3d")
    b.add_argument("--timeout", type=int, default=300)
    b.add_argument("--no-render", action="store_true")
    b.add_argument("--gif", action="store_true")
    b.add_argument("--strict", action="store_true",
                   help="exit nonzero for reported lint, mesh, animation, seam or budget defects")
    b.add_argument("--extra", nargs="*", help="extra export formats, e.g. fbx obj")
    b.add_argument("--tune-seams", action="store_true",
                   help="first adjust joint nodes until every seam bridges cleanly (writes <spec>.tuned.json)")
    b.add_argument("--tune-steps", type=int, default=12)
    b.set_defaults(fn=cmd_build)
    l_ = sub.add_parser("lint")
    l_.add_argument("spec")
    l_.set_defaults(fn=cmd_lint)
    i = sub.add_parser("inspect")
    i.add_argument("glb")
    i.add_argument("--render")
    i.add_argument("--skeleton", action="store_true")
    i.set_defaults(fn=cmd_inspect)
    d = sub.add_parser("decompile")
    d.add_argument("ds3")
    d.add_argument("-o", "--out")
    d.set_defaults(fn=cmd_decompile)
    sm = sub.add_parser("seams")
    sm.add_argument("model", help="spec .json or .ds3")
    sm.add_argument("-o", "--out")
    sm.add_argument("--dust3d")
    sm.set_defaults(fn=cmd_seams)
    r = sub.add_parser("rigs")
    r.add_argument("rig", nargs="?")
    r.set_defaults(fn=cmd_rigs)
    args = ap.parse_args(argv)
    try:
        return args.fn(args)
    except specmod.SpecError as e:
        print(json.dumps({"ok": False, "spec_error": str(e)}, indent=2))
        return 2
    except (RuntimeError, FileNotFoundError) as e:
        print(json.dumps({"ok": False, "error": str(e)}, indent=2))
        return 3


if __name__ == "__main__":
    sys.exit(main())
