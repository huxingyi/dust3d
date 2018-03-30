#ifndef UNION_MESH_H
#define UNION_MESH_H
#include <vector>

int mergeMeshs(void *meshliteContext, const std::vector<int> &meshIds);
int unionMeshs(void *meshliteContext, const std::vector<int> &meshIds);

#endif
