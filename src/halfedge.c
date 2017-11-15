#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "halfedge.h"
#include "matrix.h"
#include "vector3d.h"

static face *newFace(mesh *m) {
    face *f = (face *)calloc(1, sizeof(face));
    if (m->lastFace) {
        m->lastFace->next = f;
        f->previous = m->lastFace;
    } else {
        m->firstFace = f;
    }
    m->lastFace = f;
    return f;
}

static vertex *newVertex(mesh *m, point3d position) {
    vertex *v = (vertex *)calloc(1, sizeof(vertex));
    v->position = position;
    if (m->lastVertex) {
        m->lastVertex->next = v;
        v->previous = m->lastVertex;
    } else {
        m->firstVertex = v;
    }
    m->lastVertex = v;
    return v;
}

static void deleteVertex(mesh *m, vertex *v) {
    if (v->next) {
        v->next->previous = v->previous;
    }
    if (v->previous) {
        v->previous->next = v->next;
    }
    if (m->lastVertex == v) {
        m->lastVertex = v->previous;
    }
    if (m->firstVertex == v) {
        m->firstVertex = v->next;
    }
    free(v);
}

static halfedge *newHalfedge(void) {
    return (halfedge *)calloc(1, sizeof(halfedge));
}

mesh *halfedgeCreateMesh(void) {
    return (mesh *)calloc(1, sizeof(mesh));
}

#define pair(first, second) do {\
    (first)->opposite = (second);\
    (second)->opposite = (first);\
} while (0)

#define link(first, second) do {\
    (first)->next = (second);\
    (second)->previous = (first);\
} while (0)

#define swap(type, first, second) do {\
    type tmp = (first);\
    (first) = (second);\
    (second) = tmp;\
} while (0)

int halfedgeSaveAsObj(mesh *m, const char *filename) {
    vertex *v = 0;
    face *f = 0;
    FILE *file = 0;
    int nextVertexIndex = 1;
    int nextFaceIndex = 1;
    file = fopen(filename, "wb");
    if (!file) {
        fprintf(stderr, "Open file \"%s\" failed.\n", filename);
        return -1;
    }
    for (v = m->firstVertex; v; v = v->next) {
        v->index = nextVertexIndex++;
        fprintf(file, "v %f %f %f\n", v->position.x, v->position.z, v->position.y);
    }
    for (f = m->firstFace; f; f = f->next) {
        vector3d normal;
        halfedgeFaceNormal(m, f, &normal);
        fprintf(file, "vn %f %f %f\n", normal.x, normal.z, normal.y);
    }
    for (f = m->firstFace; f; f = f->next, nextFaceIndex++) {
        halfedge *it = f->handle;
        halfedge *stop = it;
        fprintf(file, "f");
        do {
            fprintf(file, " %d//%d", it->start->index, nextFaceIndex);
            it = it->next;
        } while (it != stop);
        fprintf(file, "\n");
    }
    fclose(file);
    return 0;
}

struct createFaceContext {
    mesh *m;
    face *left;
    vertex *previousVertex;
    vertex *currentVertex;
    vertex *firstVertex;
    halfedge *previousHalfedge;
};

createFaceContext *halfedgeCreateFaceBegin(mesh *m) {
    createFaceContext *ctx = (createFaceContext *)calloc(1, sizeof(createFaceContext));
    ctx->m = m;
    return ctx;
}

static halfedge *halfedgeCreateFaceAddHalfedge(createFaceContext *ctx, halfedge *handle) {
    handle->left = ctx->left;
    handle->start = ctx->previousVertex;
    if (ctx->previousHalfedge) {
        ctx->previousHalfedge->next = handle;
        handle->previous = ctx->previousHalfedge;
    }
    ctx->previousVertex->handle = handle;
    if (!ctx->left->handle) {
        ctx->left->handle = handle;
    }
    ctx->previousVertex = ctx->currentVertex;
    ctx->previousHalfedge = handle;
    return handle;
}

halfedge *halfedgeCreateFaceAddPoint(createFaceContext *ctx, point3d point) {
    if (!ctx->left) {
        ctx->left = newFace(ctx->m);
        ctx->firstVertex = newVertex(ctx->m, point);
        ctx->previousVertex = ctx->firstVertex;
        return 0;
    }
    ctx->currentVertex = newVertex(ctx->m, point);
    return halfedgeCreateFaceAddHalfedge(ctx, newHalfedge());
}

