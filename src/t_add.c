#include <stdio.h>
#include <string.h>
#include "dust3d.h"
#include "strutil.h"

int theAdd(dust3dState *state) {
    float radius = 1.0;
    const char *radiusStr;
    const char *name;
    name = dust3dGetNthArgument(state, 0);
    if (!name) {
        fprintf(stderr, "Base mesh type missing\n");
        return -1;
    }
    radiusStr = dust3dGetNamingArgument(state, "r");
    if (radiusStr) {
        radius = toFloat(radiusStr);
    }
    if (0 == strcmp("cube", name)) {
        halfedgeExtrudeFace(dust3dGetCurrentMesh(state), halfedgeCreatePlane(dust3dGetCurrentMesh(state), radius), radius * 2);
    } else if (endsWith(name, ".obj")) {
        halfedgeImportObj(dust3dGetCurrentMesh(state), name);
    } else {
        fprintf(stderr, "Unrecognized base mesh type:\"%s\"\n", name);
        return -1;
    }
    return 0;
}
