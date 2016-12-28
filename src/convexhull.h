#ifndef CONVEX_HULL_H
#define CONVEX_HULL_H
#include "vector3d.h"
#include "draw.h"

typedef struct convexHull convexHull;
convexHull *convexHullCreate(void);
int convexHullAddVertex(convexHull *hull, vec3 *vertex, int plane,
  int orderOnPlane);
void convexHullDestroy(convexHull *hull);
int convexHullGenerate(convexHull *hull);
int convexHullUnifyNormals(convexHull *hull, vec3 *origin);
int convexHullGetTriangleNum(convexHull *hull);
triangle *convexHullGetTriangle(convexHull *hull, int index);
int convexHullAddTodo(convexHull *hull, int vertex1, int vertex2, int vertex3);

#endif