halfedge *halfedgeCreateFaceAddVertex(createFaceContext *ctx, vertex *v) {
    if (!ctx->left) {
        ctx->left = newFace(ctx->m);
        ctx->firstVertex = v;
        ctx->previousVertex = ctx->firstVertex;
        return 0;
    }
    ctx->currentVertex = v;
    return halfedgeCreateFaceAddHalfedge(ctx, newHalfedge());
}

halfedge *halfedgeCreateFaceEnd(createFaceContext *ctx) {
    halfedge *handle;
    handle = newHalfedge();
    ctx->currentVertex = ctx->firstVertex;
    handle->next = ctx->left->handle;
    halfedgeCreateFaceAddHalfedge(ctx, handle);
    ctx->left->handle->previous = handle;
    free(ctx);
    return handle;
}

face *halfedgeCreateFaceFromPoints(mesh *m, const point3d *counterClockWisedPoints, int count) {
    int i;
    createFaceContext *ctx;
    ctx = halfedgeCreateFaceBegin(m);
    for (i = 0; i < count; i++) {
        halfedgeCreateFaceAddPoint(ctx, counterClockWisedPoints[i]);
    }
    return halfedgeCreateFaceEnd(ctx)->left;
}

face *halfedgeCreatePlane(mesh *m, float radius) {
    point3d points[] = {
        {-radius, -radius, 0}, {radius, -radius, 0}, {radius, radius, 0}, {-radius, radius, 0}
    };
    return halfedgeCreateFaceFromPoints(m, points, 4);
}

int halfedgeFaceNormal(mesh *m, face *f, vector3d *normal) {
    halfedge *it = f->handle;
    halfedge *stop = it;
    point3d *points[3] = {0};
    int num = 0;
    int total = 0;
    vector3d v;
    normal->x = normal->y = normal->z = 0;
    do {
        points[num % 3] = &it->start->position;
        num++;
        if (num >= 3) {
            vector3dNormal(points[(num - 3) % 3], points[(num - 2) % 3], points[(num - 1) % 3], &v);
            vector3dAdd(normal, &v);
            total++;
        }
        it = it->next;
    } while (it != stop);
    vector3dDivide(normal, total);
    return 0;
}

face *halfedgeFlipFace(mesh *m, face *f) {
    halfedge *it = f->handle;
    halfedge *stop = it;
    vertex *last;
    do {
        halfedge *current = it;
        it = it->next;
        swap(halfedge *, current->previous, current->next);
    } while (it != stop);
    it = f->handle;
    last = it->previous->start;
    do {
        swap(vertex *, last, it->start);
        it = it->next;
    } while (it != stop);
    return f;
}

face *halfedgeExtrudeFace(mesh *m, face *f, float radius) {
    vector3d alongv;
    halfedge *stitchFrom = 0;
    face *extrudedFace = 0;
    face *needFlipFace = 0;
    matrix mat;
    halfedgeFaceNormal(m, f, &alongv);
    vector3dMultiply(&alongv, radius);
    matrixTranslation(&mat, alongv.x, alongv.y, alongv.z);
    if (f->handle->opposite) {
        stitchFrom = f->handle->opposite;
        extrudedFace = halfedgeCutFace(m, f);
    } else {
        stitchFrom = f->handle;
        extrudedFace = halfedgeCopyFace(m, f);
        needFlipFace = f;
    }
    halfedgeTransformFace(m, extrudedFace, &mat);
    halfedgeStitch(m, stitchFrom, extrudedFace->handle);
    if (needFlipFace) {
        halfedgeFlipFace(m, needFlipFace);
    }
    return extrudedFace;
}

face *halfedgeCutFace(mesh *m, face *f) {
    halfedge *it = f->handle;
    halfedge *stop = it;
    do {
        if (it->start->handle->left == f) {
            it->start->handle = 0;
        }
        it->opposite->opposite = 0;
        it->start = newVertex(m, it->start->position);
        it->start->handle = it;
        it = it->next;
    } while (it != stop);
    return f;
}

