# Dust3D agent modeling toolkit

Lets an AI agent (or any script) create rigged, animated Dust3D models from a text
description, and check its own work, with no GUI in the loop.

```
spec.json ──compile──▶ model.ds3 ──dust3d -o──▶ model.glb ──inspect/render──▶ report.json + PNG/GIF
     ▲                                                                              │
     └─────────────────────────── agent reads, fixes spec ◀─────────────────────────┘
```

- **spec** — a small JSON description: parts as node chains `[x, y, z, radius]`, colours,
  mirror, cross-sections, bone names and animation clips. See `examples/` and
  [`AGENT_GUIDE.md`](AGENT_GUIDE.md) (the manual to give the agent).
- **compile** — writes a normal `.ds3` document (deterministic ids, escaped XML), so
  every generated model opens in the Dust3D editor for hand refinement.
- **export** — runs the Dust3D binary in batch mode (`dust3d model.ds3 -o model.glb`;
  also `.fbx`, `.obj`). Needs the batch-mode fixes on this branch to run headless.
- **inspect / render** — pure numpy + Pillow: mesh and rig metrics, orthographic
  turnarounds, skeleton overlays, and skinned animation strips/GIFs.
- **seams** — every join between parts is scored from Dust3D's own seam recombiner
  (`DUST3D_SEAM_REPORT=1` makes the engine report each union's edge loops and bridge
  geometry), with wireframe close-ups of bad joins. `--tune-seams` then adjusts the
  joint nodes automatically until the bridges have good edge flow.

## Three ways to use it

1. **Ask an AI agent.** In Claude Code (or any agent that can run shell commands)
   opened on this repository, say for example: *"Read
   tools/agent_modeling/AGENT_GUIDE.md, then make a rigged walking red panda in
   Dust3D."* The agent writes a spec, builds it, reads the report and turntable
   images, and iterates until the checklist passes. You get `.ds3`, `.glb` and
   previews. Installing the guide as a skill makes this a one-line request.
2. **Refine by hand.** Open the generated `.ds3` in Dust3D. Every part, node, bone and
   animation is a normal editable document element. Or edit the JSON spec and rebuild.
3. **Start from an existing model.** `decompile any.ds3 -o spec.json` turns a Dust3D
   document into a spec. The agent (or a script) edits it, for example to recolour,
   re-proportion, or make variants in bulk, and builds it again.

## What you need

- **This folder** (`tools/agent_modeling`): pure Python 3 with numpy and Pillow. It
  doesn't need the Dust3D sources; copy it anywhere. Rig templates (bone names,
  hierarchy) are read from the binary with `dust3d -list-rigs`, so they always match
  the Dust3D you build with.
- **A Dust3D binary**, pointed to by `DUST3D_BIN`: the AppImage, `dust3d.exe`, or
  `Dust3D.app/Contents/MacOS/dust3d`. Any release built by
  `.github/workflows/release.yml` from a commit that includes this branch's engine
  changes has everything. With an older release:

  | feature | older release | release with this branch |
  |---|---|---|
  | build `.ds3` -> `.glb`/`.fbx`/`.obj`, rig, animations, metrics, renders | works | works |
  | headless export | needs a display (Linux: Xvfb); a window flashes per export on macOS/Windows | runs hidden; `QT_QPA_PLATFORM=offscreen` on Linux |
  | failed or empty exports | exit code 0 | exit code 1 |
  | seam report, `--tune-seams`, `seams` | unavailable (the report says so) | works |
| failed booleans named in the seam report (tuner can fix them) | no | yes |
| `SpiderAttack`, `SnakeStrike`, `BipedHop` animations | no | yes |
| hurt for every rig, `FishAttack`, `InsectBite`, `BirdStrike`, `BipedKick`, authored deaths, `airborne` hurt/death | no | yes |
| part `interpolate: false` (low-poly rigid parts) | ignored (full ring count) | yes |
| hard-surface `shape` entries (box, beam, cylinder, plate, bolts, groove) | build, but joins get soft seam bridges and I/T caps may leave holes | crisp `hard` joins, per-node taper, clean concave caps |
  | rig templates (`-list-rigs`): lint of bone names, `rigs` command | unavailable (error) | works |
  | fixes (unweighted seam vertices, deterministic mirror order, XML escaping, cut-face bounds) | missing | included |

