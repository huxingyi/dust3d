---
name: dust3d-generate-from-image
description: Generate a Dust3D 3D model from a 2D turnaround reference image by tracing body parts as tube-shaped node chains, writing an XML file, and running dust3d to produce a .obj mesh, a .ds3 document, and the source .xml file.
---

# Dust3D Generate 3D Model from 2D Image

Given a turnaround reference image, generate a Dust3D XML node file, run the `dust3d` CLI to produce a `.obj` mesh and a `.ds3` document file, and verify the result. Three output files are produced: `nodes.xml` (the source node graph), `result.ds3` (the Dust3D project file), and `result.obj` (the exported 3D mesh).

---

## Overview of the Workflow

1. Analyze the reference image and identify all body parts.
2. For each part, trace a tube shape using a chain of nodes sampled from both the front view and the side view.
3. Write `nodes.xml` in Dust3D canvas XML format.
4. Run `dust3d -paste-xml nodes.xml -output result.ds3 -output result.obj`.
5. Optionally verify by projecting `result.obj` in front and side views.

---

## Step 1 — Understand the Reference Image Layout

A turnaround reference image places the **front view on the left** and the **side view on the right**, both at the same vertical scale and vertical center.

Normalize all image coordinates to the range `[0.0, 1.0]` relative to the full image width and height:

```
normalized_x = pixel_x / image_width
normalized_y = pixel_y / image_height
```

Identify the body's bilateral symmetry axis and vertical midpoint to set the canvas origin:

| Canvas attribute | Meaning |
|---|---|
| `originX` | Normalized X of the **body's bilateral symmetry axis** (left-right center of the body itself) as it appears in the front view. This is the axis used by `xMirrored` — parts are reflected across this X value. It must be placed at the actual center of the body, **not** at the center of the front-view panel in the image. If the front view panel spans from pixel 0 to pixel W/2, the body center may or may not be at W/4; measure the body's widest extent and take its midpoint. |
| `originY` | Normalized Y of the model's vertical mid-point (shared by both views) |
| `originZ` | Normalized X of the **body's depth center** in the side view (expressed as an absolute canvas X, which will be > `originX` since the side view is to the right) |

---

## Step 2 — Decompose the Model into Parts

Split the model into anatomically distinct tube-shaped parts. For a quadruped, typical parts are:

- **Body** — the torso/spine running front-to-back
- **Neck** — connecting torso to head
- **Head** — the skull volume
- **Jaw** — lower jaw (if needed)
- **Front left leg** — upper + lower segments, or one continuous tube
- **Front right leg** — mirror of front left, or use `xMirrored` on the front left
- **Rear left leg**
- **Rear right leg** — mirror of rear left
- **Tail** — one or more segments (TailBase, TailMid, TailTip)
- **Ears / fins / wings** — any additional appendages

Use `xMirrored="true"` on a part to have Dust3D automatically produce its mirror image across the X axis — this means you only need to model **one side** for symmetric parts (e.g., define the left leg only and mirror it for the right).

---

## Step 3 — Trace Each Part with Nodes

For each part:

1. In the **front view** (left half of the image) identify ~5–15 control points along the center-line of the part, spaced more densely where curvature changes.
2. In the **side view** (right half of the image) identify the same number of control points along the depth of that part.
3. Each control point pair becomes one **node**. The node stores:
   - `x` — normalized X from the front view
   - `y` — normalized Y from the **front view** (use the same Y as the front-view sample; both views share the same vertical scale)
   - `z` — normalized X from the **side view** (this encodes depth)
   - `radius` — half the visible width of the body at that point as seen in the front view, normalized: `radius = visible_width_pixels / (2 * image_width)`

> **Tip:** For left-side parts that will be mirrored, place the nodes slightly to the left of `originX` in the front view so they sit on the correct side.

---

## Step 4 — Write the XML File

The XML file has four sections: `nodes`, `edges`, `parts`, `components`.

### 4.1 Generate UUIDs

Each node, edge, and part needs a unique id formatted as `{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}`. Generate them with Python or any UUID library:

```python
import uuid
def new_id():
    return "{" + str(uuid.uuid4()) + "}"
```

### 4.2 XML structure

