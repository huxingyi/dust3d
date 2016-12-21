#ifndef B_MESH_H
#define B_MESH_H
#include "vector3d.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  BMESH_NODE_TYPE_KEY = 0,
  BMESH_NODE_TYPE_ROOT = 1,
  BMESH_NODE_TYPE_INBETWEEN = 2,
} bmeshNodeType;

typedef struct bmesh bmesh;

typedef struct {
  int index;
  vec3 position;
  float radius;
  int type;
  int childrenIndices;
  int firstChildIndex;
  int roundColor;
} bmeshNode;

typedef struct {
  int index;
  int firstNodeIndex;
  int secondNodeIndex;
} bmeshEdge;

bmesh *bmeshCreate(void);
void bmeshDestroy(bmesh *bm);
int bmeshGetNodeNum(bmesh *bm);
int bmeshGetEdgeNum(bmesh *bm);
bmeshNode *bmeshGetNode(bmesh *bm, int index);
bmeshEdge *bmeshGetEdge(bmesh *bm, int index);
int bmeshAddNode(bmesh *bm, bmeshNode *node);
int bmeshAddEdge(bmesh *bm, bmeshEdge *edge);
int bmeshGenerateInbetweenNodes(bmesh *bm);
int bmeshGetNodeNextChild(bmesh *bm, bmeshNode *node, int *childIndex);
bmeshNode *bmeshGetRootNode(bmesh *bm);

#ifdef __cplusplus
}
#endif

#endif
