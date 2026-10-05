"""Model spec: the agent-facing description of a Dust3D model.

World frame used by the spec (matches Dust3D's rig templates):
    +Y up, +Z forward (the creature faces +Z), +X is the creature's LEFT side.
    Units are Dust3D canvas units; a whole creature usually fits in ~1 unit.

A spec is a JSON object:

{
  "name": "fox",
  "rig": "Quadruped",                     # optional: Biped|Quadruped|Bird|Fish|Insect|Snake|Spider
  "defaults": {"color": "#d9772b"},       # optional per-part defaults
  "parts": [
    {
      "name": "body",
      "nodes": [[x, y, z, radius], ...],  # an ordered chain; consecutive nodes are joined
      "bones": ["Pelvis", "Spine", ...],  # optional: one bone per edge, or one string for all edges
      "mirror": false,                    # true: also generate the X-mirrored copy (Left<->Right bones swap)
      "color": "#d9772b",
      "cutFace": "Quad",                  # Quad|Pentagon|Hexagon|Triangle
      "rounded": true, "subdivided": true, "chamfered": false,
      "deformThickness": 1.0, "deformWidth": 1.0, "cutRotation": 0.0,
      "metallic": 0.0, "roughness": 1.0,   # per-part PBR, exported as the model's ORM map
      "emissive": 0.0,                    # glow strength: colour x emissive -> the emissive map
      "combine": "Normal",                # Normal|Inversion (carve)|Uncombined (separate mesh, e.g. eyes)
      "loop": false,                      # close the chain into a ring
      "smooth": 60,                       # smooth-normal cutoff in degrees (0 = faceted)
      "interpolate": true                 # false: no extra rings along long edges (rigid parts, fewer triangles)
    }
  ],
  "animations": ["QuadrupedWalk", {"type": "QuadrupedRun", "name": "run", "params": {...}}]
}
"""

from __future__ import annotations

import json
import math
import os
import re
import xml.etree.ElementTree as ET
from dataclasses import dataclass, field
from typing import Any, Dict, List, Optional

ANIMATION_TYPES = {
    "Biped": ["BipedWalk", "BipedRun", "BipedIdle", "BipedJump", "BipedHurt", "BipedDie",
              "BipedRoar", "BipedSlam", "BipedStab", "BipedCast", "BipedChannel", "BipedHop", "BipedKick", "BipedThrow",
              "BipedCombatIdle", "BipedStrafeLeft", "BipedStrafeRight", "BipedWalkBackward", "BipedTurnLeft", "BipedTurnRight", "BipedJumpStart", "BipedFall", "BipedLand", "BipedSlash", "BipedBlock", "BipedDodge"],
    "Quadruped": ["QuadrupedWalk", "QuadrupedRun", "QuadrupedIdle", "QuadrupedEat",
                  "QuadrupedAttack", "QuadrupedHurt", "QuadrupedRoar", "QuadrupedDie"],
    "Bird": ["BirdWalk", "BirdRun", "BirdFly", "BirdGlide", "BirdIdle", "BirdEat",
             "BirdAttack", "BirdStrike", "BirdHurt", "BirdDie"],
    "Fish": ["FishSwim", "FishIdle", "FishAttack", "FishHurt", "FishDie"],
    "Insect": ["InsectWalk", "InsectFly", "InsectIdle", "InsectAttack", "InsectBite", "InsectHurt", "InsectRubHands",
               "InsectDie"],
    "Snake": ["SnakeSlither", "SnakeIdle", "SnakeStrike", "SnakeHurt", "SnakeDie"],
    "Spider": ["SpiderWalk", "SpiderRun", "SpiderIdle", "SpiderAttack", "SpiderHurt", "SpiderDie"],
}

ANIMATION_TYPES["Biped"] += [
    "BipedSprint", "BipedSneak", "BipedCrouchEnter", "BipedCrouchIdle",
    "BipedCrouchExit", "BipedMountedIdle", "BipedMountedRide", "BipedStunned",
    "BipedKnockdown", "BipedGetUp", "BipedDodgeRoll", "BipedSwimIdle",
    "BipedSwimForward", "BipedSwimBackward", "BipedSwimLeft", "BipedSwimRight",
    "BipedOneHandSlash", "BipedTwoHandSwing", "BipedBowDraw", "BipedBowAim",
    "BipedBowShot", "BipedParry", "BipedCastStart", "BipedCastRelease",
    "BipedCastRecover", "BipedChannelEnter", "BipedChannelExit", "BipedChannelInterrupt",
    "BipedGather", "BipedPickUp", "BipedInteract", "BipedMine",
    "BipedChop", "BipedDrink", "BipedEat", "BipedSitDown",
    "BipedSitIdle", "BipedStandUp", "BipedSleepLieDown", "BipedSleepIdle",
    "BipedWakeUp", "BipedWave", "BipedCheer", "BipedBow",
    "BipedPoint", "BipedClap", "BipedDance", "BipedTalk",
]

# Each animation type's own default clip timing (durationSeconds, frameCount), as set in
# dust3d/animation/<rig>/<clip>.cc. The editor shows 3 s / 90 frames for any clip that
# doesn't store them, so the compiler writes these explicitly: the exported clip and the
# editor preview then agree. Override per clip with params durationSeconds / frameCount.
ANIMATION_TIMING = {
    "BipedWalk": (1.0, 30), "BipedRun": (1.0, 30), "BipedIdle": (4.0, 90),
    "BipedJump": (1.2, 40), "BipedHurt": (1.0, 36), "BipedDie": (1.3, 40),
    "BipedRoar": (3.0, 120), "BipedSlam": (0.9, 48), "BipedStab": (0.7, 48),
    "BipedCast": (1.0, 48), "BipedChannel": (2.0, 64), "BipedHop": (0.6, 20), "BipedKick": (0.8, 24), "BipedThrow": (0.9, 36),
    "BipedCombatIdle": (2.0, 60), "BipedStrafeLeft": (1.0, 30), "BipedStrafeRight": (1.0, 30), "BipedWalkBackward": (1.0, 30), "BipedTurnLeft": (0.8, 32), "BipedTurnRight": (0.8, 32), "BipedJumpStart": (0.3, 12), "BipedFall": (1.0, 30), "BipedLand": (0.4, 16), "BipedSlash": (0.75, 30), "BipedBlock": (1.5, 45), "BipedDodge": (0.65, 30),
    "QuadrupedWalk": (1.0, 30), "QuadrupedRun": (1.0, 30), "QuadrupedIdle": (4.0, 90),
    "QuadrupedEat": (2.0, 40), "QuadrupedAttack": (1.2, 40), "QuadrupedHurt": (1.0, 36),
    "QuadrupedRoar": (3.0, 120), "QuadrupedDie": (1.4, 42),
    "BirdWalk": (1.0, 30), "BirdRun": (1.0, 30), "BirdFly": (1.0, 30), "BirdGlide": (3.0, 60),
    "BirdIdle": (4.0, 90), "BirdEat": (3.0, 60), "BirdAttack": (2.5, 60), "BirdHurt": (0.8, 24), "BirdStrike": (0.9, 27),
    "BirdDie": (1.4, 42),
    "FishSwim": (1.0, 30), "FishIdle": (4.0, 90), "FishDie": (1.8, 54),
    "FishAttack": (0.9, 27), "FishHurt": (0.8, 24),
    "InsectWalk": (1.0, 30), "InsectFly": (1.0, 30), "InsectIdle": (4.0, 90),
    "InsectAttack": (1.0, 30), "InsectRubHands": (1.0, 30), "InsectDie": (1.2, 36),
    "InsectHurt": (0.7, 21), "InsectBite": (0.9, 27),
    "SnakeSlither": (1.0, 30), "SnakeIdle": (4.0, 90), "SnakeStrike": (0.9, 30), "SnakeDie": (1.4, 42),
    "SnakeHurt": (0.8, 24),
    "SpiderWalk": (1.0, 30), "SpiderRun": (1.0, 30), "SpiderIdle": (4.0, 90), "SpiderAttack": (1.0, 36),
    "SpiderDie": (1.2, 36), "SpiderHurt": (0.8, 24),
}

