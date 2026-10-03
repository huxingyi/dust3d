# AutoRemesher (core) for the Dust3D skin modifier

The quad remeshing core of [AutoRemesher](https://github.com/huxingyi/autoremesher)
(MIT, see `LICENSE`), from commit `3cb2012c`: the curvature aligned cross field
(`FrameField`, `SingularitySimplifier`), the mixed-integer quad parameterization
(`Parameterizer`, `QuadParameterizer`, `MixedIntegerLeastSquares`,
`ConstrainedLeastSquares`) and the quad extraction (`QuadExtractor`), and the isotropic remesher that prepares the triangles for them
(`IsotropicRemesher`, `thirdparty/isotropicremesher`).

Dust3D's skin modifier (`dust3d/mesh/wrap_mesh_builder.cc`) gives it a closed surface
already extracted from a distance field at the right density, so AutoRemesher's own
voxel resampling, decimation and adaptive sizing stages are not needed and not included.

The sources are unchanged. Two things stand in for AutoRemesher's dependencies:

- `tbbshim/`: the few oneTBB calls the core uses (`blocked_range`, `parallel_for`,
  `parallel_sort`), on `std::thread`, so Dust3D does not need TBB. It must come before
  any system TBB on the include path.
- `../eigen/`: Eigen's headers (MPL2, see `../eigen/COPYING.*`).
