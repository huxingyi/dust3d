#include <stdio.h>
#include "dust3d.h"

int theSlice(dust3dState *state) {
    const char *faceNormalStr;
    const char *faceOriginStr;
    point3d faceOrigin;
    vector3d faceNormal;
    mesh *frontMesh;
    faceNormalStr = dust3dGetNamingArgument(state, "fn");
    if (!faceNormalStr) {
        fprintf(stderr, "Face normal missing\n");
        return -1;
    }
    faceOriginStr = dust3dGetNamingArgument(state, "fo");
    if (!faceOriginStr) {
        fprintf(stderr, "Face origin missing\n");
        return -1;
    }
    faceNormal = toVector3d(faceNormalStr);
    faceOrigin = toPoint3d(faceOriginStr);
    frontMesh = halfedgeSliceMeshByFace(dust3dGetCurrentMesh(state), &faceOrigin, &faceNormal);
    halfedgeDestroyMesh(frontMesh);
    return 0;
}

