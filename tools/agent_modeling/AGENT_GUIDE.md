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

## The building blocks

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
- `interpolate: false` turns off those extra rings (and the rings Dust3D adds one radius in
  from each end): the part gets rings only at its own nodes. Use it for rigid, low-poly pieces (props, plates, blades, posts) together with
  `subdivided: false`; a desert shrub drops from about 6000 to 550 triangles. Leave it on
  for anything that bends across several bones (the linter warns).
- A node may carry two more numbers, `[x, y, z, radius, width, thickness]`: that node's
  cross-section is scaled across and through independently (the part's `flatten` and deform
  still apply on top). One part can then taper one way only: a wedge, a hull wide at the
  back and tall at the front, a blade that thins to an edge.
- `hard: true` joins the part to the parts before it with a plain boolean (a crisp edge)
  instead of Dust3D's smooth seam bridge. Use it for machines and props; the seam check
  skips hard joins.
- `mirror: true` adds the X-mirrored copy. Model the left side (x > 0) with `Left`
  bones; the copy gets `Right`. A mirrored part must not cross x = 0. For a separate
  (`Uncombined`) piece this includes its radius: two halves that touch at the midline leave
  non-manifold edges (the linter warns). Put midline details on one unmirrored part at x = 0.
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
- `metallic` and `roughness` (0..1) and `emissive` (glow, 0..2: the part's colour times this
  goes into the emissive map) are per part: Dust3D paints them into the exported model's
  metal/roughness and emissive textures, so a game engine shows steel as steel and lamps lit.
- `smooth` (normal smoothing cutoff in degrees: 0 faceted, 60-90 stylized, 120+ very
  smooth), `disabled` (kept in the document, not in the mesh).
- `slot: "armor/2"` makes the part equipment (see "Game-ready assets" below).

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

### 5. Hard-surface shapes — machines, props, armour, faces

Organic parts are round tubes. For anything built rather than grown, use a `shape` entry: it
expands into ordinary tube parts with a custom polygon cut face, flat ends, no subdivision, no
extra rings, `smooth: 30` and `hard: true`, placed and sized in world units, so you never
work out the cut face's frame yourself. Linting, seams, variants, rigging and decompiling
treat the result like any other part.

```json
{"shape": "box", "name": "chassis", "center": [0, 0.46, 0.12], "size": [0.4, 0.18, 0.42],
 "axis": "z", "taper": [0.85, 0.8], "bevel": 0.025, "bones": "Chest"}
{"shape": "beam", "name": "leg", "path": [[0.13, 0.44, 0.3], [0.38, 0.58, 0.4], [0.58, 0.02, 0.58]],
 "profile": "I", "width": 0.07, "height": 0.09, "taper": [1, 0.85, 0.45], "mirror": true,
 "bones": ["FrontLeftCoxa", "FrontLeftTibia"]}
{"shape": "cylinder", "name": "drill", "from": [0, 0.37, 0.46], "to": [0, 0.28, 0.72],
 "radius": 0.065, "radius_to": 0.008, "sides": 8}
{"shape": "plate", "name": "armour", "center": [0.2, 0.46, 0.12], "normal": [1, 0, 0],
 "size": [0.3, 0.12], "thickness": 0.018, "bevel": 0.004}
{"shape": "bolts", "points": [[0.21, 0.51, 0], [0.21, 0.41, 0]], "normal": [1, 0, 0], "radius": 0.01}
{"shape": "groove", "from": [-0.1, 0.1, 0.3], "to": [-0.1, 0.5, 0.3], "normal": [0, 0, 1], "width": 0.015}
```

| shape | what it makes | main keys |
|---|---|---|
| `box` | a bevelled box, or a frustum/wedge with `taper` | `center`, `size` ([x, y, z] with `axis` "x"/"y"/"z", or [width, height, length] with an `axis` vector and `up`), `bevel`, `taper` (number, or [width, height] at the far end), `profile` |
| `beam` | a profile swept along a path, mitred at the bends | `path`, `profile` (rect, I, L, T, U, trapezoid, ngon, polygon), `width`, `height`, `thickness`, `web`, `top`, `taper` (number or one per point), `up` |
| `prism` | your own profile `points` [[side, up], ...] in metres | as beam |
| `cylinder` | an n-sided post, pipe, lens or cone | `from`, `to`, `radius`, `radius_to`, `sides` |
| `plate` | a thin panel lying on a surface, lifted clear of it | `center`, `normal`, `size` [w, h], `thickness`, `lift`, `up` |
| `bolts` | hex bolt heads sunk into a surface | `points`, `normal`, `radius`, `height` |
| `groove` | a cut (Inversion) for panel lines, straddling the surface | `from`, `to`, `normal`, `width`, `depth` |

