#ifndef DUST3D_FUNCTIONLIST_H
#define DUST3D_FUNCTIONLIST_H
#include "dust3d.h"

int theAdd(dust3dState *state);
int theChamfer(dust3dState *state);
int theExtrude(dust3dState *state);
int theShow(dust3dState *state);
int theSlice(dust3dState *state);
int theSubdiv(dust3dState *state);
int theSave(dust3dState *state);

#define rust3dRegisterFunctionList(state)   do {        \
    dust3dRegister((state), "add", theAdd);             \
    dust3dRegister((state), "chamfer", theChamfer);     \
    dust3dRegister((state), "extrude", theExtrude);     \
    dust3dRegister((state), "show", theShow);           \
    dust3dRegister((state), "slice", theSlice);         \
    dust3dRegister((state), "subdiv", theSubdiv);       \
    dust3dRegister((state), "save", theSave);           \
} while (0)

#endif
