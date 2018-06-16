#ifndef MESH_UTIL_H
#define MESH_UTIL_H
#include <vector>
#include <set>

int mergeMeshs(void *meshliteContext, const std::vector<int> &meshIds);
int unionMeshs(void *meshliteContext, const std::vector<int> &meshIds, const std::set<int> &inverseIds, int *errorCount=0);
int subdivMesh(void *meshliteContext, int meshId, int *errorCount=0);
int fixMeshHoles(void *meshliteContext, int meshId);

#endif
