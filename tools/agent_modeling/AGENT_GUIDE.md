# Dust3D agent modeling guide

The operating manual for an AI agent that creates Dust3D models automatically.
Give it to the agent (or install it as a skill) together with the specs in `examples/`.

## The loop

```
1. Pick a rig        python3 -m dust3d_agent rigs Quadruped      (bone names + template positions)
2. Write a spec      start from the closest file in examples/
3. Build             python3 -m dust3d_agent build my.json -o out/my
4. Read the report   out/my/my_report.json  -> lint, warnings, metrics
5. Look              out/my/my_turnaround.png, my_skeleton.png, my_anim_<clip>.png
6. Fix and repeat    usually 2-4 iterations; finish with --tune-seams for clean joins
```

`build` compiles the spec into a normal `.ds3` document, runs Dust3D headlessly
(`dust3d model.ds3 -o model.glb`) and inspects/renders the result. The `.ds3` opens
in the Dust3D editor for manual refinement at any time.

Extras: `--gif` (animated previews), `--extra fbx obj` (more export formats),
`decompile some.ds3 -o spec.json` (any existing Dust3D model as a spec, to learn
from or modify; imported meshes/images are extracted next to it).

## World frame and scale

- **+Y up, +Z forward** (the creature faces +Z), **+X is the creature's LEFT**.
- Canvas units. **Keep the largest dimension around 1.0**: some animation parameters
  are absolute distances tuned for that size.
- Rigged models are grounded automatically (lowest foot moved to y = 0).

## The four building blocks

`parts` is a list; each entry is one of these (groups nest).

### 1. Tube part — the workhorse

```json
{"name": "leg", "nodes": [[x, y, z, radius], ...], "bones": [...], "mirror": true, "color": "#aa7744"}
```

An ordered chain of nodes. Dust3D sweeps a cross-section along it, interpolating radius.

- A part is a chain, never a tree. Branches (legs, ears) are separate parts whose first
  node sits inside the part they grow from.
- You do **not** need many nodes. On any edge longer than 1.5x the sum of its end radii,
  Dust3D inserts extra rings one radius in from each end, which keeps joints round and
  gives edge loops where limbs bend. Add nodes only to change the silhouette.
- `rounded` caps the ends; `subdivided` smooths the cross-section; `loop: true` closes
  the chain into a ring (a torus-like part).
- `mirror: true` adds the X-mirrored copy. Model the left side (x > 0) with `Left`
  bones; the copy gets `Right`. A mirrored part must not cross x = 0.
- Cross-section `cutFace`: `"Quad"` (default), `"Pentagon"`, `"Hexagon"`, `"Triangle"`, a
  polygon `[[u, v], ...]` (e.g. a diamond blade `[[1,0],[0,0.18],[-1,0],[0,-0.18]]`), or a
  stroke `{"stroke": [[u, v, r], ...]}`. Profiles are normalized; only shape matters.
  `cutRotation` turns it (1.0 = 180 degrees).
- **Flatten with `flatten`, not the raw deform parameters**: `{"x": 0.2}` squashes to 20%
  along world X. The raw `deformWidth` axis depends on the chain's bend plane (on a
  straight chain along Z it squashes Y), which is easy to get backwards. `deformUnified`
  keeps a flattened part evenly thin instead of scaling with each node's radius.
- `import: "mesh.glb"` sweeps an existing mesh along the chain instead of a
  cross-section (its local Y axis follows the spine). Use a 2-node chain for a rigid prop.
- `image: "texture.png"` paints the part with an image instead of a flat colour.
- `metallic`, `roughness`, `smooth` (normal smoothing cutoff in degrees: 0 faceted, 60-90
  stylized, 120+ very smooth), `disabled` (kept in the document, not in the mesh).

### 2. Group — assemblies and carving

```json
{"name": "head", "combine": "Normal", "group": [ ...parts or groups... ]}
```

A group is unioned on its own and then combined with its siblings. Use it to carve a
whole assembly (`"combine": "Inversion"`) or to control union order (see below).

### 3. Stitching lines — thin shells: fins, wings, leaves, ears, capes

```json
{"name": "tail_fin", "stitch": "lines", "color": "#f6b26b",
 "lines": [{"nodes": [[x,y,z,r], ...], "bones": ["TailStart", "TailEnd"]}, ...],
 "frontClosed": false, "backClosed": false, "sideClosed": false, "targetSegments": 0}
```

