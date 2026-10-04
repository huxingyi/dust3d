# AutoRemesher (core) for the Dust3D wrap modifier

The quad remeshing core of [AutoRemesher](https://github.com/huxingyi/autoremesher)
(MIT, see `LICENSE`), from commit `3cb2012c`: the curvature aligned cross field
(`FrameField`, `SingularitySimplifier`), the mixed-integer quad parameterization
(`Parameterizer`, `QuadParameterizer`, `MixedIntegerLeastSquares`,
`ConstrainedLeastSquares`) and the quad extraction (`QuadExtractor`), and the isotropic remesher that prepares the triangles for them
(`IsotropicRemesher`, `thirdparty/isotropicremesher`).

Dust3D's wrap modifier (`dust3d/mesh/wrap_mesh_builder.cc`) gives it a closed surface
already extracted from a distance field at the right density, so AutoRemesher's own
voxel sizing and decimation stages are not needed and not included. Its resample step's
adaptive target length field (`AutoRemesher::resample` in `autoremesher.cpp`) is ported
into `wrap_mesh_builder.cc` and run with the Parameterizer at adaptivity 1.0 and
anisotropy 1.0, so curved regions get smaller quads than flat ones.

The sources are unchanged. Two things stand in for AutoRemesher's dependencies:

- `tbbshim/`: the few oneTBB calls the core uses (`blocked_range`, `parallel_for`,
  `parallel_sort`), on `std::thread`, so Dust3D does not need TBB. It must come before
  any system TBB on the include path.
- `../eigen/`: Eigen's headers (MPL2, see `../eigen/COPYING.*`).