How they combine:

- **Kitbash by default.** Each shape is its own closed shell (`Uncombined`) that simply
  overlaps its neighbours, as game props are built. There is no boolean to fail, and
  overlapping shells cost only a few hidden faces.
- **Carving switches a list to booleans.** In a list (the model, or a group) that holds a
  groove or an `Inversion` part, the shapes are unioned instead (hard, crisp edges) so the
  cutter has one surface to carve. Keep the carved piece and its grooves in their own group:
  many booleans in one run are where Dust3D's CSG gets fragile.
- `mirror: true` writes an explicit mirrored copy (`<name>_mirror`, Left/Right bones swapped),
  a separate part you can see, override or remove by name.
- Don't end one shape exactly where another begins (a rotor hub centred on an arm's end):
  coincident cap vertices weld into open edges. Push one a few millimetres into the other.
- Multi-bone rigid parts (a leg beam spanning coxa, femur and tibia) bend only at their own
  nodes, which is right for machines.
- Hard shapes also work on organic models: a brow ridge, jaw, armour slab or backpack on a
  person or animal. Give such a part `"hard": false, "smooth": 45` if it should still shade
  softly.

### Variants — families that share a rig and a design

```json
{"extends": "wolf.tuned.json", "name": "hell_hound",
 "recolor": {"#6f6a63": "#4a2620"}, "remove": ["mane"],
 "override": {"tail": {"color": "#1a1412"}}, "add": [{"name": "horn", ...}], "scale": 1.15}
```

A variant names its base (relative path; the base may extend another spec) and lists only
what differs: `recolor` (every colour equal to a key), `remove` (parts or groups by name),
`override` (replace fields of a named part or group), `add` (extra parts at the top level)
and `scale` (every node and radius). Other keys (`name`, `animations`, `defaults`...)
replace the base's. Extend the base's **tuned** spec (`*.tuned.json`), so the variant
inherits its clean seams and only the new parts need tuning.

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
  moves `Uncombined` parts and groups into a trailing group automatically
  (`"autoOrder": false` keeps your order exactly).
- **Carving splits runs.** An `Inversion` part ends the solid run: the solid parts after it
  are unioned only with each other, then merged with the carved result in one boolean, so
  limbs "don't touch" their body. Put the carved part and its carvers in their own group
  (`{"name": "head", "group": [skull, jaw, socket]}`); the linter flags the problem.
- **Decoration doesn't need a boolean.** Ribs, stripes, armour shells, hats, capes, wings and
  props that overlap the body are best `Uncombined`: skinned to their bone like everything
  else, with no seam to get wrong. Keep booleans for joins that must read as one surface.
- **A boolean can fail** on coincident or grazing surfaces. Dust3D then drops the part; the
  seam report says `boolean failed` for it and the export exits 1. `--tune-seams` shifts or
  resizes that part slightly; by hand, move it a few millimetres or change its radius.
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

### Clip timing, and using the models in a game engine

Each clip is generated with its type's own timing (a walk cycle 1 s / 30 frames, a slam
0.9 s / 48 frames, a death 1.2 s...), the same as a clip added in the editor. Override it
with `params` (`{"durationSeconds": 1.5, "frameCount": 45}`), e.g. to match an attack to the
game's attack interval.

`build` writes `<name>_clips.json` next to the `.glb`: every clip's name, type, duration,
frame count and whether it loops. Game engines don't know which clips loop, so read this
manifest on import. Dust3D samples a clip at `t = i / frameCount * duration`, which leaves the
last key one frame short: set each clip's length to `durationSeconds` and looped clips wrap
seamlessly. Name clips by what the game does with them (`idle`, `walk`, `attack`, `die`)
using `{"type": ..., "name": ...}`; a flying monster can use the same type for two names.

The Spider rig (spiders, crabs, scorpions) has `SpiderAttack`: rear up, lift the front legs,
curl the abdomen (a scorpion tail), lunge and slam. Tune it with `lungeDistanceFactor`,
`rearHeightFactor`, `frontLegRaiseFactor`, `pedipalpStrikeFactor`, `abdomenCurlFactor`
and `strikeTimingFactor`.

The Snake rig has `SnakeStrike`: raise the front third of the body, coil back into an S,
lunge and snap the jaw. Tune it with `liftHeightFactor`, `coilFactor`,
`lungeDistanceFactor`, `jawOpenFactor` and `strikeTimingFactor`.

