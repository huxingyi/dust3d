#ifndef TRI2QUAD_H
#define TRI2QUAD_H
#include "draw.h"

typedef struct tri2QuadContext tri2QuadContext;

tri2QuadContext *tri2QuadContextCreate(void);
int tri2QuadAddTriangle(tri2QuadContext *ctx, triangle *tri);
int tri2QuadConvert(tri2QuadContext *ctx);
int tri2QuadGetTriangleNum(tri2QuadContext *ctx);
triangle *tri2QuadGetTriangle(tri2QuadContext *ctx, int index);
int tri2QuadGetQuadNum(tri2QuadContext *ctx);
quad *tri2QuadGetQuad(tri2QuadContext *ctx, int index);
void tri2QuadContextDestroy(tri2QuadContext *ctx);

#endif

