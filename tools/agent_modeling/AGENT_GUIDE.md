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
- A closed loop drawn without `fillInterior` is an opening, and stays one: an eye between its
  lids is left open so the eyelids can close over the eyeball when the eyes blink
  (`headHasEyelids`). Keep loops from crossing each other in the front (XY) view; where they
  do, the projected triangulation leaves slits that are not meant to be there.
- **A character's face** can be modelled on its own and imported as the head: a `backClosed`
  loop mask with eyeball, iris and eyelid parts, built to a `.glb`, then used as the character's
  `head` part with its colour atlas as the `image`:
  `{"name": "head", "import": "face.glb", "image": "face.png", "combine": "Uncombined",
  "rounded": false, "nodes": [[0, chin_y, z, r], [0, crown_y, z, r]], "bones": ["Head"]}`.
  The mesh's height runs along the two nodes and its width is scaled to `2 r`; run the body's
  neck chain up inside it, and give the back of the head a plain `skull` part (hair, hats and
  helmets can `wrap` it). The build report counts an imported part's open edges (the eye
  openings) as intended, like a garment's rims.

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

### 6. Skin modifier — one seamless body, garments that fit it

A group with `"skin"` is turned into **one new surface wrapped around everything its
children generate** (tubes, stitched shells, imported meshes, nested groups): no booleans,
no seams, no inner faces, a face count you set. The surface is all quads with edge flow:
Dust3D remeshes it with the AutoRemesher core (a curvature aligned cross field and a quad
parameterization), so edge loops run around the limbs, the neck and the torso, and a garment's
openings are cut cleanly along their crease.

```json
{"name": "body", "color": "#e2c4b0", "skin": {"mode": "creature", "smoothness": 0.014, "faces": 2800},
 "group": [ ...torso, bust, neck, arms, hands, legs, feet... ]}
{"name": "tunic", "slot": "chest/1", "combine": "Uncombined", "color": "#5d86b3",
 "skin": {"mode": "cloth", "keep": false, "bindTo": "body", "offset": 0.009, "smoothness": 0.03,
          "drape": 0.7, "drapeLength": 0.08, "thickness": 0.0025, "faces": 1050},
 "group": [{"wrap": "torso", "name": "tunic_torso", "offset": 0, "range": [0.27, 1], "combine": "Normal"},
           {"wrap": "bust", "name": "tunic_bust", "offset": 0, "combine": "Normal"},
           {"wrap": "arm", "name": "tunic_sleeve", "offset": 0, "range": [0, 0.42], "combine": "Normal"},
           {"wrap": "neck", "name": "tunic_neckline", "offset": 0.016, "combine": "Inversion"}]}
```

- `mode`: `"creature"` — a tight skin over bones and muscle shapes (the children are replaced
  by it); `"cloth"` — a loose garment (by default worn *over* the children, which stay).
- `smoothness` — blend radius between children: creases narrower than about half of it fill
  in (a bust into the chest, a shoulder into the torso). Keep it small (0.01-0.02) on a body,
  or close limbs web together; larger (0.03-0.05) on cloth bridges gaps (no cleavage).
- `offset` — distance over the children. Garments: at least 0.005 over the body skin (both
  surfaces are approximations; less and the body shows through at rest), and layers 0.004
  apart (a tunic 0.009 over trousers 0.005).
- `drape` (cloth, 0..1) — how straight the cloth falls from an overhang (a bust, the hips)
  instead of following the body back in. `drapeLength` limits the fall (0 = to the bottom of
  the group): a tunic 0.08, a skirt unlimited with `drape` 0.9.