ANIMATION_TIMING.update({
    "BipedSprint": (0.65, 32),
    "BipedSneak": (1.4, 44),
    "BipedCrouchEnter": (0.45, 24),
    "BipedCrouchIdle": (2, 60),
    "BipedCrouchExit": (0.45, 24),
    "BipedMountedIdle": (2, 60),
    "BipedMountedRide": (1, 40),
    "BipedStunned": (1.8, 60),
    "BipedKnockdown": (0.9, 48),
    "BipedGetUp": (2.4, 80),
    "BipedDodgeRoll": (0.8, 48),
    "BipedSwimIdle": (2, 48),
    "BipedSwimForward": (1.4, 48),
    "BipedSwimBackward": (1.4, 48),
    "BipedSwimLeft": (1.4, 48),
    "BipedSwimRight": (1.4, 48),
    "BipedOneHandSlash": (0.8, 40),
    "BipedTwoHandSwing": (1.1, 48),
    "BipedBowDraw": (0.8, 40),
    "BipedBowAim": (2, 60),
    "BipedBowShot": (0.7, 36),
    "BipedParry": (0.6, 32),
    "BipedCastStart": (0.25, 20),
    "BipedCastRelease": (0.3, 24),
    "BipedCastRecover": (0.45, 28),
    "BipedChannelEnter": (0.5, 28),
    "BipedChannelExit": (0.5, 28),
    "BipedChannelInterrupt": (0.3, 24),
    "BipedGather": (1.8, 60),
    "BipedPickUp": (1.2, 48),
    "BipedInteract": (1, 36),
    "BipedMine": (1.2, 48),
    "BipedChop": (1.2, 48),
    "BipedDrink": (2.2, 64),
    "BipedEat": (2.4, 72),
    "BipedSitDown": (1, 48),
    "BipedSitIdle": (3, 72),
    "BipedStandUp": (1, 48),
    "BipedSleepLieDown": (3, 90),
    "BipedSleepIdle": (4, 90),
    "BipedWakeUp": (3, 90),
    "BipedWave": (2, 64),
    "BipedCheer": (2, 64),
    "BipedBow": (1.8, 60),
    "BipedPoint": (1.5, 48),
    "BipedClap": (2, 64),
    "BipedDance": (2.4, 80),
    "BipedTalk": (3, 90),
})

# Clips that are cycles (a game engine should play them looped). The others play once.
LOOPING_ANIMATIONS = {"BipedCombatIdle", "BipedStrafeLeft", "BipedStrafeRight", "BipedWalkBackward", "BipedFall", "BipedBlock", "BipedWalk", "BipedRun", "BipedIdle", "BipedChannel", "BipedHop",
                      "QuadrupedWalk", "QuadrupedRun", "QuadrupedIdle", "QuadrupedEat",
                      "BirdWalk", "BirdRun", "BirdFly", "BirdGlide", "BirdIdle", "BirdEat",
                      "FishSwim", "FishIdle", "InsectWalk", "InsectFly", "InsectIdle",
                      "InsectRubHands", "SnakeSlither", "SnakeIdle",
                      "SpiderWalk", "SpiderRun", "SpiderIdle"}
LOOPING_ANIMATIONS.update({
    "BipedBowAim", "BipedChop", "BipedCrouchIdle", "BipedDance",
    "BipedMine", "BipedMountedIdle", "BipedMountedRide", "BipedSitIdle",
    "BipedSleepIdle", "BipedSneak", "BipedSprint", "BipedStunned",
    "BipedSwimBackward", "BipedSwimForward", "BipedSwimIdle", "BipedSwimLeft",
    "BipedSwimRight", "BipedTalk",
})


def animation_timing(anim) -> tuple:
    """(durationSeconds, frameCount) a clip is generated with."""
    if anim.type == "Pose":
        d = float(anim.pose.get("durationSeconds", 1.0))
        return (d, max(2, int(round(d * 30))))
    d, n = ANIMATION_TIMING.get(anim.type, (3.0, 90))
    return (float(anim.params.get("durationSeconds", d)), int(float(anim.params.get("frameCount", n))))


CUT_FACES = ["Quad", "Pentagon", "Hexagon", "Triangle"]
COMBINE_MODES = ["Normal", "Inversion", "Uncombined"]

_rig_cache: Dict[str, Dict[str, Any]] = {}


def _load_rigs_from_dust3d() -> None:
    """Rig templates come from the Dust3D binary itself (`dust3d -list-rigs` prints the
    templates compiled into it), so the toolkit never carries its own copies."""
    import platform
    import subprocess
    from .export import find_dust3d
    exe = find_dust3d()
    env = os.environ.copy()
    if platform.system() == "Linux" and not env.get("DISPLAY"):
        env.setdefault("QT_QPA_PLATFORM", "offscreen")
    try:
        res = subprocess.run([exe, "-list-rigs"], capture_output=True, timeout=30, env=env)
        out = res.stdout.decode("utf-8", "replace")
        root = ET.fromstring(out[out.index("<?xml"):] if "<?xml" in out else out)
    except OSError as e:
        raise RuntimeError("cannot run Dust3D binary %s (%s); set DUST3D_BIN" % (exe, e.strerror or e)) from None
    except (subprocess.TimeoutExpired, ET.ParseError, ValueError):
        raise RuntimeError("%s does not support -list-rigs (it needs a Dust3D build that includes the agent "
                           "modeling changes)" % exe) from None
    for r in root.iter("rig"):
        bones: Dict[str, Any] = {}
        order: List[str] = []
        for b in r.iter("bone"):
            pos = b.find("position")
            end = b.find("endPosition")
            bones[b.get("name")] = {
                "parent": b.get("parent") or "",
                "pos": tuple(float(pos.get(k)) for k in "xyz") if pos is not None else (0, 0, 0),
                "end": tuple(float(end.get(k)) for k in "xyz") if end is not None else (0, 0, 0),
            }
            order.append(b.get("name"))
        _rig_cache[r.get("type")] = {"bones": bones, "order": order,
                                     "description": (r.findtext("description") or "").strip()}


