#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include "dust3d.h"
#include "array.h"

static void calculateEdgeSliceFace(mesh *m, halfedge *h, float amount, point3d *facePoint, vector3d *faceNormal) {
    vector3d edge;
    vector3d normal;
    vector3d line1;
    vector3d line2;
    assert(h);
    assert(h->opposite);
    halfedgeFaceNormal(m, h->left, &normal);
    halfedgeVector3d(m, h, &edge);
    vector3dCrossProduct(&edge, &normal, &line1);
    vector3dMultiply(&line1, amount);
    halfedgeFaceNormal(m, h->opposite->left, &normal);
    halfedgeVector3d(m, h->opposite, &edge);
    vector3dCrossProduct(&edge, &normal, &line2);
    vector3dMultiply(&line2, amount);
    *faceNormal = line1;
    vector3dAdd(faceNormal, &line2);
    vector3dMultiply(faceNormal, -0.5);
    *facePoint = h->start->position;
    vector3dAdd(facePoint, &line1);
}

typedef struct sliceFace {
    point3d point;
    vector3d normal;
} sliceFace;

int theChamfer(dust3dState *state) {
    float amount = 0.1;
    array *sliceFaceArray = arrayCreate(sizeof(sliceFace));
    mesh *m;
    face *itFace;
    int i;
    const char *amountStr;
    amountStr = dust3dGetNamingArgument(state, "a");
    if (amountStr) {
        amount = toFloat(amountStr);
        if (amount <= 0) {
            fprintf(stderr, "Invalid amount:\"%s\"\n", amountStr);
            return -1;
        }
    }
    m = dust3dGetCurrentMesh(state);
    for (itFace = m->firstFace; itFace; itFace = itFace->next) {
        halfedge *itHandle = itFace->handle;
        halfedge *itStop = itHandle;
        do {
            if (halfedgeMakeSureOnlyOnce(itHandle)) {
                sliceFace *sf = (sliceFace *)arrayNewItem(sliceFaceArray);
                calculateEdgeSliceFace(m, itHandle, amount, &sf->point, &sf->normal);
            }
            itHandle = itHandle->next;
        } while (itHandle != itStop);
    }
    for (i = 0; i < arrayGetLength(sliceFaceArray); i++) {
        sliceFace *sf = (sliceFace *)arrayGetItem(sliceFaceArray, i);
        mesh *frontMesh = halfedgeSliceMeshByPlane(m, &sf->point, &sf->normal, 0);
        halfedgeDestroyMesh(frontMesh);
    }
    arrayDestroy(sliceFaceArray);
    return 0;
}
