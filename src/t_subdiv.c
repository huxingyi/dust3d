#include "dust3d.h"
#include "subdivision.h"

int theSubdiv(dust3dState *state) {
    mesh *m = dust3dGetCurrentMesh(state);
    mesh *subdividedMesh = subdivisionCreateSubdividedMesh(m);
    halfedgeDestroyMesh(m);
    dust3dSetCurrentMesh(state, subdividedMesh);
    return 0;
}

