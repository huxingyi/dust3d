#include <stdio.h>
#include "dust3d.h"
#include "strutil.h"

int theNew(dust3dState *state) {
    const char *id = dust3dGetNthArgument(state, 0);
    if (!id) {
        fprintf(stderr, "Mesh id missing\n");
        return -1;
    }
    if (dust3dFindMeshById(state, id)) {
        fprintf(stderr, "Mesh id \"%s\" alreay exists\n", id);
        return -1;
    }
    dust3dCreateMeshById(state, id);
    return 0;
}
