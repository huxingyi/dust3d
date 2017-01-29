#ifndef SKINNED_MESH_H
#define SKINNED_MESH_H
#include "3dstruct.h"
#include "matrix.h"

#define SKINNED_MESH_MAX_BONE 4

typedef struct skinnedMeshVertex {
  vec3 position;
  vec3 normal;
  int boneIndices[SKINNED_MESH_MAX_BONE];
  float boneWeights[SKINNED_MESH_MAX_BONE];
} skinnedMeshVertex;

typedef struct skinnedMeshBone {
  vec3 translation;
  vec3 rotation;
  matrix toRoot;
} skinnedMeshBone;

#endif
