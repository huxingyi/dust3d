#include <stdio.h>
#include <string.h>
#include "dust3d.h"

int theAdd(dust3dState *state) {
    const char *base = dust3dGetNthArgument(state, 0);
    float radius = 1.0;
    if (!base) {
        fprintf(stderr, "Base mesh type missing\n");
        return -1;
    }
    if (0 == strcmp("cube", base)) {
        halfedgeExtrudeFace(dust3dGetCurrentMesh(state), halfedgeCreatePlane(dust3dGetCurrentMesh(state), radius), radius * 2);
    } else {
        fprintf(stderr, "Unrecognized base mesh type:\"%s\"\n", base);
        return -1;
    }
    return 0;
}