## Using the models in a game

Every `build` writes `<name>_clips.json` beside the `.glb`: each clip's type, true duration,
frame count, loop flag and events (`hit`, `step`), for a game engine's importer (see
`AGENT_GUIDE.md`, "Clip timing"). Families of creatures share a rig and a design through
`"extends"` variants.

The exported model carries per-part materials (metal/roughness and emissive maps) and
per-vertex part labels. From those, `build` splits equipment variants into their own meshes
(`slot`), smooths skin weights at joints, keys posed clips and checks a triangle budget; clothes
that follow the body are `wrap` parts. See `AGENT_GUIDE.md`, "Game-ready assets".

## Seam report switch

`DUST3D_SEAM_REPORT` is an environment variable the **engine** reads once at start-up.
When it is `1` (anything except unset, empty, `0`, `false`, `off`), Dust3D prints one
`SEAM_REPORT ...` line to stdout for every union it bridges. It changes nothing else:
the geometry is identical with or without it.

You normally never set it. The toolkit turns it on for its own Dust3D runs (`build`,
`seams`, `--tune-seams`) and leaves your environment alone. To see the raw lines yourself:

```bash
DUST3D_SEAM_REPORT=1 dust3d model.ds3 -o model.obj | grep SEAM_REPORT
```

## Quick start

```bash
pip install numpy pillow            # the only dependencies
export DUST3D_BIN=/path/to/dust3d   # or dust3d.app/Contents/MacOS/dust3d on macOS
cd tools/agent_modeling
python3 -m dust3d_agent build examples/fox.json -o out/fox --gif
open out/fox/fox_turnaround.png out/fox/fox_anim_walk.gif
```

Commands:

| command | purpose |
|---|---|
| `build SPEC [-o DIR] [--gif] [--extra fbx obj] [--no-render]` | spec → .ds3 → .glb → report + images |
| `build SPEC --tune-seams` | first tune joint nodes for clean seams (writes `SPEC.tuned.json`) |
| `seams SPEC\|MODEL.ds3` | join quality report + wireframe close-ups of bad seams |
| `lint SPEC` | static checks only |
| `inspect MODEL.glb [--render out.png --skeleton]` | metrics for any Dust3D export |
| `decompile MODEL.ds3 [-o SPEC.json]` | existing document → spec (for learning/editing) |
| `rigs [RIG]` | rig bone names, template positions, animation types |

On Linux without a display the exporter sets `QT_QPA_PLATFORM=offscreen` automatically.

## Tests

```bash
python3 -m unittest discover -s tools/agent_modeling/tests -v
# with a Dust3D build and the reference models:
DUST3D_BIN=... DUST3D_TEST_MODELS=/path/to/dust3d-test-models python3 -m unittest discover -s tools/agent_modeling/tests
```

The reference round trip decompiles each model in `dust3d-test-models`, recompiles
it through the spec and checks the export matches the original triangle for triangle
(all 19 pass).

## Spec coverage

Everything Dust3D's mesh generator reads is covered:

- tube parts: chains and loops, mirror, colour/metallic/roughness, preset and custom
  cross-sections (polygon and stroke profiles), cut rotation, flattening, `deformUnified`,
  rounded/subdivided/chamfered, smoothing cutoff, disabled parts
- combine modes (Normal / Inversion / Uncombined) and nested groups
- stitching-line surfaces (front/back/side closing, target segments, mirrored groups)
- stitching-loop surfaces (open/closed loops, fill interior, back closing with depth/sharpness)
- imported meshes swept along a spine, colour images
- all rig types, bone assignment per edge, animation clips with parameters and per-type timing
- per-part metallic, roughness and glow (`emissive`); equipment slots (component-name suffix)
- variants (`extends` with recolour, remove, override, add, scale)
- skin modifier groups (`"skin"`): a seamless creature skin over the children, or a cloth
  garment over them (offset, drape, openings, hem thickness, face budget, weights bound to the body)

The editor also stores `hollowThickness` and per-node cut faces, but the mesh generator
never reads them, so the spec leaves them out.
