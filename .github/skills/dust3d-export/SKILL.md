---
name: dust3d-export
description: Open a dust3d .ds3 file and export it to a specific format (.obj, .glb, or .fbx) using the dust3d command-line interface.
---

# dust3d Export Skill

Use the `dust3d` command-line tool to open a `.ds3` file and export it to a target format.

## Command Syntax

```
dust3d <input.ds3> -o <output.obj|output.glb|output.fbx>
```

- The input file must end in `.ds3`.
- The `-o` (or `-output`) flag specifies the output file. The export format is determined by the output file's extension.
- Supported export formats: `.obj`, `.glb`, `.fbx`.
- Only one input `.ds3` file is supported when using export (`-o`).
- Multiple `-o` flags can be provided to export to several formats at once.

## Examples

Export to OBJ:
```
dust3d model.ds3 -o model.obj
```

Export to GLB:
```
dust3d model.ds3 -o model.glb
```

Export to FBX:
```
dust3d model.ds3 -o model.fbx
```

Export to multiple formats at once:
```
dust3d model.ds3 -o model.obj -o model.glb -o model.fbx
```

## Exit Codes

- `0` — All exports succeeded.
- `1` — One or more exports failed.
