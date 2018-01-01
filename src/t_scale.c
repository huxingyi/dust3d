#include <stdio.h>
#include <string.h>
#include "dust3d.h"
#include "strutil.h"
#include "selector.h"

int theScale(dust3dState *state) {
    int faceIndex = dust3dGetNamingInt(state, "f", -1);
    int edgeIndex = dust3dGetNamingInt(state, "e", -1);
    float amount = dust3dGetNamingFloat(state, "a", 1.0);
    float amountX = dust3dGetNamingFloat(state, "x", 1.0);
    float amountY = dust3dGetNamingFloat(state, "y", 1.0);
    float amountZ = dust3dGetNamingFloat(state, "z", 1.0);
    //matrix mat;
    mesh *m = dust3dGetCurrentMesh(state);
    halfedge *h = 0;
    face *f = 0;
    if (-1 != faceIndex) {
        f = selectorGetNthFace(m, faceIndex);
        if (!f) {
            fprintf(stderr, "face index is out of range\n");
            return -1;
        }
    }
    if (-1 != edgeIndex) {
        if (!f) {
            fprintf(stderr, "face index is needed for edge selector\n");
            return -1;
        }
        h = selectorGetNthHalfedge(f, edgeIndex);
        if (!h) {
            fprintf(stderr, "edge index is out of range\n");
            return -1;
        }
    }
    if (h) {
        vector3d vec;
        halfedgeVector3d(m, h, &vec);
        vector3dNormalize(&vec);
        vector3dMultiply(&vec, (1 - amount) / 4);
        vector3dAdd(&h->start->position, &vec);
        vector3dMultiply(&vec, -1);
        vector3dAdd(&h->next->start->position, &vec);
        //matrixScaling(&mat, float x, float y, float z);
        return 0;
    }
    if (f) {
        return 0;
    }
    return 0;
}
