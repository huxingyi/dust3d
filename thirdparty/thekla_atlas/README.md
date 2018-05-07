thekla_atlas
============

This tool performs mesh segmentation, surface parameterization, and chart packing.

This is especially useful when generating light maps for meshes that do not have artist-supplied uv's.

### How to build (Windows)

Open `vc9/thekla.sln` using Visual Studio.

### How to build (OS X)

Use cmake.  For example:

```bash
$ brew install cmake  # Install cmake
$ cmake -H. -Bbuild   # Create makefiles
$ cmake --build build # Invoke the build
```