def rig_types() -> List[str]:
    """Rig types compiled into the Dust3D binary."""
    if not _rig_cache:
        _load_rigs_from_dust3d()
    return list(_rig_cache)


def load_rig_template(rig_type: str) -> Dict[str, Any]:
    """{bones: {name: {parent, pos, end}}, order: [...], description} for one rig type."""
    if not _rig_cache:
        _load_rigs_from_dust3d()
    if rig_type not in _rig_cache:
        raise SpecError("rig %r is not provided by this Dust3D build (available: %s)"
                        % (rig_type, ", ".join(sorted(_rig_cache))))
    return _rig_cache[rig_type]


_COLOR_RE = re.compile(r"^#([0-9a-fA-F]{6}|[0-9a-fA-F]{8})$")


@dataclass
class Part:
    """A node chain. kind: Model (swept tube), ImportedModel (a GLB swept along the chain),
    StitchingLine / StitchingLoop (members of a stitch group)."""
    name: str
    nodes: List[List[float]]
    bones: List[str] = field(default_factory=list)  # per edge, "" = none
    mirror: bool = False
    color: str = "#ffe0e0e0"
    cutFace: Any = "Quad"  # preset name, [[u, v], ...] polygon, or {"stroke": [[u, v, r], ...]}
    rounded: bool = True
    subdivided: bool = True
    chamfered: bool = False
    deformThickness: float = 1.0
    deformWidth: float = 1.0
    cutRotation: float = 0.0
    metallic: float = 0.0
    roughness: float = 1.0
    emissive: float = 0.0  # glow: the part's colour x this is written to the emissive map (0..4)
    slot: str = ""  # equipment slot/variant ("armor/2"): exported as its own mesh the game shows or hides
    combine: str = "Normal"
    loop: bool = False
    smooth: float = 60.0
    flatten: Dict[str, float] = field(default_factory=dict)  # e.g. {"x": 0.2}: squash along world X
    deformUnified: bool = False  # deform relative to the part's largest radius (keeps flat parts evenly thin)
    interpolate: bool = True  # False: no extra rings along long edges (rigid thin parts: far fewer triangles)
    hard: bool = False  # hard surface: joins other parts with a crisp boolean edge (no smooth bridge)
    node_deform: List[List[float]] = field(default_factory=list)  # per node [width, thickness] scale
    disabled: bool = False
    image: str = ""       # colour texture (PNG path), applied through the part's component
    kind: str = "Model"
    import_path: str = ""  # kind == ImportedModel: GLB file swept along the chain
    fillInterior: bool = False  # StitchingLoop only: cap the loop when it is isolated


@dataclass
class Group:
    """A component with children. stitch: "" (plain group), "lines" or "loops"."""
    name: str
    children: List[Any]
    stitch: str = ""
    combine: str = "Normal"
    color: str = ""
    smooth: float = 60.0
    image: str = ""
    slot: str = ""
    frontClosed: bool = False
    backClosed: bool = False
    sideClosed: bool = False
    targetSegments: int = 0
    backCloseDepthRatio: float = 1.0
    backCloseSharpness: float = 0.0
    mirror: bool = False  # stitch "lines" only: emit an X-mirrored copy of the whole group
    wrap: Dict[str, Any] = field(default_factory=dict)  # wrap modifier (see WRAP_KEYS); {} = none


@dataclass
class Animation:
    type: str
    name: str
    params: Dict[str, Any] = field(default_factory=dict)
    events: Dict[str, Any] = field(default_factory=dict)  # {"hit": 0.45} (fraction of the clip), or a list
    pose: Dict[str, Any] = field(default_factory=dict)    # type "Pose": {"keys": [...], "base": clip, "loop": bool}


@dataclass
class ModelSpec:
    name: str
    elements: List[Any]
    rig: str = ""
    animations: List[Animation] = field(default_factory=list)
    headHasEyelids: bool = False
    autoOrder: bool = True  # move Uncombined parts into a trailing group (better union seams)
    smoothWeights: int = 0  # skin-weight smoothing passes at joints (0 = Dust3D's weights as they are)
    budget: int = 0  # triangle budget for the heaviest outfit (0 = no limit)

    @property
    def parts(self) -> List[Part]:
        """Every node-bearing part, depth first (tubes, imported, stitch lines/loops)."""
        out: List[Part] = []

        def walk(items):
            for e in items:
                if isinstance(e, Group):
                    walk(e.children)
                else:
                    out.append(e)
        walk(self.elements)
        return out

    def groups(self) -> List[Group]:
        out: List[Group] = []

        def walk(items):
            for e in items:
                if isinstance(e, Group):
                    out.append(e)
                    walk(e.children)
        walk(self.elements)
        return out


class SpecError(Exception):
    pass


PART_KEYS = {f for f in Part.__dataclass_fields__} - {"kind", "import_path", "fillInterior", "node_deform"} | {"import"}
# "skin" is the old name of "wrap", still read so existing specs keep working.
GROUP_KEYS = {"name", "group", "combine", "color", "smooth", "image", "slot", "wrap", "skin"}
# The wrap modifier of a group: one surface wrapped around everything the group's children
# generate. "creature" replaces the children (a tight, seamless skin over bones and muscle
# shapes); "cloth" keeps them and adds a loose garment over them.
WRAP_MODES = {"creature": "Skin", "cloth": "Cloth"}
WRAP_KEYS = {"mode", "offset", "smoothness", "drape", "drapeLength", "openTop", "openBottom", "thickness",
             "faces", "keep", "weightRadius", "bindTo", "pattern", "patternColor", "patternScale", "belly",
             "wrinkles", "wrinkleSize"}
WRAP_ATTRS = {"offset": "wrapOffset", "smoothness": "wrapSmoothness", "drape": "wrapDrape",
              "drapeLength": "wrapDrapeLength", "openTop": "wrapOpenTop", "openBottom": "wrapOpenBottom",
              "thickness": "wrapThickness", "faces": "wrapFaces", "weightRadius": "wrapWeightRadius",
              "patternScale": "wrapPatternScale", "belly": "wrapBelly",
              "wrinkles": "wrapWrinkles", "wrinkleSize": "wrapWrinkleSize"}
# An animal coat painted into the texture (Dust3D's SurfacePattern): spec name -> document value.
WRAP_PATTERNS = {"spots": "Spots", "rosettes": "Rosettes", "stripes": "Stripes", "patches": "Patches",
                 "mottled": "Mottled"}


