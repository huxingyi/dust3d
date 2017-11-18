#ifndef DUST3D_HALFEDGE_H
#define DUST3D_HALFEDGE_H
#include "vector3d.h"
#include "matrix.h"

typedef struct halfedge halfedge;

typedef struct vertex {
    point3d position;
    halfedge *handle;
    struct vertex *previous;
    struct vertex *next;
    int index;
#if DUST3D_DEBUG
    long line_;
#endif
} vertex;

#define MAX_FACE_NAME_SIZE  8

typedef struct face {
    halfedge *handle;
    struct face *previous;
    struct face *next;
    char name[MAX_FACE_NAME_SIZE];
} face;

struct halfedge {
    vertex *start;
    face *left;
    halfedge *next;
    halfedge *previous;
    halfedge *opposite;
};

typedef struct mesh {
    vertex *firstVertex;
    vertex *lastVertex;
    face *firstFace;
    face *lastFace;
} mesh;

typedef struct createFaceContext createFaceContext;

mesh *halfedgeCreateMesh(void);
int halfedgeSaveMeshAsObj(mesh *m, const char *filename);
int halfedgeSaveFaceAsObj(mesh *m, face *f, const char *filename);
createFaceContext *halfedgeCreateFaceBegin(mesh *m);
halfedge *halfedgeCreateFaceAddPoint(createFaceContext *ctx, point3d point);
halfedge *halfedgeCreateFaceEnd(createFaceContext *ctx);
face *halfedgeCreateFaceFromPoints(mesh *m, const point3d *counterClockWisedPoints, int count);
face *halfedgeCreatePlane(mesh *m, float radius);
face *halfedgeCopyFace(mesh *m, face *f);
face *halfedgeCutFace(mesh *m, face *f);
face *halfedgeExtrudeFace(mesh *m, face *f, float radius);
face *halfedgeFlipFace(mesh *m, face *f);
int halfedgeFaceNormal(mesh *m, face *f, vector3d *normal);
int halfedgeTransformFace(mesh *m, face *f, matrix *mat);
int halfedgeStitch(mesh *m, halfedge *from, halfedge *to);
face *halfedgeChamferVertex(mesh *m, vertex *v, float ammount);
int halfedgeIsBoundaryVertex(mesh *m, vertex *v);
face *halfedgeChamferEdge(mesh *m, halfedge *h, float ammount);

#endif
