#include <stdio.h>
#include "dust3d.h"
#include "util.h"

int theSave(dust3dState *state) {
    const char *name = dust3dGetNthArgument(state, 0);
    if (!name) {
        fprintf(stderr, "File name missing\n");
        return -1;
    }
    if (endsWith(name, ".obj")) {
        halfedgeSaveMeshAsObj(dust3dGetCurrentMesh(state), name);
    } else {
        fprintf(stderr, "Unrecognized file type:\"%s\"\n", name);
        return -1;
    }
    return 0;
}