def _check_wrap(gname, wrap):
    if wrap in (None, {}, False):
        return {}
    if isinstance(wrap, str):
        wrap = {"mode": wrap}
    if not isinstance(wrap, dict):
        raise SpecError("group %r: wrap must be \"creature\", \"cloth\" or an object" % gname)
    unknown = set(wrap) - WRAP_KEYS
    if unknown:
        raise SpecError("group %r: unknown wrap keys %s (known: %s)" % (gname, sorted(unknown), sorted(WRAP_KEYS)))
    mode = wrap.get("mode", "creature")
    if mode not in WRAP_MODES:
        raise SpecError("group %r: wrap mode must be one of %s, got %r" % (gname, sorted(WRAP_MODES), mode))
    out = {"mode": mode}
    if "bindTo" in wrap:
        if not isinstance(wrap["bindTo"], str) or not wrap["bindTo"]:
            raise SpecError("group %r: wrap bindTo must be the name of a group" % gname)
        out["bindTo"] = wrap["bindTo"]
    if "keep" in wrap:
        if not isinstance(wrap["keep"], bool):
            raise SpecError("group %r: wrap keep must be true or false" % gname)
        out["keep"] = wrap["keep"]
    if "pattern" in wrap:
        if wrap["pattern"] not in WRAP_PATTERNS:
            raise SpecError("group %r: wrap pattern must be one of %s, got %r" % (gname, sorted(WRAP_PATTERNS), wrap["pattern"]))
        out["pattern"] = wrap["pattern"]
    if "patternColor" in wrap:
        try:
            out["patternColor"] = _norm_color(wrap["patternColor"])
        except SpecError:
            raise SpecError("group %r: wrap patternColor must be a colour (#RRGGBB), got %r" % (gname, wrap["patternColor"]))
    for k in WRAP_KEYS - {"mode", "keep", "bindTo", "pattern", "patternColor"}:
        if k in wrap:
            v = wrap[k]
            if not isinstance(v, (int, float)) or isinstance(v, bool):
                raise SpecError("group %r: wrap %s must be a number, got %r" % (gname, k, v))
            if k in ("drape",) and not 0.0 <= v <= 1.0:
                raise SpecError("group %r: wrap drape must be within [0, 1], got %r" % (gname, v))
            if k in ("openTop", "openBottom") and not 0.0 <= v <= 0.45:
                raise SpecError("group %r: wrap %s must be within [0, 0.45] (fraction of the height), got %r" % (gname, k, v))
            if k == "faces" and not 64 <= v <= 40000:
                raise SpecError("group %r: wrap faces must be within [64, 40000], got %r" % (gname, v))
            if k == "wrinkles" and not 0.0 <= v <= 1.0:
                raise SpecError("group %r: wrap wrinkles must be within [0, 1], got %r" % (gname, v))
            if k == "wrinkleSize" and not 0.2 <= v <= 4.0:
                raise SpecError("group %r: wrap wrinkleSize must be within [0.2, 4] (1: a natural fold width for the figure), got %r" % (gname, v))
            if k == "belly" and not 0.0 <= v <= 1.0:
                raise SpecError("group %r: wrap belly must be within [0, 1], got %r" % (gname, v))
            if k == "patternScale" and not 0.005 <= v <= 1.0:
                raise SpecError("group %r: wrap patternScale must be within [0.005, 1] (world units), got %r" % (gname, v))
            if k in ("smoothness", "thickness", "drapeLength", "weightRadius") and v < 0:
                raise SpecError("group %r: wrap %s must not be negative, got %r" % (gname, k, v))
            out[k] = v
    return out
# Material keys a stitched surface passes on to every line or loop it is made of.
STITCH_MATERIAL_KEYS = {"metallic", "roughness", "emissive"}
LINES_KEYS = {"name", "stitch", "lines", "combine", "color", "smooth", "image", "frontClosed",
              "backClosed", "sideClosed", "targetSegments", "mirror"} | STITCH_MATERIAL_KEYS
LOOPS_KEYS = {"name", "stitch", "loops", "combine", "color", "smooth", "image", "backClosed",
              "backCloseDepthRatio", "backCloseSharpness", "targetSegments"} | STITCH_MATERIAL_KEYS
MEMBER_KEYS = {"name", "nodes", "bones", "color", "closed", "fillInterior", "disabled"}


def _norm_color(c: str) -> str:
    if not isinstance(c, str) or not _COLOR_RE.match(c):
        raise SpecError("bad color %r (use #RRGGBB)" % (c,))
    c = c.lower()
    return c if len(c) == 9 else "#ff" + c[1:]


def _parse_cut_face(pname, cut):
    """Preset name, polygon [[u, v], ...] (>= 3 points) or {"stroke": [[u, v, r], ...]} (>= 2)."""
    if isinstance(cut, str):
        if cut not in CUT_FACES:
            raise SpecError("part %r: cutFace must be one of %s, a polygon or a stroke" % (pname, CUT_FACES))
        return cut
    if isinstance(cut, list):
        pts = [[float(v) for v in p] for p in cut]
        if len(pts) < 3 or any(len(p) != 2 for p in pts):
            raise SpecError("part %r: polygon cutFace needs >= 3 [u, v] points" % pname)
        return pts
    if isinstance(cut, dict) and "stroke" in cut:
        pts = [[float(v) for v in p] for p in cut["stroke"]]
        if len(pts) < 2 or any(len(p) != 3 or p[2] <= 0 for p in pts):
            raise SpecError("part %r: stroke cutFace needs >= 2 [u, v, radius>0] points" % pname)
        return {"stroke": pts}
    raise SpecError("part %r: bad cutFace %r" % (pname, cut))


def _parse_flatten(pname, raw) -> Dict[str, float]:
    if not raw:
        return {}
    if not isinstance(raw, dict) or not set(raw) <= {"x", "y", "z"}:
        raise SpecError('part %r: flatten must look like {"x": 0.2} (axes x/y/z)' % pname)
    return {k: float(v) for k, v in raw.items()}


def _norm(v):
    n = math.sqrt(sum(c * c for c in v))
    return [c / n for c in v] if n > 1e-12 else [0.0, 0.0, 0.0]


def _cross(a, b):
    return [a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]]


def _dot(a, b):
    return sum(x * y for x, y in zip(a, b))


def tube_axes(nodes: List[List[float]]):
    """Replicates dust3d::BaseNormal::calculateTubeBaseNormal + TubeMeshBuilder's u/v.

    Returns (u, v): u is scaled by deformWidth, v by deformThickness (cutRotation = 0).
    """
    pts = [n[:3] for n in nodes]
    if len(pts) < 2:
        return None
    dirs = [_norm([b[i] - a[i] for i in range(3)]) for a, b in zip(pts, pts[1:])]
    base = [0.0, 0.0, 0.0]
    for a, b in zip(dirs, dirs[1:]):
        if abs(_dot(a, b)) < 0.966:
            c = _cross([-x for x in a], b)
            base = [base[i] + c[i] for i in range(3)]
    if all(abs(c) < 1e-12 for c in base):
        for d in dirs[:-1] if len(dirs) > 1 else dirs:
            dots = [abs(x) for x in d]
            idx = max(range(3), key=lambda i: (dots[i], i))
            sign = -1 if d[idx] < 0 else 1
            nxt = [0.0, 0.0, 0.0]
            nxt[(idx + 1) % 3] = 1.0
            c = _norm(_cross(d, nxt))
            base = [base[i] + sign * c[i] for i in range(3)]
    u = _norm(base)
    fwd = _norm([pts[-1][i] - pts[0][i] for i in range(3)])
    v = _norm(_cross(fwd, u))
    u = _norm(_cross(v, fwd))
    return u, v


