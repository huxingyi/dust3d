#ifndef DUST3D_HALFEDGE_H
#define DUST3D_HALFEDGE_H
#include "vector3d.h"
#include "matrix.h"
#include "rbtree.h"

typedef struct halfedge halfedge;

typedef struct vertex {
    point3d position;
    halfedge *handle;
    struct vertex *previous;
    struct vertex *next;
    int index;
    int front:1;
#if DUST3D_DEBUG
    long line_;
#endif
} vertex;

#define MAX_FACE_NAME_SIZE  8

typedef struct face {
    halfedge *handle;
    struct face *previous;
    struct face *next;
    int color;
    int order;
    char name[MAX_FACE_NAME_SIZE];
} face;

struct halfedge {
    vertex *start;
    face *left;
    halfedge *next;
    halfedge *previous;
    halfedge *opposite;
    int color;
    int order;
};

typedef struct mesh {
    vertex *firstVertex;
    vertex *lastVertex;
    face *firstFace;
    face *lastFace;
} mesh;

mesh *halfedgeCreateMesh(void);
void halfedgeResetMesh(mesh *m);
void halfedgeDestroyMesh(mesh *m);
int halfedgeSaveMeshAsObj(mesh *m, const char *filename);
int halfedgeSaveFaceAsObj(mesh *m, face *f, const char *filename);
face *halfedgeCreateFaceFromPoints(mesh *m, point3d *counterClockWisedPoints, int count);
face *halfedgeCreatePlane(mesh *m, float width, float depth);
face *halfedgeCopyFace(mesh *m, face *f);
face *halfedgeCutFace(mesh *m, face *f);
face *halfedgeExtrudeFace(mesh *m, face *f, float radius);
int halfedgeFlipFace(face *f);
int halfedgeFaceNormal(face *f, vector3d *normal);
int halfedgeFaceCenter(face *f, point3d *point);
int halfedgeMeshCenter(mesh *m, point3d *point);
int halfedgeTransformFace(face *f, matrix *mat);
int halfedgeTransformMesh(mesh *m, matrix *mat);
int halfedgeStitch(mesh *m, halfedge *from, halfedge *to);
face *halfedgeChamferVertex(mesh *m, vertex *v, float amount);
int halfedgeIsBoundaryVertex(mesh *m, vertex *v);
void halfedgeVector3d(mesh *m, halfedge *h, vector3d *v);
mesh *halfedgeSliceMeshByPlane(mesh *m, point3d *facePoint, vector3d *faceNormal, int coincidenceAsFront);
mesh *halfedgeSplitMeshBySide(mesh *m);
#define halfedgeMakeSureOnlyOnce(h) ((h)->opposite && (h)->start < (h)->opposite->start)
int halfedgeImportObj(mesh *m, const char *filename);
int halfedgeFixHoles(mesh *m);
int halfedgeIntersectMesh(mesh *m, mesh *by);
mesh *halfedgeSplitMeshByMesh(mesh *m, mesh *by);
int halfedgeFlipMesh(mesh *m);
int halfedgeJoinMesh(mesh *m, mesh *sub);
mesh *halfedgeCloneMesh(mesh *m);
int halfedgeIsEmptyMesh(mesh *m);
int halfedgeConcatMesh(mesh *m, mesh *src);
int halfedgeCombineDuplicateVertices(mesh *m);
int halfedgeFixPairing(mesh *m);
int halfedgeFixTjunction(mesh *m);
int halfedgeIsWatertight(mesh *m);
int halfedgeUnionMesh(mesh *m, mesh *by);
int halfedgeDiffMesh(mesh *m, mesh *by);
int halfedgeFixAll(mesh *m);
int halfedgeWrapMesh(mesh *m, mesh *sub);
int halfedgeStitchTwoFaces(mesh *m, face *f1, face *f2);
void halfedgeDestroyFace(mesh *m, face *f);

#endif
