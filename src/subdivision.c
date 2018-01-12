#include <stdlib.h>
#include <string.h>
#include "subdivision.h"
#include "util.h"
#include "math3d.h"

typedef struct subdivisionFaceData {
    point3d averageOfPoints;
    vertex *generatedVertex;
    struct subdivisionFaceData *next;
} subdivisionFaceData;

typedef struct subdivisionEdgeData {
    point3d midPoint;
    vertex *generatedVertex;
    struct subdivisionEdgeData *next;
} subdivisionEdgeData;

typedef struct subdivisionVertexData {
    vertex *generatedVertex;
    struct subdivisionVertexData *next;
} subdivisionVertexData;

typedef struct subdivisionContext {
    subdivisionFaceData *faceDataList;
    subdivisionEdgeData *edgeDataList;
    subdivisionVertexData *vertexDataList;
    mesh *newMesh;
} subdivisionContext;

static subdivisionFaceData *getFaceData(subdivisionContext *ctx, face *f) {
    subdivisionFaceData *faceData;
    if (f->data) {
        return (subdivisionFaceData *)f->data;
    }
    faceData = (subdivisionFaceData *)calloc(1, sizeof(subdivisionFaceData));
    f->data = faceData;
    halfedgeFaceCenter(f, &faceData->averageOfPoints);
    faceData->generatedVertex = halfedgeCreateVertex(ctx->newMesh, &faceData->averageOfPoints);
    addToList(faceData, ctx->faceDataList);
    return faceData;
}

static subdivisionEdgeData *getEdgeData(subdivisionContext *ctx, halfedge *h) {
    subdivisionEdgeData *edgeData;
    subdivisionFaceData *f1Data;
    subdivisionFaceData *f2Data;
    point3d *points[4];
    point3d averageOfFacesAndEndpoints;
    if (!halfedgeMakeSureOnlyOnce(h)) {
        return getEdgeData(ctx, h->opposite);
    }
    if (h->data) {
        return (subdivisionEdgeData *)h->data;
    }
    edgeData = (subdivisionEdgeData *)calloc(1, sizeof(subdivisionEdgeData));
    halfedgeEdgeCenter(h, &edgeData->midPoint);
    f1Data = getFaceData(ctx, h->left);
    f2Data = getFaceData(ctx, h->opposite->left);
    points[0] = &f1Data->averageOfPoints;
    points[1] = &f2Data->averageOfPoints;
    points[2] = &h->start->position;
    points[3] = &h->next->start->position;
    math3dAveragePointOfPoints(points, 4, &averageOfFacesAndEndpoints);
    h->data = edgeData;
    edgeData->generatedVertex = halfedgeCreateVertex(ctx->newMesh, &averageOfFacesAndEndpoints);
    addToList(edgeData, ctx->edgeDataList);
    return edgeData;
}

static subdivisionVertexData *getVertexData(subdivisionContext *ctx, vertex *v) {
    subdivisionVertexData *vertexData;
    halfedge *h;
    halfedge *stop;
    point3d averageOfFaces;
    point3d averageOfEdgeMidpoints;
    point3d tmpPoint;
    int faceNum = 0;
    point3d baryCenter;
    if (v->data) {
        return (subdivisionVertexData *)v->data;
    }
    vertexData = (subdivisionVertexData *)calloc(1, sizeof(subdivisionVertexData));
    h = v->handle;
    stop = h;
    averageOfFaces.x = averageOfFaces.y = averageOfFaces.z = 0;
    averageOfEdgeMidpoints.x = averageOfEdgeMidpoints.y = averageOfEdgeMidpoints.z = 0;
    baryCenter.x = baryCenter.y = baryCenter.z = 0;
    do {
        subdivisionFaceData *faceData = getFaceData(ctx, h->left);
        subdivisionEdgeData *edgeData = getEdgeData(ctx, h);
        vector3dAdd(&averageOfFaces, &faceData->averageOfPoints);
        vector3dAdd(&averageOfEdgeMidpoints, &edgeData->midPoint);
        faceNum++;
        h = h->previous->opposite;
    } while (h != stop);
    vector3dDivide(&averageOfFaces, faceNum);
    vector3dDivide(&averageOfEdgeMidpoints, faceNum);
    baryCenter = averageOfFaces;
    tmpPoint = averageOfEdgeMidpoints;
    vector3dMultiply(&tmpPoint, 2);
    vector3dAdd(&baryCenter, &tmpPoint);
    tmpPoint = v->position;
    vector3dMultiply(&tmpPoint, faceNum - 3);
    vector3dAdd(&baryCenter, &tmpPoint);
    vector3dDivide(&baryCenter, faceNum);
    v->data = vertexData;
    vertexData->generatedVertex = halfedgeCreateVertex(ctx->newMesh, &baryCenter);
    return vertexData;
}

mesh *subdivisionCreateSubdividedMesh(mesh *m) {
    face *itFace;
    subdivisionContext ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.newMesh = halfedgeCreateMesh();
    for (itFace = m->firstFace; itFace; itFace = itFace->next) {
        halfedge *h = itFace->handle;
        halfedge *stop = h;
        subdivisionFaceData *faceData = getFaceData(&ctx, itFace);
        do {
            subdivisionEdgeData *e1Data = getEdgeData(&ctx, h);
            subdivisionEdgeData *e2Data = getEdgeData(&ctx, h->next);
            subdivisionVertexData *vertexData = getVertexData(&ctx, h->next->start);
            halfedge *newHalfedges[4];
            face *generatedFace = halfedgeCreateFace(ctx.newMesh);
            newHalfedges[0] = halfedgeCreateHalfedge();
            newHalfedges[1] = halfedgeCreateHalfedge();
            newHalfedges[2] = halfedgeCreateHalfedge();
            newHalfedges[3] = halfedgeCreateHalfedge();
            newHalfedges[0]->start = faceData->generatedVertex;
            newHalfedges[1]->start = e1Data->generatedVertex;
            newHalfedges[2]->start = vertexData->generatedVertex;
            newHalfedges[3]->start = e2Data->generatedVertex;
            newHalfedges[0]->left = generatedFace;
            newHalfedges[1]->left = generatedFace;
            newHalfedges[2]->left = generatedFace;
            newHalfedges[3]->left = generatedFace;
            halfedgeLinkHalfeges(newHalfedges[0], newHalfedges[1]);
            halfedgeLinkHalfeges(newHalfedges[1], newHalfedges[2]);
            halfedgeLinkHalfeges(newHalfedges[2], newHalfedges[3]);
            halfedgeLinkHalfeges(newHalfedges[3], newHalfedges[0]);
            generatedFace->handle = newHalfedges[0];
            faceData->generatedVertex->handle = newHalfedges[0];
            e1Data->generatedVertex->handle = newHalfedges[1];
            vertexData->generatedVertex->handle = newHalfedges[2];
            e2Data->generatedVertex->handle = newHalfedges[3];
            h = h->next;
        } while (h != stop);
    }
    freeList(subdivisionFaceData, ctx.faceDataList);
    freeList(subdivisionEdgeData, ctx.edgeDataList);
    freeList(subdivisionVertexData, ctx.vertexDataList);
    halfedgeFixPairing(ctx.newMesh);
    return ctx.newMesh;
}