def resolve_flatten(p: "Part"):
    """Map world-axis flatten factors onto deformWidth / deformThickness."""
    w, t = p.deformWidth, p.deformThickness
    notes = []
    if not p.flatten:
        return w, t, notes
    axes = tube_axes(p.nodes)
    if axes is None:
        return w, t, ["part %r: flatten ignored on a single-node part" % p.name]
    u, v = axes
    unit = {"x": [1, 0, 0], "y": [0, 1, 0], "z": [0, 0, 1]}
    for ax, amount in p.flatten.items():
        cu, cv = abs(_dot(u, unit[ax])), abs(_dot(v, unit[ax]))
        if max(cu, cv) < 0.5:
            notes.append("part %r: flatten %s is along the chain direction and cannot be applied" % (p.name, ax))
            continue
        if cu >= cv:
            w = amount
        else:
            t = amount
    return w, t, notes


def _clean_nodes(pname, nodes, min_count=1):
    if not nodes or len(nodes) < min_count:
        raise SpecError("part %r needs at least %d node(s)" % (pname, min_count))
    clean = []
    for n in nodes:
        if not isinstance(n, (list, tuple)) or len(n) not in (4, 6):
            raise SpecError("part %r: node must be [x, y, z, radius] or [x, y, z, radius, width, thickness], got %r"
                            % (pname, n))
        if len(n) == 6 and (float(n[4]) <= 0 or float(n[5]) <= 0):
            raise SpecError("part %r: node width/thickness scale must be > 0, got %r" % (pname, n))
        x, y, z, r = (float(v) for v in n[:4])
        if not all(math.isfinite(v) for v in (x, y, z, r)):
            raise SpecError("part %r: non-finite node %r" % (pname, n))
        if r <= 0:
            raise SpecError("part %r: radius must be > 0, got %r" % (pname, r))
        clean.append([x, y, z, r])
    return clean


def _node_deform(nodes):
    """Per-node [width, thickness] scales from 6-number nodes; [] when every node has 4."""
    if not any(isinstance(n, (list, tuple)) and len(n) == 6 for n in nodes or []):
        return []
    return [[float(n[4]), float(n[5])] if len(n) == 6 else [1.0, 1.0] for n in nodes]


def _clean_bones(pname, bones, edge_count):
    bones = bones or []
    if isinstance(bones, str):
        bones = [bones] * edge_count
    bones = [b or "" for b in bones]
    if bones and len(bones) != edge_count:
        raise SpecError("part %r: %d bones given for %d edges" % (pname, len(bones), edge_count))
    return bones


_SLOT_RE = re.compile(r"^[a-z][a-z0-9]*/[A-Za-z0-9_-]+$")


def _check_slot(pname, slot):
    slot = str(slot or "")
    if slot and not _SLOT_RE.match(slot):
        raise SpecError("%r: slot must look like \"armor/2\" (slot name in lower case letters/digits, then a "
                        "variant), got %r" % (pname, slot))
    return slot


def _inherit_slot(children, slot):
    for c in children:
        if isinstance(c, Group):
            if not c.slot:
                c.slot = slot
                _inherit_slot(c.children, slot)
        elif not c.slot:
            c.slot = slot


def _check_combine(pname, comb):
    if comb not in COMBINE_MODES:
        raise SpecError("%r: combine must be one of %s" % (pname, COMBINE_MODES))
    return comb


def _resolve_path(base_dir, p):
    if not p:
        return ""
    p = p if os.path.isabs(p) else os.path.join(base_dir or ".", p)
    if not os.path.isfile(p):
        raise SpecError("file not found: %s" % p)
    return os.path.abspath(p)


