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
    halfedgeFaceNormal(h->left, &normal);
    vector3dSegment(&h->next->start->position, &h->start->position, &edge);
    vector3dCrossProduct(&edge, &normal, &line1);
    vector3dMultiply(&line1, amount);
    halfedgeFaceNormal(h->opposite->left, &normal);
    vector3dSegment(&h->opposite->next->start->position, &h->opposite->start->position, &edge);
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
    float amount = dust3dGetNamingFloat(state, "a", 0.1);
    int f = dust3dGetNamingInt(state, "f", -1);
    int e = dust3dGetNamingInt(state, "e", -1);
    array *sliceFaceArray = arrayCreate(sizeof(sliceFace));
    mesh *m = dust3dGetCurrentMesh(state);
    face *itFace;
    int i;
    int faceIndex;
    int edgeIndex;
    for (itFace = m->firstFace, faceIndex = 0; itFace; itFace = itFace->next, faceIndex++) {
        halfedge *itHandle = itFace->handle;
        halfedge *itStop = itHandle;
        if (-1 != f && f != faceIndex) {
            continue;
        }
        edgeIndex = 0;
        do {
            if ((-1 == e && halfedgeMakeSureOnlyOnce(itHandle)) || e == edgeIndex) {
                sliceFace *sf = (sliceFace *)arrayNewItem(sliceFaceArray);
                calculateEdgeSliceFace(m, itHandle, amount, &sf->point, &sf->normal);
            }
            itHandle = itHandle->next;
            edgeIndex++;
        } while (itHandle != itStop);
    }
    for (i = 0; i < arrayGetLength(sliceFaceArray); i++) {
        sliceFace *sf = (sliceFace *)arrayGetItem(sliceFaceArray, i);
        mesh *frontMesh = halfedgeSliceMeshByPlane(m, &sf->point, &sf->normal, 0);
        halfedgeCombineDuplicateVertices(m);
        halfedgeFixPairing(m);
        halfedgeFixHoles(m);
        halfedgeDestroyMesh(frontMesh);
    }
    arrayDestroy(sliceFaceArray);
    return 0;
}