```xml
<?xml version="1.0" encoding="UTF-8"?>
<canvas originX="FRONT_CENTER_X" originY="MODEL_CENTER_Y" originZ="SIDE_CENTER_X" rigType="Quadruped">
 <nodes>
  <!-- One <node> per control point -->
  <node id="{NODE_ID}" partId="{PART_ID}" radius="RADIUS" x="X" y="Y" z="Z"/>
  ...
 </nodes>
 <edges>
  <!-- Connect consecutive nodes within a part in order -->
  <edge id="{EDGE_ID}" partId="{PART_ID}" from="{NODE_A_ID}" to="{NODE_B_ID}"/>
  <!-- Optional: boneName="Spine" etc. for rig bone labeling -->
  ...
 </edges>
 <parts>
  <!-- One <part> per tube shape -->
  <part id="{PART_ID}"
        chamfered="false"
        disabled="false"
        locked="false"
        visible="true"
        rounded="true"
        subdived="true"
        xMirrored="false"
        deformThickness="1.0"
        deformWidth="1.0"
        cutFace="Quad"/>
  ...
 </parts>
 <components>
  <!-- One <component> per part, links display color to part -->
  <component id="{COMPONENT_ID}"
             linkData="{PART_ID}"
             linkDataType="partId"
             color="#ffccbbaa"
             combineMode="Normal"
             expanded="false"
             smoothCutoffDegrees="180.000000">
  </component>
  ...
 </components>
 <animations>
 </animations>
</canvas>
```

### 4.3 Part attributes reference

| Attribute | Values | Effect |
|---|---|---|
| `rounded` | `true` / `false` | Smooth round end-caps |
| `subdived` | `true` / `false` | Catmull-Clark subdivision for smoother mesh |
| `xMirrored` | `true` / `false` | Mirror part across the X (left-right) axis |
| `deformThickness` | `0.0` – `2.0` (default `1.0`) | Scale the tube along the **normal of the part's base plane**. The base plane is derived by averaging the angles of all edges in the tube (e.g., for an arm, the plane roughly follows the limb's swept surface), and `deformThickness` pushes in/out along that plane's normal |
| `deformWidth` | `0.0` – `2.0` (default `1.0`) | Scale the tube **within** the base plane, orthogonal to `deformThickness` |
| `cutFace` | `Quad`, `Triangle`, `Pentagon`, `Hexagon` | Cross-section polygon shape; `Pentagon` gives more organic results for tails/limbs |
| `chamfered` | `true` / `false` | Chamfer (bevel) the edges of the `cutFace` cross-section along the tube |

### 4.4 Edges

Connect the nodes of each part in a single chain from one end to the other. The order of `from`→`to` does not affect the mesh but should follow the natural direction of the part (e.g., head-to-tail for the spine).

Use `boneName` on edges to label rig bones. Common names for a quadruped: `Spine`, `Neck`, `Head`, `Jaw`, `TailBase`, `TailMid`, `TailTip`.

---

## Step 5 — A Minimal Working Example

Below is a two-part skeleton: a body tube and a mirrored front leg, for an animal whose front view center is at x=0.33 and whose side view occupies the right portion of the canvas.

