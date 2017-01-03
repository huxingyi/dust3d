#ifndef SUBDIVIDE_H
#define SUBDIVIDE_H
#include "array.h"
#include "3dstruct.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct subdivModel subdivModel;

subdivModel *subdivCreateModel(void);
void subdivDestroyModel(subdivModel *model);
int subdivAddVertex(subdivModel *model, vec3 *v);
int subdivAddTriangleFace(subdivModel *model, int p1, int p2, int p3);
int subdivAddQuadFace(subdivModel *model, int p1, int p2, int p3, int p4);
subdivModel *subdivCatmullClark(subdivModel *model);
subdivModel *subdivCatmullClarkWithLoops(subdivModel *model, int loops);
int subdivAddCube(subdivModel *model);
int subdivCalculteNorms(subdivModel *model);
void subdivDrawModel(subdivModel *model);

#ifdef __cplusplus
}
#endif

#endif