def parse_spec(data: Dict[str, Any], base_dir: str = "") -> ModelSpec:
    if not isinstance(data, dict):
        raise SpecError("spec must be a JSON object")
    name = str(data.get("name") or "model")
    rig = data.get("rig") or ""
    if rig == "None":
        rig = ""
    defaults = dict(data.get("defaults") or {})
    seen = set()
    counter = [0]

    def unique(nm, prefix):
        counter[0] += 1
        nm = str(nm or "%s%d" % (prefix, counter[0]))
        if nm in seen:
            raise SpecError("duplicate name %r" % nm)
        seen.add(nm)
        return nm

    def parse_tube(raw):
        merged = dict(defaults)
        merged.update(raw)
        pname = unique(merged.get("name"), "part")
        unknown = set(merged) - PART_KEYS
        if unknown:
            raise SpecError("part %r: unknown keys %s" % (pname, sorted(unknown)))
        nodes = _clean_nodes(pname, merged.get("nodes"))
        loop = bool(merged.get("loop", False))
        edge_count = len(nodes) - 1 + (1 if loop and len(nodes) > 2 else 0)
        imp = _resolve_path(base_dir, merged.get("import"))
        if imp and len(nodes) < 2:
            raise SpecError("part %r: an imported mesh needs a spine of >= 2 nodes" % pname)
        return Part(
            name=pname, nodes=nodes, bones=_clean_bones(pname, merged.get("bones"), edge_count),
            mirror=bool(merged.get("mirror", False)),
            color=_norm_color(merged.get("color", "#e0e0e0")),
            cutFace=_parse_cut_face(pname, merged.get("cutFace", "Quad")),
            rounded=bool(merged.get("rounded", True)),
            subdivided=bool(merged.get("subdivided", True)),
            chamfered=bool(merged.get("chamfered", False)),
            deformThickness=float(merged.get("deformThickness", 1.0)),
            deformWidth=float(merged.get("deformWidth", 1.0)),
            cutRotation=float(merged.get("cutRotation", 0.0)),
            metallic=float(merged.get("metallic", 0.0)),
            roughness=float(merged.get("roughness", 1.0)),
            emissive=float(merged.get("emissive", 0.0)),
            slot=_check_slot(pname, merged.get("slot", "")),
            combine=_check_combine(pname, merged.get("combine", "Normal")), loop=loop,
            smooth=float(merged.get("smooth", 60.0)),
            flatten=_parse_flatten(pname, merged.get("flatten")),
            deformUnified=bool(merged.get("deformUnified", False)),
            interpolate=bool(merged.get("interpolate", True)),
            hard=bool(merged.get("hard", False)),
            node_deform=_node_deform(merged.get("nodes")),
            disabled=bool(merged.get("disabled", False)),
            image=_resolve_path(base_dir, merged.get("image")),
            kind="ImportedModel" if imp else "Model", import_path=imp)

    def parse_member(raw, gname, i, kind, material=None):
        if isinstance(raw, list):
            raw = {"nodes": raw}
        material = material or {}
        unknown = set(raw) - MEMBER_KEYS
        if unknown:
            raise SpecError("%r member %d: unknown keys %s" % (gname, i, sorted(unknown)))
        pname = unique(raw.get("name") or "%s_%d" % (gname, i), "member")
        closed = bool(raw.get("closed", False)) and kind == "StitchingLoop"
        nodes = _clean_nodes(pname, raw.get("nodes"), 2)
        edge_count = len(nodes) - 1 + (1 if closed and len(nodes) > 2 else 0)
        return Part(name=pname, nodes=nodes, kind=kind,
                    bones=_clean_bones(pname, raw.get("bones"), edge_count),
                    color=_norm_color(raw["color"]) if raw.get("color") else "",
                    loop=closed, fillInterior=bool(raw.get("fillInterior", False)),
                    disabled=bool(raw.get("disabled", False)), rounded=False, subdivided=False,
                    metallic=float(material.get("metallic", 0.0)),
                    roughness=float(material.get("roughness", 1.0)),
                    emissive=float(material.get("emissive", 0.0)))

    def parse_element(raw):
        if not isinstance(raw, dict):
            raise SpecError("each entry of parts must be an object, got %r" % (raw,))
        if "group" in raw:
            unknown = set(raw) - GROUP_KEYS
            if unknown:
                raise SpecError("group %r: unknown keys %s" % (raw.get("name"), sorted(unknown)))
            gname = unique(raw.get("name"), "group")
            children = [parse_element(c) for c in raw["group"]]
            if not children:
                raise SpecError("group %r is empty" % gname)
            slot = _check_slot(gname, raw.get("slot", ""))
            if "wrap" in raw and "skin" in raw:
                raise SpecError("group %r: has both \"wrap\" and \"skin\" (its old name); keep only \"wrap\"" % gname)
            wrap = _check_wrap(gname, raw.get("wrap", raw.get("skin")))
            # a wrap group is one surface: the slot goes on the group itself (a cloth group
            # keeps its children, and they are the body, not the equipment)
            if slot and not wrap:
                _inherit_slot(children, slot)
            return Group(name=gname, children=children,
                         combine=_check_combine(gname, raw.get("combine", "Normal")),
                         color=_norm_color(raw["color"]) if raw.get("color") else "",
                         smooth=float(raw.get("smooth", defaults.get("smooth", 60.0 if not wrap else 0.0))),
                         image=_resolve_path(base_dir, raw.get("image")), slot=slot, wrap=wrap)
        if "stitch" in raw:
            kind = raw["stitch"]
            if kind not in ("lines", "loops"):
                raise SpecError('stitch must be "lines" or "loops", got %r' % (kind,))
            keys = LINES_KEYS if kind == "lines" else LOOPS_KEYS
            unknown = set(raw) - keys
            if unknown:
                raise SpecError("stitch %s %r: unknown keys %s" % (kind, raw.get("name"), sorted(unknown)))
            gname = unique(raw.get("name"), "stitch")
            members = raw.get("lines" if kind == "lines" else "loops") or []
            pk = "StitchingLine" if kind == "lines" else "StitchingLoop"
            material = {k: raw[k] for k in STITCH_MATERIAL_KEYS if k in raw}
            children = [parse_member(m, gname, i, pk, material) for i, m in enumerate(members)]
            if kind == "lines" and len(children) < 2:
                raise SpecError("stitch lines %r needs at least 2 lines" % gname)
            if kind == "loops" and not children:
                raise SpecError("stitch loops %r needs at least 1 loop" % gname)
            color = raw.get("color", defaults.get("color", "#e0e0e0"))
            return Group(name=gname, children=children, stitch=kind,
                         combine=_check_combine(gname, raw.get("combine", "Normal")),
                         color=_norm_color(color), smooth=float(raw.get("smooth", defaults.get("smooth", 60.0))),
                         image=_resolve_path(base_dir, raw.get("image")),
                         frontClosed=bool(raw.get("frontClosed", False)),
                         backClosed=bool(raw.get("backClosed", False)),
                         sideClosed=bool(raw.get("sideClosed", False)),
                         targetSegments=int(raw.get("targetSegments", 0)),
                         backCloseDepthRatio=float(raw.get("backCloseDepthRatio", 1.0)),
                         backCloseSharpness=float(raw.get("backCloseSharpness", 0.0)),
                         mirror=bool(raw.get("mirror", False)))
        return parse_tube(raw)

    from .shapes import expand_shapes, ShapeError
    from .garment import expand_shells
    try:
        raw_elements = expand_shells(expand_shapes(data.get("parts") or []), defaults)
    except ShapeError as e:
        raise SpecError(str(e))
    elements = [parse_element(e) for e in raw_elements]
    if not elements:
        raise SpecError("spec has no parts")
    anims: List[Animation] = []
    for a in data.get("animations") or []:
        if isinstance(a, str):
            a = {"type": a}
        t = a.get("type")
        nm = a.get("name") or re.sub(r"^(Biped|Quadruped|Bird|Fish|Insect|Snake|Spider)", "", t or "").lower()
        events = a.get("events") or {}
        if not isinstance(events, dict):
            raise SpecError("animation %r: events must look like {\"hit\": 0.45}" % nm)
        pose = {}
        if t == "Pose":
            unknown = set(a) - {"type", "name", "keys", "base", "loop", "durationSeconds", "events"}
            if unknown:
                raise SpecError("pose clip %r: unknown keys %s" % (nm, sorted(unknown)))
            keys = a.get("keys") or []
            if len(keys) < 2 or any(not isinstance(k, dict) or "t" not in k for k in keys):
                raise SpecError('pose clip %r needs >= 2 keys like {"t": 0.0, "pose": {"Spine": [30, 0, 0]}}' % nm)
            duration = float(a.get("durationSeconds", max(float(k["t"]) for k in keys)))
            if duration <= 0:
                raise SpecError("pose clip %r: durationSeconds must be > 0" % nm)
            pose = {"keys": keys, "base": a.get("base", ""), "loop": bool(a.get("loop", False)),
                    "durationSeconds": duration}
        anims.append(Animation(type=t, name=nm, params=dict(a.get("params") or {}), events=dict(events), pose=pose))
    return ModelSpec(name=name, elements=elements, rig=rig, animations=anims,
                     headHasEyelids=bool(data.get("headHasEyelids", False)),
                     autoOrder=bool(data.get("autoOrder", True)),
                     smoothWeights=int(data.get("smoothWeights", 0)),
                     budget=int(data.get("budget", 0)))


VARIANT_KEYS = ("extends", "recolor", "remove", "override", "add", "addTo", "scale")