Each line is a spline (a "rib"). Dust3D resamples all lines to the same number of
segments and lofts consecutive lines into a quad surface. **Node radius is the shell
thickness** at that point: roughly 0.02-0.04 at the root, tapering to ~0.003 at the edge.

- Order lines across the surface (front to back), each running root to edge. At least 2.
- An open surface gets a solid shell (thickness from the radii). `sideClosed` joins each
  line's two ends (a tube-like loft), `frontClosed`/`backClosed` cap the first/last line.
- `targetSegments` (0 = auto) sets the resolution along the lines.
- `mirror: true` emits a mirrored copy of the whole group (for paired fins/wings). Don't
  use Dust3D's per-part mirror here: it would put both sides into one loft.
- This gives much better fins and wings than flattened tubes: sharp edges, real
  outlines (forked tails, scalloped wings), and bones per rib.

### 4. Stitching loops — faces and sculpted surfaces

```json
{"name": "face", "stitch": "loops", "color": "#e8a25c", "backClosed": true,
 "backCloseDepthRatio": 0.6, "backCloseSharpness": 0.2,
 "loops": [{"name": "outline", "nodes": [...]},
           {"name": "eye", "closed": true, "fillInterior": true, "color": "#2c9a3a", "nodes": [...]}]}
```

Edge loops, like retopology, viewed from the **front** (XY). Dust3D remeshes the area
between loops into a quad grid, then lifts it back to 3D using each node's z.

- Always X-mirrored. Draw one half. An **open** loop is half a shape: put its ends
  on x = 0 (outline, mouth, nose). A **closed** loop entirely on one side is copied to
  the other side (eyes). A closed loop spanning both sides is used as is.
- Each loop colours the surface nearest to it. Loops without a colour inherit the
  group colour. Surround small coloured details (mouth, nose) with a face-coloured
  loop, or their colour floods the area.
- `fillInterior` caps an isolated closed loop (a solid eye patch). `backClosed` closes
  the open outer boundary into a solid, `backCloseDepthRatio` sets its depth, and
  `backCloseSharpness` makes the back rounded (0) or pointed.
- The surface is a single mesh island. Add ears, horns etc. as tube parts next to the
  group; they union onto it.

## How parts are combined, and getting good edge flow at joins

Dust3D walks each group's children in order. It unions consecutive children with the
same combine mode as one run, then combines the runs (`Inversion` subtracts).

After every union the **seam recombiner** runs. It removes the jagged triangles along
the intersection curve, leaving one edge loop on each part, then **bridges** the two
loops with a strip of triangles, pairing each vertex of the smaller loop with its
nearest vertex on the larger one. So the quality of a join depends on the two loops.

- It must be **one closed ring per side**. Otherwise nothing is bridged and the raw
  boolean triangles stay.
- The loops should be of **similar size and resolution**. A small loop against large
  parent faces gives a **fan**: many bridge triangles sharing one vertex.
- They should sit **close together**. A wide gap gives long, stretched triangles and
  a visible pinch, e.g. a tail grazing a rounded body end.

### The seam check (every build)

`build` runs Dust3D with seam reporting on (the engine prints one `SEAM_REPORT` per
union) and scores every join. In the summary, `seam_penalty` is the model total (0 is
perfect) and `bad_seams` lists the failing joins with reasons. Details are in
`report.seams.all`. Each bad seam gets a wireframe close-up, `<name>_seam_<part>.png`
(quads grey, triangles orange). Failing checks, calibrated on the hand-made
reference models:

| check | bad when | typical cause |
|---|---|---|
| not bridged / several places | not one ring per side | limb passes through, grazes, or touches two parts at once |
| fan | >= 8 bridge triangles on one vertex | limb much finer or coarser than the faces it lands on |
| sliver | a bridge triangle under 5 degrees | loops nearly tangent, or offset along the limb |
| resolution mismatch | loop edge lengths differ > 3x | tiny detail on big faces, or vice versa |
| wide gap | bridge longer than 1.35x seam radius | joint on a rounded end cap or at a shallow angle |

### Fixing seams

Automatic: `build my.json --tune-seams`. For the worst seam it finds the part and node
that make it, tries small edits and rebuilds. The edits: sink or pull the node
0.2-0.4 radius along the limb, move it 0.3 radius toward or away from the seam, scale
its radius by 0.85 or 1.15, or add a ring near the joint. It keeps whichever lowers the
total penalty, and repeats. Every node stays within 0.6 of its radius of where you put
it, so the design is preserved. It writes `my.tuned.json` and logs each change in
`report.seam_tuning`. Expect 1-3 minutes for a creature.

