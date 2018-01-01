#ifndef DUST3D_SELECTOR_H
#define DUST3D_SELECTOR_H
#include "halfedge.h"

face *selectorGetNthFace(mesh *m, int n);
halfedge *selectorGetNthHalfedge(face *f, int n);
int selectorArrangeMesh(mesh *m);

#endif

