#include <stdio.h>
#include <string.h>
#include "dust3d.h"
#include "util.h"
#include "matrix4x4.h"

int theMove(dust3dState *state) {
    const char *xStr;
    const char *yStr;
    const char *zStr;
    float x = 0;
    float y = 0;
    float z = 0;
    matrix4x4 mat;
    xStr = dust3dGetNamingArgument(state, "x");
    yStr = dust3dGetNamingArgument(state, "y");
    zStr = dust3dGetNamingArgument(state, "z");
    if (!xStr && !yStr && !zStr) {
        fprintf(stderr, "Please specify at least one dimmesion, for example \"x:1.0\"\n");
        return -1;
    }
    x = xStr ? toFloat(xStr) : 0.0f;
    y = yStr ? toFloat(yStr) : 0.0f;
    z = zStr ? toFloat(zStr) : 0.0f;
    matrix4x4Translation(&mat, x, y, z);
    halfedgeTransformMesh(dust3dGetCurrentMesh(state), &mat);
    return 0;
}
