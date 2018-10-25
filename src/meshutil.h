#ifndef DUST3D_MESH_UTIL_H
#define DUST3D_MESH_UTIL_H
#include <vector>
#include <set>
#include <QVector3D>

#define MAX_VERTICES_PER_FACE   100

int mergeMeshs(void *meshliteContext, const std::vector<int> &meshIds);
int unionMeshs(void *meshliteContext, const std::vector<int> &meshIds, const std::set<int> &inverseIds, int *errorCount=0);
int subdivMesh(void *meshliteContext, int meshId, int *errorCount=0);
int fixMeshHoles(void *meshliteContext, int meshId);

void initMeshUtils();
void *convertToCombinableMesh(void *meshliteContext, int meshId);
void *unionCombinableMeshs(void *first, void *second);
void *diffCombinableMeshs(void *first, void *second);
int convertFromCombinableMesh(void *meshliteContext, void *mesh);
void deleteCombinableMesh(void *mesh);
void *cloneCombinableMesh(void *mesh);
void *convertToCombinableConvexHullMesh(void *meshliteContext, int meshId);
void loadMeshVerticesPositions(void *meshliteContext, int meshId, std::vector<QVector3D> &positions);
void loadCombinableMeshVerticesPositions(void *mesh, std::vector<QVector3D> &positions);

#endif