face *halfedgeCopyFace(mesh *m, face *f) {
    createFaceContext *ctx;
    halfedge *it = f->handle;
    halfedge *stop = it;
    ctx = halfedgeCreateFaceBegin(m);
    do {
        halfedgeCreateFaceAddPoint(ctx, it->start->position);
        it = it->next;
    } while (it != stop);
    return halfedgeCreateFaceEnd(ctx)->left;
}

int halfedgeStitch(mesh *m, halfedge *from, halfedge *to) {
    halfedge *itFrom = from;
    halfedge *stopFrom = itFrom;
    halfedge *itTo = to;
    halfedge *stopTo = itTo;
    halfedge *head = 0;
    halfedge *last = 0;
    do {
        createFaceContext *ctx;
        halfedge *handle;
        halfedge *second;
        ctx = halfedgeCreateFaceBegin(m);
        halfedgeCreateFaceAddVertex(ctx, itFrom->start);
        handle = halfedgeCreateFaceAddVertex(ctx, itFrom->next->start);
        pair(handle, itFrom);
        second = halfedgeCreateFaceAddVertex(ctx, itTo->next->start);
        handle = halfedgeCreateFaceAddVertex(ctx, itTo->start);
        pair(handle, itTo);
        handle = halfedgeCreateFaceEnd(ctx);
        if (!head) {
            head = handle;
        } else {
            pair(handle, last);
        }
        last = second;
        itFrom = itFrom->next;
        itTo = itTo->next;
    } while (itFrom != stopFrom && itTo != stopTo);
    pair(head, last);
    return 0;
}

int halfedgeTransformFace(mesh *m, face *f, matrix *mat) {
    halfedge *it = f->handle;
    halfedge *stop = it;
    do {
        matrixTransformXYZ(mat, &it->start->position.x, &it->start->position.y, &it->start->position.z);
        it = it->next;
    } while (it != stop);
    return 0;
}

face *halfedgeChamferVertex(mesh *m, vertex *v, float firstRadius, float secondRadius, float thirdRadius) {
    face *tri;
    int i;
    halfedge *cornerHalfedges[6];
    halfedge *newInnerHalfedges[3];
    halfedge *newOutterHalfedges[3];
    const int outterAdjcents[3][2] = {{5, 0}, {3, 4}, {1, 2}};
    point3d newPoints[3];
    cornerHalfedges[0] = v->handle;
    cornerHalfedges[1] = cornerHalfedges[0]->opposite;
    cornerHalfedges[2] = cornerHalfedges[1]->next;
    cornerHalfedges[3] = cornerHalfedges[2]->opposite;
    cornerHalfedges[4] = cornerHalfedges[3]->next;
    cornerHalfedges[5] = cornerHalfedges[4]->opposite;
    vector3dLerp(&v->position, &cornerHalfedges[1]->start->position, firstRadius, &newPoints[0]);
    vector3dLerp(&v->position, &cornerHalfedges[5]->start->position, secondRadius, &newPoints[1]);
    vector3dLerp(&v->position, &cornerHalfedges[3]->start->position, thirdRadius, &newPoints[2]);
    tri = halfedgeCreateFaceFromPoints(m, newPoints, 3);
    newInnerHalfedges[0] = tri->handle;
    newInnerHalfedges[1] = tri->handle->next;
    newInnerHalfedges[2] = tri->handle->next->next;
    for (i = 0; i < 3; i++) {
        newOutterHalfedges[i] = newHalfedge();
        newOutterHalfedges[i]->start = newInnerHalfedges[i]->next->start;
        pair(newInnerHalfedges[i], newOutterHalfedges[i]);
    }
    for (i = 0; i < 3; i++) {
        const int first = outterAdjcents[i][0];
        const int second = outterAdjcents[i][1];
        newOutterHalfedges[i]->left = cornerHalfedges[first]->left;
        link(cornerHalfedges[first], newOutterHalfedges[i]);
        link(newOutterHalfedges[i], cornerHalfedges[second]);
        cornerHalfedges[second]->start = newInnerHalfedges[i]->start;
    }
    deleteVertex(m, v);
    return newInnerHalfedges[0]->left;
}
