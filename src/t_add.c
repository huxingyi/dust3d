#include <stdio.h>
#include <string.h>
#include "dust3d.h"
#include "util.h"

int theAdd(dust3dState *state) {
    const char *name;
    name = dust3dGetNthArgument(state, 0);
    if (!name) {
        fprintf(stderr, "Base mesh type missing\n");
        return -1;
    }
    if (0 == strcmp("plane", name)) {
        float w = dust3dGetNamingFloat(state, "w", 1.0);
        float d = dust3dGetNamingFloat(state, "d", 1.0);
        halfedgeCreatePlane(dust3dGetCurrentMesh(state), w, d);
    } else if (0 == strcmp("cube", name)) {
        float w = dust3dGetNamingFloat(state, "w", 1.0);
        float d = dust3dGetNamingFloat(state, "d", 1.0);
        float h = dust3dGetNamingFloat(state, "h", 1.0);
        halfedgeExtrudeFace(dust3dGetCurrentMesh(state), halfedgeCreatePlane(dust3dGetCurrentMesh(state), w, d), h);
    } else if (endsWith(name, ".obj")) {
        halfedgeImportObj(dust3dGetCurrentMesh(state), name);
    } else {
        fprintf(stderr, "Unrecognized base mesh type:\"%s\"\n", name);
        return -1;
    }
    return 0;
}
