#ifndef DUST3D_CONVEXHULL_H
#define DUST3D_CONVEXHULL_H
#include "vector3d.h"
#include "array.h"
#include "rbtree.h"

typedef struct convexhull convexhull;

convexhull *convexhullCreate(void);
void convexhullDestroy(convexhull *hull);
int convexhullAddPoint(convexhull *hull, point3d *point, void *sourcePlane, void *tag);
void *convexhullGetPointSourcePlane(convexhull *hull, int index);
int convexhullUpdatePointSourcePlane(convexhull *hull, int index, void *sourcePlane);
int convexhullAddStartup(convexhull *hull, int p1, int p2, vector3d *baseNormal);
int convexhullGenerate(convexhull *hull);
int convexhullGetGeneratedFaceCount(convexhull *hull);
int convexhullGetNthGeneratedFace(convexhull *hull, int index, int *p1, int *p2, int *p3);
int convexhullGetVertexInfo(convexhull *hull, int p, point3d *point, void **tag);

#endif