def resolve_extends(data: Dict[str, Any], base_dir: str, _seen: Optional[set] = None) -> Dict[str, Any]:
    """Expand a variant spec into a full spec.

    A variant names its base and lists only what differs, so creature families share
    one rig and one design (a hell hound from the wolf, a boss from the skeleton):

      {"extends": "wolf.json", "name": "hell_hound",
       "recolor": {"#6f6a63": "#3a2320"},          # every colour equal to a key (case-insensitive)
       "remove": ["mane"],                          # parts or groups, by name
       "override": {"tail": {"color": "#1a1412"}},  # replace fields of a part or group, by name
       "add": [{"name": "horn", ...}],              # extra parts, appended at the top level
       "addTo": {"body": [{"name": "bust", ...}]},  # extra children appended to a named group
       "scale": 1.3}                                # uniform scale of every node and radius

    Other top-level keys (name, rig, defaults, animations, autoOrder...) replace the
    base's. The base may itself extend another spec. The path is relative to the variant.
    """
    if "extends" not in data:
        return data
    seen = set(_seen or ())
    base_path = os.path.normpath(os.path.join(base_dir, data["extends"]))
    if base_path in seen:
        raise SpecError("extends cycle at %s" % base_path)
    seen.add(base_path)
    try:
        with open(base_path, "r", encoding="utf-8") as f:
            base = json.load(f)
    except OSError as e:
        raise SpecError("extends: cannot read %s (%s)" % (data["extends"], e))
    out = resolve_extends(base, os.path.dirname(base_path), seen)
    out = json.loads(json.dumps(out))  # deep copy

    def anchor(obj):  # the base's image/mesh paths are relative to the base, not the variant
        if isinstance(obj, dict):
            for k, v in obj.items():
                if k in ("image", "import") and isinstance(v, str) and v and not os.path.isabs(v):
                    obj[k] = os.path.normpath(os.path.join(os.path.dirname(base_path), v))
                else:
                    anchor(v)
        elif isinstance(obj, list):
            for v in obj:
                anchor(v)
    anchor(out.get("parts"))

    def elements(items):
        for e in items:
            yield e
            for key in ("group", "lines", "loops"):
                if isinstance(e.get(key), list):
                    yield from elements(e[key])

    remove = set(data.get("remove") or [])
    if remove:
        known = {e.get("name") for e in elements(out.get("parts") or [])}
        missing = sorted(remove - known)
        if missing:
            raise SpecError("remove: no part or group named %s in %s" % (", ".join(missing), data["extends"]))

        def prune(items):
            kept = []
            for e in items:
                if e.get("name") in remove:
                    continue
                if isinstance(e.get("group"), list):
                    e["group"] = prune(e["group"])
                kept.append(e)
            return kept
        out["parts"] = prune(out.get("parts") or [])

    for name, fields in (data.get("override") or {}).items():
        hits = [e for e in elements(out.get("parts") or []) if e.get("name") == name]
        if not hits:
            raise SpecError("override: no part or group named %r in %s" % (name, data["extends"]))
        for e in hits:
            # old names ("skin" for a group's "wrap", "wrap" for a part's "shell"): an override
            # in one name replaces the other
            renamed = (("wrap", "skin"), ("skin", "wrap")) if "group" in e else (("shell", "wrap"), ("wrap", "shell"))
            for new, old in renamed:
                if new in fields:
                    e.pop(old, None)
            e.update(json.loads(json.dumps(fields)))

    out["parts"] = (out.get("parts") or []) + json.loads(json.dumps(data.get("add") or []))

    # Children for a group of the base: a bust into a creature-skin body, a guide into a
    # garment so it covers the bust too, a beard curl into a hair group. Top-level `add`
    # can't do this: a wrap modifier wraps only its own children.
    for name, children in (data.get("addTo") or {}).items():
        hits = [e for e in elements(out.get("parts") or []) if e.get("name") == name and isinstance(e.get("group"), list)]
        if not hits:
            raise SpecError("addTo: no group named %r in %s" % (name, data["extends"]))
        if not isinstance(children, list):
            raise SpecError("addTo: %r must map to a list of parts" % name)
        for e in hits:
            e["group"].extend(json.loads(json.dumps(children)))

    recolor = {k.lower(): v for k, v in (data.get("recolor") or {}).items()}
    if recolor:
        def paint(obj):
            if isinstance(obj, dict):
                for k, v in obj.items():
                    if k == "color" and isinstance(v, str) and v.lower() in recolor:
                        obj[k] = recolor[v.lower()]
                    else:
                        paint(v)
            elif isinstance(obj, list):
                for v in obj:
                    paint(v)
        paint(out.get("parts"))
        paint(out.get("defaults"))

    scale = float(data.get("scale", 1.0))
    if scale != 1.0:
        from .shapes import expand_shapes, ShapeError
        try:  # shapes are measured in metres: expand them so their nodes scale like any other
            out["parts"] = expand_shapes(out["parts"])
        except ShapeError as e:
            raise SpecError(str(e))

        def grow(items):
            for e in items:
                if isinstance(e.get("nodes"), list):
                    e["nodes"] = [[round(c * scale, 5) for c in n[:4]] + list(n[4:]) for n in e["nodes"]]
                for key in ("group", "lines", "loops"):
                    if isinstance(e.get(key), list):
                        grow(e[key])
        grow(out["parts"])

    for k, v in data.items():
        if k not in VARIANT_KEYS:
            out[k] = v
    return out


def load_spec_dict(path: str) -> Dict[str, Any]:
    """Read a spec file as a dict, with any `extends` expanded."""
    with open(path, "r", encoding="utf-8") as f:
        data = json.load(f)
    return resolve_extends(data, os.path.dirname(os.path.abspath(path)))


def load_spec(path: str) -> ModelSpec:
    return parse_spec(load_spec_dict(path), os.path.dirname(os.path.abspath(path)))