- **Openings** (cloth): an `"Inversion"` child cuts an opening of its shape — a neckline
  (a copy of the neck, `offset` 0.015-0.03 = how low it is cut), armholes, a slit. Flat cuts:
  `openTop` / `openBottom` (fractions of the group's height). `thickness` gives the rim a
  hem: an inner lip two faces deep, not a full inner shell (no triangles wasted inside).
  A garment with no opening is closed; where a limb leaves it the closed end reads as a hem.
- `keep` — show the children too (`true` is the cloth default). A garment over **part** of
  the body (a top must not swallow the arms) wraps *guides*: copies of the body chains it
  covers, made with the existing `wrap` element at `offset` 0 and `"combine": "Normal"`, with
  `keep: false`. `range` picks the stretch (a short sleeve = the first 40% of the arm).
- `bindTo` — **give every garment `"bindTo": "<body group>"`**. Its skin weights then come
  from the body's own parts, through the same blend, so body and garment bend alike and the
  body stays inside in every pose. Without it the weights come from the group's children.
- `faces` — the quad count of the surface (before the hem lip; for a garment, of what is
  kept after its openings are cut). Body 2500-3500, garments 500-1200. A part much thinner
  than a quad (a hand at a low budget) gets few loops: raise `faces` rather than shrink it.
- Keep garments at least a few quads across: a narrow band cut out of a big shape by two flat
  cuts is mostly cut away, and too thin to carry edge loops. Make such a garment closed from
  guides of its own size instead (its closed ends sit against the body like a hem). `weightRadius` (default 0.03) is the joint blend distance, in world units.
- Weights blend only between nodes near each other *in the body* (along a part, or where two
  parts' node spheres overlap, never between a left and right twin), so gaps never bleed:
  but a garment surface that **bridges** two limbs (boots fused between the heels) stretches
  between them. Keep limbs a cell apart: the rest pose stands with feet slightly apart and
  arms clear of the ribs (an A-pose), and close garments use small `smoothness`.
- In the Dust3D editor this is the **Skin Modifier** box of a group's properties (mode,
  settings, Keep Children, Weights From, Faces); the document stores it on the group as
  `wrap`, `wrapOffset`, `wrapSmoothness`, `wrapDrape`, `wrapDrapeLength`, `wrapOpenTop`,
  `wrapOpenBottom`, `wrapThickness`, `wrapFaces`, `wrapKeep`, `wrapBindTo`, `wrapWeightRadius`.

### Variants — families that share a rig and a design

```json
{"extends": "wolf.tuned.json", "name": "hell_hound",
 "recolor": {"#6f6a63": "#4a2620"}, "remove": ["mane"],
 "override": {"tail": {"color": "#1a1412"}}, "add": [{"name": "horn", ...}], "scale": 1.15}
```

A variant names its base (relative path; the base may extend another spec) and lists only
what differs: `recolor` (every colour equal to a key), `remove` (parts or groups by name),
`override` (replace fields of a named part or group), `add` (extra parts at the top level),
`addTo` (extra children for a named group: `{"body": [bust...], "tunic": [bust guide...]}`, the
way to change what a skin modifier wraps, e.g. a woman's figure from a shared base) and `scale`
(every node and radius). Other keys (`name`, `animations`, `defaults`...)
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
- **Held props and clothes don't move bones.** A bone is placed from the body parts assigned to
  it; parts that are not unioned into the body (`Uncombined`, or inside an Uncombined group: a
  spear, a club, a hat, a garment's guides) ride on the bone without stretching it, as long as
  the bone has body parts too. Give a spear's edges `LeftHand` and the hand bone still runs from
  wrist to fingertips, so every clip that aims the hand aims the hand.
- Animations: `["QuadrupedWalk", {"type": "QuadrupedRun", "name": "run", "params": {...}}]`.
  Tune amplitude-like params for unusual bodies (the goldfish swim uses
  `spineAmplitude 0.035`; the T-rex walk `armSwingFactor 0.2` for tiny arms).

### Clip timing, and using the models in a game engine

Each clip is generated with its type's own timing (a walk cycle 1 s / 30 frames, a slam
0.9 s / 48 frames, a death 1.2 to 1.8 s...), the same as a clip added in the editor. Override it
with `params` (`{"durationSeconds": 1.5, "frameCount": 45}`), e.g. to match an attack to the
game's attack interval.

`build` writes `<name>_clips.json` next to the `.glb`: every clip's name, type, duration,
frame count and whether it loops. Game engines don't know which clips loop, so read this
manifest on import. Dust3D samples a clip at `t = i / frameCount * duration`, which leaves the
last key one frame short: set each clip's length to `durationSeconds` and looped clips wrap
seamlessly. Name clips by what the game does with them (`idle`, `walk`, `attack`, `die`)
using `{"type": ..., "name": ...}`; a flying monster can use the same type for two names.

### A game's clip set, per rig

A creature in a game needs at least `idle`, `walk` (or a flyer's `fly`), `attack`, `hurt` and
`die`, and usually `run`. Every rig has all of them:

| rig | idle | move | attack | hurt | die | also |
|---|---|---|---|---|---|---|
| Biped | `BipedIdle` | `BipedWalk`, `BipedRun`, `BipedHop` | `BipedSlam`, `BipedStab`, `BipedKick`, `BipedCast`, `BipedThrow` | `BipedHurt` | `BipedDie` | `BipedJump`, `BipedRoar`, `BipedChannel` |
| Quadruped | `QuadrupedIdle` | `QuadrupedWalk`, `QuadrupedRun` | `QuadrupedAttack` | `QuadrupedHurt` | `QuadrupedDie` | `QuadrupedRoar`, `QuadrupedEat` |
| Bird | `BirdIdle` (ground) | `BirdWalk`, `BirdRun`, `BirdFly`, `BirdGlide` | `BirdStrike` (ground), `BirdAttack` (flying dive) | `BirdHurt` | `BirdDie` | `BirdEat` |
| Insect | `InsectIdle` | `InsectWalk`, `InsectFly` | `InsectBite` (ground), `InsectAttack` (flying dive) | `InsectHurt` | `InsectDie` | `InsectRubHands` |
| Spider | `SpiderIdle` | `SpiderWalk`, `SpiderRun` | `SpiderAttack` | `SpiderHurt` | `SpiderDie` | |
| Snake | `SnakeIdle` | `SnakeSlither` | `SnakeStrike` | `SnakeHurt` | `SnakeDie` | |
| Fish | `FishIdle` | `FishSwim` | `FishAttack` | `FishHurt` | `FishDie` | |

A run is often the walk type with other parameters (`gaitSpeedFactor: 2`, a longer
`stepLengthFactor`; a faster `FishSwim`). A flyer uses its fly clip as idle and walk.

What the game clips do, and their main parameters:

- **Hurt** (all rigs): the body snaps away from the blow over about 0.1 s, holds for a
  couple of frames (hit-stop), then settles back with one small overshoot, ending exactly at
  rest. Feet stay planted. `recoilFactor`, `hitDirection` (-1 left, 0 front, 1 right),
  `recoverySpeed`, plus per rig: `flinchFactor`, `neckWhipFactor`, `wingFlareFactor`,
  `kinkFactor`, `tailThrashFactor`, `frontLegGuardFactor`, `legScrabbleFactor`... For a flyer
  set `airborne: 1` (`BirdHurt`, `InsectHurt`): the reaction is layered on the flying wing
  beat, so the wings keep beating.
- **Die** (all rigs): one authored death per rig, ending still on the ground, timed with
  gravity: the blow jolts the body, the legs give way (the hips land first, or the body
  drops onto its chest), the body topples or rolls over and hits the ground with a small
  bounce, and the head and limbs follow a moment later and settle. Lying, the limbs are
  half bent and not symmetric (a held weapon lies flat); nothing sinks below the ground.
  `collapseSpeedFactor`, `groundBounce`, `headDropFactor`, and per rig:
  - Biped: `fallDirection` (-1 onto its back, 1 face down, 0 onto its side, for
    big-tailed bipeds), `fallSide`, `legBuckleFactor`, `armFlailFactor`.
  - Quadruped: `fallSide`, `rollIntensityFactor` (1 = onto its side, 2 = onto its back: a
    dead lizard), `legBuckleFactor`, `legSpreadFactor`.
  - Bird: `airborne: 1` for flyers (failing wing beats, a limp fall nose down with the wings
    trailing up, lands breast first at 0.62 of the clip with the wings spread flat; drop the
    model from its flying height to land then), `wingSpreadFactor`, `wingFlapFactor`.
  - Spider / Insect: `legCurlFactor` (1 = the death curl, 0 = legs splayed flat, e.g. a
    mechanical walker), `flipOver` (1 = rolls onto its back, legs folded over the belly),
    `twitchFactor`; the abdomen (a scorpion's tail) falls over to one side.
  - Snake: `thrashFactor`, `flipAngle` (degrees rolled belly-up), `jawOpen`.
  - Fish: `onGround: 1` with `flipAngle: 90` for a fish dying on land or the bottom (drops
    onto its side and flops, head and tail lifting, a few times); otherwise it thrashes and
    rolls belly-up in water. `hitIntensityFactor`, `hitFrequency`, `spinDecay`.
- `QuadrupedAttack`: rock back, lunge off the planted hind feet while the front feet step
  forward, head snap and bite, step back. `chargeDistanceFactor`, `headDropFactor`,
  `headStrikeIntensity`, `jawOpenFactor`, `spineCompressionFactor`, `tailWhipFactor`,
  timing `anticipationDuration` / `strikeMoment` / `strikeEnd`.
- `BipedSlam` (two-handed overhead blow), `BipedStab` (thrust with a guard arm),
  `BipedCast` (gather, then push both hands out): arm targets are directions, so they work
  for arms that hang down at rest (A-pose) and arms held out (T-pose).
- `BipedKick`: knee up, snap the foot out, retract, step down. `kickHeightFactor`,
  `kickReachFactor`, `chamberFactor`, `leanBackFactor`, `armBalanceFactor`, `kickLeg`;
  `bothLegs: 1` is a kangaroo's kick, the body rocking back onto its tail.
- `BipedThrow`: a spear, boomerang or stone hurled at the target. The front foot steps into a
  wide stance while the body coils away and the throwing arm cocks back (upper arm out level,
  forearm up, the hand beside the head, the weapon held level and aimed) and the other arm points
  at the target; then the hips and chest uncoil, the arm whips through and follows across the
  body. The wrist turns the held weapon onto the target from whatever angle it is modelled at
  (`weaponPitch`: 0 = carried upright, 90 = pointing forward), keeping its flat side to the side,
  so the roll never flips between frames. `throwArm` (0 = the modelled left hand), `sidearmFactor`
  (1 = a low, flat boomerang throw), `crouchFactor`, `windupFactor`, `twistFactor`, `leanFactor`,
  `stepFactor`, `offArmPointFactor`, `weaponAimFactor`, `aimHeightFactor`, `releaseTimingFactor`.
  The `hit` event lands at the release.
- `BirdStrike`: rear up with the neck cocked and wings flared, then a peck (`peckFactor`)
  and/or a forward kick (`kickFactor`: emus, cassowaries).
- `InsectBite`: rear up on the back legs, lunge and snap the head (jaws) down, the abdomen
  curling forward to sting (`abdomenCurlFactor`). Needs no wings.
- `InsectAttack`: a flying dive layered on `InsectFly` (so it takes the fly parameters too).
  `InsectFly` has `wingBeatFactor` (beats per cycle, x3) and `wingFlapFactor`; fast beats need
  more frames (60 per second) or they strobe.
- `FishAttack`: S-coil back, dart forward with one hard tail stroke, bite (with a `Jaw`
  bone), head shake. `lungeDistanceFactor`, `coilFactor`, `tailBeatFactor`, `biteShakeFactor`.
- `SpiderAttack`: rear up, lift the front legs, curl the abdomen (a scorpion's tail), lunge
  and slam. `lungeDistanceFactor`, `rearHeightFactor`, `frontLegRaiseFactor`,
  `pedipalpStrikeFactor`, `abdomenCurlFactor`, `strikeTimingFactor`.
- `SnakeStrike`: raise the front third, coil back into an S, lunge and snap the jaw.
  `liftHeightFactor`, `coilFactor`, `lungeDistanceFactor`, `jawOpenFactor`, `strikeTimingFactor`.
- `BipedHop`: a looping two-legged hop in place (kangaroos, hopping birds; walk and run with
  different params). `hopHeightFactor`, `strideFactor`, `crouchDepthFactor`,
  `groundTimeFactor`, `leanForwardFactor`, `tailSwingFactor`, `armTuckFactor`, `hopsPerCycle`.

Loops are whole cycles (speed factors round to whole cycles per clip), so they wrap without a
seam. One-shots (attack, hurt, cast) start and end at the rest pose, so a game can blend
them in from any loop and back.

### Checking clips the way a game plays them

`build` checks every clip frame by frame (in `metrics.animations`, failures in `warnings`):

- **pops**: a frame that jumps much further than the frames within two on either side of it
  (a fast strike that speeds up and slows down is fine; a limb teleporting is not);
- **loop wraps**: the jump from a loop's last frame back to its first, held to a finer
  standard (an idle that twitches once per cycle shows);
- **one-shot ends**: an attack or hurt must end within ~6% of the model size of the rest pose
  or a frame of the base loop (`idle`, else the first loop, e.g. a flyer's `fly`); engines
  blend back over ~0.15 s and a bigger gap reads as a snap. Deaths are exempt.

Fix a warning by tuning the clip's parameters (a smaller amplitude, a longer duration, more
frames for fast wing beats), not by loosening the check.

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

**Garments that are one surface each: skin modifier groups** (building block 6). For a whole
outfit: one `creature` body; every garment a `cloth` group with `keep: false`, `bindTo` the body,
`combine: Uncombined` and a `slot` (on the group: the garment is one mesh). Variant `0` of a slot
is the underwear that shows when the slot is empty. Building block 6 above shows a body and a tunic bound to it.

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
| `metrics.animations[].max_frame_step_rel`, `loop_seam_rel`, `end_offset_rel` | Frame-to-frame motion, a loop's wrap, a one-shot's distance from rest at its end (see "Checking clips"). |
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
- [ ] outfits: every garment of a skin-modifier outfit has `bindTo` the body; `game.budget.ok`