By hand, what usually works:

- Start a limb **inside** the parent, and let it cross the surface **once**, roughly
  perpendicular to it. Enter through the side of a body, not a rounded end cap: attach a
  tail to the rump's side a little before the end of the body chain.
- **Step sizes down gradually.** A limb more than about 3x thinner than the part it
  lands on meets only one or two big faces and fans. The tuner can't fix that within
  its bounds, so change the design: give the limb a flared base (thigh, shoulder, ear
  base, thicker fin root), then continue thin. The linter flags such joins as advisory.
- **Attach to the modelled side.** Dust3D adds mirrored copies after all other parts,
  so a prop held in the mirrored (-X) hand is unioned before that hand exists: first
  floating, then a double seam. Put props on the +X side (the linter warns).
- Resolution mismatch on a stitched-loop surface: the grid size follows the loops'
  edge lengths, so add nodes to nearby loops to make the surface finer.
- Add a node close to the joint (just outside the parent), so the limb's first ring
  outside is at a similar size to the parent's rings.
- Small details (nose, ears) need a finer parent: give the head more nodes, or use
  `Hexagon`/`subdivided` so its faces are closer in size to the detail.
- Keep solid parts in one run and avoid three parts meeting at one spot. The compiler
  moves `Uncombined` parts into a trailing group automatically (`"autoOrder": false`
  keeps your order exactly).
- Stitched shells lying exactly on x = 0 are nudged by 0.001, because exactly
  coincident geometry breaks Dust3D's boolean (non-manifold edges).

## Rigging and animation

Set `"rig"` to `Biped, Quadruped, Bird, Fish, Insect, Snake` or `Spider`, then give every
edge of every part (and every stitch line) a bone via `bones`: one name per edge, or one
string for all. Valid names: `python3 -m dust3d_agent rigs <Rig>`.

- Follow the template topology: spine bones along the body chain in order, limb chains
  `Upper -> Lower -> Foot/Hand` from body outward.
- A limb's first edge lives inside the body. Give it the body bone it attaches to (front
  leg: `Chest`), or the shoulder bone for arms.
- Single-node parts need no bone. Optional bones (hair, capes, tails) can be left
  unmodelled (the linter says `info:`).
- Animations: `["QuadrupedWalk", {"type": "QuadrupedRun", "name": "run", "params": {...}}]`.
  Tune amplitude-like params for unusual bodies (the goldfish swim uses
  `spineAmplitude 0.035`; the T-rex walk `armSwingFactor 0.2` for tiny arms).

## Reading the report

| field | meaning / action |
|---|---|
| `lint` | Static spec problems. Fix all except `info:` / `advisory:` lines. |
| `export.ok` | Dust3D produced the GLB. A non-zero exit means an empty or failed mesh; see `export.log_highlights`. |
| `metrics.size_xyz` | Overall proportions. |
| `metrics.islands` | Should be 1 + the number of Uncombined parts. More means a part doesn't overlap its parent. |
| `seam_penalty`, `bad_seams` | Join quality (see above). Fix every bad seam, or run `--tune-seams`. |
| `metrics.sliver_triangles` | Thin triangles anywhere in the mesh. |
| `metrics.open_edges`, `nonmanifold_edges` | Should be ~0 for tube models. Many usually means coincident geometry or grazing contact. |
| `metrics.signed_volume` | Negative means inside-out surfaces. |
| `metrics.asymmetry` | ~0 for symmetric models. |
| `metrics.unweighted_vertices` | Must be 0. |
| `metrics.animations[].max_vertex_motion_rel` | Near 0: the clip does nothing. Above ~1: exploded rig or amplitude too large. |

Pictures: `turnaround` (front / left / top / three-quarter), `skeleton` (bones over the
mesh), `anim_<clip>` (6 frames). In the front view the creature's left is image-right.

## Checklist before declaring done

- [ ] lint has only `info:` / `advisory:` lines; `warnings` empty
- [ ] silhouette recognizable in the front **and** left views
- [ ] islands = 1 + Uncombined parts
- [ ] `bad_seams` empty (use `--tune-seams`); seam close-ups show clean strips
- [ ] skeleton: every bone inside its limb
- [ ] each animation strip shows plausible motion, with no spikes or collapsing vertices
