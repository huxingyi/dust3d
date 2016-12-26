#ifndef CONVEX_HULL_H
#define CONVEX_HULL_H
#include "vector3d.h"
#include "draw.h"

typedef struct convexHull convexHull;
convexHull *convexHullCreate(void);
int convexHullAddVertex(convexHull *hull, vec3 *vertex);
void convexHullDestroy(convexHull *hull);
int convexHullGenerate(convexHull *hull);
int convexHullGetTriangleNum(convexHull *hull);
triangle *convexHullGetTriangle(convexHull *hull, int index);

#endif