def lint_spec(spec: ModelSpec) -> List[str]:
    """Static checks an agent should fix before exporting. Returns human-readable warnings."""
    w: List[str] = []
    bone_names = set()
    rig = None
    if spec.rig:
        try:
            rig = load_rig_template(spec.rig)
        except SpecError as e:
            return [str(e)]
        bone_names = set(rig["bones"])
    used = {}
    mirrored_members = {m.name for g in spec.groups() if g.stitch == "lines" and g.mirror for m in g.children}
    for g in spec.groups():
        if g.stitch == "lines":
            xs = [n[0] for m in g.children for n in m.nodes]
            if g.mirror and min(xs) < -1e-3 < 1e-3 < max(xs):
                w.append("stitch lines %r is mirrored but crosses x=0" % g.name)
            counts = sorted({len(m.nodes) for m in g.children})
            if g.targetSegments and g.targetSegments < max(counts) - 1:
                w.append("advisory: stitch lines %r: targetSegments %d is below the node count of its lines; detail will be lost"
                         % (g.name, g.targetSegments))
        if g.stitch == "loops":
            for m in g.children:
                xs = [n[0] for n in m.nodes]
                if not m.loop and min(xs) < -1e-3 < 1e-3 < max(xs):
                    w.append("stitch loop %r is open but spans both sides; Dust3D mirrors open loops, "
                             "so draw only one half (or make it closed)" % m.name)
    # Dust3D appends mirrored copies after all other parts of their group, so a part that
    # attaches to a mirrored copy is unioned before that copy exists (floating seam, then a
    # double seam when the copy arrives).
    # Parts inside a wrap group are not unioned by booleans: one surface is wrapped
    # around them, so the seam and union-order checks do not apply to them.
    wrapped = set()

    def collect_wrapped(items, inside):
        for e in items:
            if isinstance(e, Group):
                collect_wrapped(e.children, inside or bool(e.wrap))
            elif inside:
                wrapped.add(e.name)

    collect_wrapped(spec.elements, False)

    def check_wraps(items):
        for e in items:
            if not isinstance(e, Group):
                continue
            if e.wrap.get("mode") == "cloth" and e.wrap.get("keep") is False and not e.wrap.get("bindTo"):
                w.append("advisory: garment group %r takes its skin weights from its own guide shapes; give it "
                         "\"bindTo\": the body group, so the body stays inside it in every pose" % e.name)
            if e.wrap and e.slot and e.combine != "Uncombined":
                w.append("wrap group %r has a slot but is combined with %s; make it \"combine\": \"Uncombined\" so "
                         "the garment stays its own mesh" % (e.name, "the model"))
            check_wraps(e.children)

    check_wraps(spec.elements)
    tubes = [p for p in spec.parts if p.kind in ("Model", "ImportedModel") and p.combine != "Uncombined"
             and p.name not in wrapped]
    mirrored = [p for p in tubes if p.mirror]
    for p in tubes:
        if p.mirror or all(n[0] > -1e-3 for n in p.nodes):
            continue
        for m in mirrored:
            touch = any(math.dist(a[:3], [-b[0], b[1], b[2]]) < a[3] + b[3] for a in p.nodes for b in m.nodes)
            if touch:
                w.append("part %r attaches to the mirrored copy of %r (x < 0). Dust3D unions mirrored copies "
                         "last, so %r is joined before its support exists; attach it on the +X side instead"
                         % (p.name, m.name, p.name))
                break
    # Dust3D combines a group's children in runs of the same combine mode. An Inversion in
    # the middle ends the solid run: the solid parts after it are unioned only with each
    # other (a separate run) and then merged with the carved result in one boolean, so
    # limbs after an eye-socket carve "don't touch" their body and join badly.
    def check_runs(items, owner):
        for i, e in enumerate(items):
            if e.combine == "Uncombined" and not spec.autoOrder:
                after = [x.name for x in items[i + 1:] if x.combine == "Normal"]
                if after:
                    w.append("part %r (Uncombined) in %s is followed by solid parts (%s); they form a separate run "
                             "that is unioned apart from the body. Move Uncombined parts to the end (autoOrder does "
                             "this)" % (e.name, owner, ", ".join(after[:4])))
                continue
            if e.combine != "Inversion":
                continue
            after = [x.name for x in items[i + 1:] if x.combine == "Normal"]
            if after:
                w.append("part %r (Inversion) in %s is followed by solid parts (%s); they form a separate run that "
                         "is unioned apart from the body. Put the carved part and its Inversion parts in their own "
                         "group, or move the Inversion parts to the end" % (e.name, owner, ", ".join(after[:4])))
        for e in items:
            if isinstance(e, Group) and not e.stitch and not e.wrap:
                check_runs(e.children, "group %r" % e.name)
    check_runs(spec.elements, "the model")
    # scale steps at joins: a limb far thinner than the part it lands on fans at the seam
    for i, p in enumerate(tubes):
        if i == 0 or len(p.nodes) < 2 or p.hard:
            continue  # a hard-surface join is a plain boolean: no bridge to fan
        first = p.nodes[0]
        best = None
        for q in tubes[:i]:
            for n in q.nodes:
                d = math.dist(first[:3], n[:3])
                if best is None or d < best[0]:
                    best = (d, q, n)
        if best and best[0] < best[2][3] + first[3] and first[3] * 3.0 < best[2][3]:
            w.append("advisory: part %r (radius %.3f) joins %r (radius %.3f) more than 3x thinner; give it a "
                     "flared base (thigh, shoulder, ear base) or the seam will fan" % (p.name, first[3], best[1].name, best[2][3]))
    for p in spec.parts:
        w.extend(resolve_flatten(p)[2])
        xs = [n[0] for n in p.nodes]
        if p.mirror:
            if min(xs) < 0 < max(xs):
                w.append("part %r is mirrored but crosses x=0; mirrored copies will overlap" % p.name)
            if all(abs(x) < 1e-4 for x in xs):
                w.append("part %r is mirrored but lies on x=0; mirror is redundant" % p.name)
        for a, b in zip(p.nodes, p.nodes[1:]):
            if math.dist(a[:3], b[:3]) < 1e-5:
                w.append("part %r has two coincident consecutive nodes" % p.name)
        for i, b in enumerate(p.bones):
            if not b:
                continue
            if not spec.rig:
                w.append("part %r assigns bone %r but spec has no rig" % (p.name, b))
                break
            if b not in bone_names:
                w.append("part %r: bone %r not in %s rig (valid: %s)" % (p.name, b, spec.rig, ", ".join(rig["order"])))
                continue
            # Side sanity: Left bones should be on +X, Right on -X
            a, c = p.nodes[i], p.nodes[(i + 1) % len(p.nodes)]
            mx = (a[0] + c[0]) / 2
            if "Left" in b and mx < -1e-3:
                w.append("part %r: bone %r is on -X but Left is +X in Dust3D's frame" % (p.name, b))
            if "Right" in b and mx > 1e-3:
                w.append("part %r: bone %r is on +X but Right is -X in Dust3D's frame" % (p.name, b))
            used.setdefault(b, []).append(p.name)
            if p.mirror or p.name in mirrored_members:
                sw = b.replace("Left", "\0").replace("Right", "Left").replace("\0", "Right")
                used.setdefault(sw, []).append(p.name + "(mirror)")
    if spec.rig:
        missing = [b for b in rig["order"] if b != "Root" and b not in used]
        if missing:
            w.append("info: %s bones with no geometry (fine if the creature lacks them): %s" % (spec.rig, ", ".join(missing)))
        valid_anims = ANIMATION_TYPES.get(spec.rig, [])
        clip_names = {a.name for a in spec.animations}
        for a in spec.animations:
            if a.type == "Pose":
                base = a.pose.get("base")
                if base and (base not in clip_names or base == a.name):
                    w.append("pose clip %r: base clip %r is not another clip of this model" % (a.name, base))
                for k in a.pose.get("keys", []):
                    for b in (k.get("pose") or {}):
                        if b not in bone_names:
                            w.append("pose clip %r: bone %r not in %s rig" % (a.name, b, spec.rig))
                continue
            if a.type not in valid_anims:
                w.append("animation %r is not valid for %s rig (valid: %s)" % (a.type, spec.rig, ", ".join(valid_anims + ["Pose"])))
        for a in spec.animations:
            for ev, at in a.events.items():
                for v in (at if isinstance(at, list) else [at]):
                    if not isinstance(v, (int, float)) or not 0.0 <= float(v) <= 1.0:
                        w.append("animation %r: event %r time must be a fraction of the clip (0..1), got %r" % (a.name, ev, v))
    elif spec.animations:
        w.append("animations given but no rig; they will be ignored")
    for p in spec.parts:
        if not p.interpolate and not p.hard and len({b for b in p.bones if b}) > 1:
            w.append("advisory: part %r has interpolate false but spans several bones; with no extra rings "
                     "it bends only at its own nodes" % p.name)
        if p.mirror and not p.disabled and p.combine == "Uncombined":
            touching = [n for n in p.nodes if len(n) >= 4 and abs(n[0]) <= n[3] * 1.02]
            if touching:
                w.append("advisory: mirrored uncombined part %r has %d node(s) within their radius of the x=0 plane; "
                         "the two halves overlap and can leave non-manifold edges. Move it off the midline "
                         "(|x| > radius) or make it a single unmirrored part at x=0" % (p.name, len(touching)))
    return w