The Biped rig has `BipedHop`, a looping two-legged hop in place (kangaroos, wallabies, hopping
birds; use it for both walk and run with different params). Tune it with `hopHeightFactor`,
`strideFactor`, `crouchDepthFactor`, `groundTimeFactor`, `leanForwardFactor`,
`tailSwingFactor`, `armTuckFactor` and `hopsPerCycle`.

## Game-ready assets: outfits, clothes, posed clips, events, budgets

Everything here is done by `build` after Dust3D exports (`dust3d_agent/gamekit.py`), using
the part labels Dust3D writes into the glTF: every vertex carries a `_PART` attribute, an index
into `meshes[0].extras.dust3dParts` (each part's component id and name).

**Clothes and armour that never poke through: `wrap`.** A garment built on the body part's own
chain bends at the same nodes by the same bones, so it stays outside the body in every pose:

```json
{"wrap": "torso", "name": "vest", "offset": 0.015, "range": [0.1, 0.9], "color": "#6a4a30"}
{"wrap": "arm", "name": "sleeve", "offset": 0.01, "range": [0.0, 0.35], "flare": 0.006}
{"wrap": "head", "name": "cap", "offset": 0.016, "range": [0.66, 1.0], "rounded": true, "metallic": 0.45}
```

`range` is the stretch of the body chain to cover (fractions of its length), `offset` the
thickness over the body, `flare` / `flareStart` extra radius at the ends (cuffs, skirts,
collars). It copies nodes, bones, mirror, cut face and deform from the body part; anything
else is an ordinary part key. Defaults: `combine: Uncombined`, flat ends. Keep head gear above
the eyes (nose and brows poke through a wrap that covers the face).

**Equipment slots: `"slot": "<slot>/<variant>"`.** Parts with a slot are exported as their own
mesh per variant, `slot_<slot>_<variant>`, skinned to the same skeleton; the game shows one
variant per slot. Variant `0` is by convention what shows when the slot is empty (hair and a
class's own headband as `helmet/0`, hidden when any helmet is on). A `group` can carry `slot`
for all its parts. In the Dust3D editor the slot is the "Equipment" field of a part (stored as
a component-name suffix, `vest @armor/2`), so hand-made models work the same way. The report
lists `game.slots` (triangles per variant) and `<name>_outfits.png` shows each outfit.

**Skin weights: `"smoothWeights": 2`** (top level) blends bone weights across joints over two
rings of vertices (up to 4 bones per vertex): softer shoulders and hips. Metal (`metallic` >= 0.5)
and `hard` parts keep rigid weights.

**Posed clips.** Besides the rig's generated clips, key your own from a few poses:

```json
{"type": "Pose", "name": "cheer", "base": "idle", "durationSeconds": 1.4, "loop": false,
 "keys": [{"t": 0, "pose": {}},
          {"t": 0.35, "pose": {"LeftUpperArm": [0, 0, 150], "RightUpperArm": [0, 0, -150],
                               "Hips": {"move": [0, 0.03, 0]}}},
          {"t": 1.4, "pose": {}}]}
```

Rotations are degrees about the world X, Y, Z axes as seen in the rest pose (+X pitches the
top of a bone toward +Z, the front; +Z swings a left arm up and out), applied on top of the
`base` clip (its motion keeps playing underneath) or the rest pose. `move` shifts a bone
(usually `Hips`). Keys are eased in and out.

**Clip events** go into `<name>_clips.json` as `"events": [{"name", "time", "bone"}]`: `hit`
for attack-type clips (when the fastest limb peaks) and `step` for walks, runs and hops (each
foot touching down), found from the motion. Give your own with `"events": {"hit": 0.45}`
(fractions of the clip) on any animation; they replace the automatic ones of that name.

**Triangle budget: `"budget": 4200`** (top level) fails the report's `game.budget.ok` when the
body plus the heaviest variant of every slot is over it. Game engines usually generate LODs on
import, so budget the close-up model.

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
| `game.slots`, `game.budget` | Equipment variants and their triangles; the heaviest outfit against `budget`. |
| `game.weights` | How many vertices the weight smoothing changed. |

Pictures: `turnaround` (front / left / top / three-quarter), `skeleton` (bones over the
mesh), `anim_<clip>` (6 frames). In the front view the creature's left is image-right.

## Checklist before declaring done

- [ ] lint has only `info:` / `advisory:` lines; `warnings` empty
- [ ] silhouette recognizable in the front **and** left views
- [ ] islands = 1 + Uncombined parts
- [ ] `bad_seams` empty (use `--tune-seams`); seam close-ups show clean strips
- [ ] skeleton: every bone inside its limb
- [ ] each animation strip shows plausible motion, with no spikes or collapsing vertices