```xml
<?xml version="1.0" encoding="UTF-8"?>
<canvas originX="0.330000" originY="0.500000" originZ="1.500000" rigType="Quadruped">
 <nodes>
  <!-- Body (spine), 4 nodes front-to-back -->
  <node id="{a0000001-0000-0000-0000-000000000001}" partId="{b0000001-0000-0000-0000-000000000001}" radius="0.120" x="0.330" y="0.420" z="1.100"/>
  <node id="{a0000001-0000-0000-0000-000000000002}" partId="{b0000001-0000-0000-0000-000000000001}" radius="0.150" x="0.330" y="0.430" z="1.300"/>
  <node id="{a0000001-0000-0000-0000-000000000003}" partId="{b0000001-0000-0000-0000-000000000001}" radius="0.145" x="0.330" y="0.440" z="1.500"/>
  <node id="{a0000001-0000-0000-0000-000000000004}" partId="{b0000001-0000-0000-0000-000000000001}" radius="0.110" x="0.330" y="0.430" z="1.700"/>
  <!-- Front left leg, 3 nodes top-to-toe (will be mirrored for right leg) -->
  <node id="{a0000002-0000-0000-0000-000000000001}" partId="{b0000002-0000-0000-0000-000000000001}" radius="0.045" x="0.295" y="0.450" z="1.150"/>
  <node id="{a0000002-0000-0000-0000-000000000002}" partId="{b0000002-0000-0000-0000-000000000001}" radius="0.035" x="0.290" y="0.580" z="1.160"/>
  <node id="{a0000002-0000-0000-0000-000000000003}" partId="{b0000002-0000-0000-0000-000000000001}" radius="0.020" x="0.288" y="0.750" z="1.165"/>
 </nodes>
 <edges>
  <edge id="{e0000001-0000-0000-0000-000000000001}" partId="{b0000001-0000-0000-0000-000000000001}" boneName="Spine" from="{a0000001-0000-0000-0000-000000000001}" to="{a0000001-0000-0000-0000-000000000002}"/>
  <edge id="{e0000001-0000-0000-0000-000000000002}" partId="{b0000001-0000-0000-0000-000000000001}" boneName="Spine" from="{a0000001-0000-0000-0000-000000000002}" to="{a0000001-0000-0000-0000-000000000003}"/>
  <edge id="{e0000001-0000-0000-0000-000000000003}" partId="{b0000001-0000-0000-0000-000000000001}" boneName="Spine" from="{a0000001-0000-0000-0000-000000000003}" to="{a0000001-0000-0000-0000-000000000004}"/>
  <edge id="{e0000002-0000-0000-0000-000000000001}" partId="{b0000002-0000-0000-0000-000000000001}" from="{a0000002-0000-0000-0000-000000000001}" to="{a0000002-0000-0000-0000-000000000002}"/>
  <edge id="{e0000002-0000-0000-0000-000000000002}" partId="{b0000002-0000-0000-0000-000000000001}" from="{a0000002-0000-0000-0000-000000000002}" to="{a0000002-0000-0000-0000-000000000003}"/>
 </edges>
 <parts>
  <part id="{b0000001-0000-0000-0000-000000000001}" chamfered="false" deformThickness="0.900" disabled="false" locked="false" rounded="true" subdived="true" visible="true" xMirrored="false"/>
  <part id="{b0000002-0000-0000-0000-000000000001}" chamfered="false" deformWidth="0.780" disabled="false" locked="false" rounded="true" subdived="true" visible="true" xMirrored="true"/>
 </parts>
 <components>
  <component id="{c0000001-0000-0000-0000-000000000001}" linkData="{b0000001-0000-0000-0000-000000000001}" linkDataType="partId" color="#ffc7b5b4" combineMode="Normal" expanded="false" smoothCutoffDegrees="180.000000">
  </component>
  <component id="{c0000002-0000-0000-0000-000000000001}" linkData="{b0000002-0000-0000-0000-000000000001}" linkDataType="partId" color="#fffff4cd" combineMode="Normal" expanded="false" smoothCutoffDegrees="180.000000">
  </component>
 </components>
 <animations>
 </animations>
</canvas>
```

---

## Step 6 — Run Dust3D to Generate the Mesh

Save the file as `nodes.xml`, then run:

```bash
dust3d -paste-xml nodes.xml -output result.ds3 -output result.obj
```

- Exit code `0` means success.
- `nodes.xml` is the source node graph (already written in Step 4).
- `result.ds3` is the Dust3D project file, which can be reopened in the Dust3D GUI for further editing.
- `result.obj` is the exported 3D mesh.

---

## Step 7 — Verify the Result

Confirm all three output files were created:

```bash
# Quick sanity check: confirm all output files exist and are non-empty
ls -lh nodes.xml result.ds3 result.obj

# Count vertices and faces in the mesh to check plausibility
grep -c '^v ' result.obj   # vertex count
grep -c '^f ' result.obj   # face count
```

For visual verification, open `result.obj` in any 3D viewer that supports orthographic projection (e.g., Blender, MeshLab) and switch to front orthographic and right orthographic cameras. The silhouette should roughly match the front and side views of the reference image.

If a part is too flat or too round, adjust `deformThickness` or `deformWidth` on that part and re-run.

---

## Quick Reference: Coordinate System

```
Canvas X axis  →  increases left to right
Canvas Y axis  ↓  increases top to bottom (0 at top, 1 at bottom)

Front view (left portion of canvas):
  node.x = horizontal position in front view
  node.y = vertical position

Side view (right portion of canvas):
  node.z = horizontal position in side view  (larger X values = right side of canvas)
  node.y = vertical position (same scale as front view)

canvas.originX  = body's bilateral symmetry axis X in the front view
                  (NOT the front-view panel center — measure the body's
                   actual left-right extent and take its midpoint;
                   xMirrored reflects parts across this exact value)
canvas.originY  = center Y shared by both views  (model's vertical center)
canvas.originZ  = center X of side view   (model's depth center)
```

All coordinates in the XML are **absolute canvas-space** values (not offsets from origin). The origin attributes tell Dust3D where to treat as the world center.
