#include <stdio.h>
#include "dust3d.h"
#include "strutil.h"
#include "selector.h"
#include "color.h"

int theWrap(dust3dState *state) {
    const char *meshId1;
    const char *meshId2;
    mesh *m1;
    mesh *m2;
    meshId1 = dust3dGetNthArgument(state, 0);
    if (!meshId1) {
        fprintf(stderr, "Mesh id 1 missing\n");
        return -1;
    }
    m1 = dust3dFindMeshById(state, meshId1);
    if (!m1) {
        fprintf(stderr, "Mesh id \"%s\" not found\n", meshId1);
        return -1;
    }
    meshId2 = dust3dGetNthArgument(state, 1);
    if (!meshId2) {
        fprintf(stderr, "Mesh id 2 missing\n");
        return -1;
    }
    m2 = dust3dFindMeshById(state, meshId2);
    if (!m2) {
        fprintf(stderr, "Mesh id \"%s\" not found\n", meshId2);
        return -1;
    }
    halfedgeJoinMesh(m1, m2);
    dust3dSetCurrentMesh(state, m1);
    selectorArrangeMesh(m1);
    {
        face *f1 = selectorGetNthFace(m1, 9);
        face *f2 = selectorGetNthFace(m1, 3);
        f1->color = RED;
        f2->color = BLUE;
        halfedgeStitchTwoFaces(m1, f1, f2);
    }
    return 0;
}
