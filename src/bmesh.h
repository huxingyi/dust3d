#ifndef B_MESH_H
#define B_MESH_H
#include "vector3d.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  BMESH_NODE_FLAG_KEY = 0x00000001,
  BMESH_NODE_FLAG_INBETWEEN = 0x00000002,
  BMESH_NODE_FLAG_ROOT = 0x00000010,
} bmeshNodeFlag;

typedef struct bmesh bmesh;

typedef struct {
  int index;
  vec3 position;
  float radius;
  unsigned int flag;
} bmeshNode;

typedef struct {
  int index;
  int firstNode;
  int secondNode;
} bmeshEdge;

bmesh *bmeshCreate(void);
void bmeshDestroy(bmesh *bm);
int bmeshGetNodeNum(bmesh *bm);
int bmeshGetEdgeNum(bmesh *bm);
bmeshNode *bmeshGetNode(bmesh *bm, int index);
bmeshEdge *bmeshGetEdge(bmesh *bm, int index);
int bmeshAddNode(bmesh *bm, bmeshNode *node);
int bmeshAddEdge(bmesh *bm, bmeshEdge *edge);

#ifdef __cplusplus
}
#endif

#endif
