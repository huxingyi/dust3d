#include <stdio.h>
#include "dust3d.h"
#include "strutil.h"

int theSel(dust3dState *state) {
    mesh *m;
    const char *id = dust3dGetNthArgument(state, 0);
    if (!id) {
        fprintf(stderr, "Mesh id missing\n");
        return -1;
    }
    if (!(m=dust3dFindMeshById(state, id))) {
        fprintf(stderr, "Mesh id \"%s\" not found\n", id);
        return -1;
    }
    dust3dSetCurrentMesh(state, m);
    return 0;
}
