#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "halfedge.h"
#include "matrix.h"
#include "vector3d.h"

#define min(x, y) (((x) < (y)) ? (x) : (y))

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

static void setFaceName(face *f, const char *name) {
    strncpy(f->name, name, sizeof(f->name) - 1);
    f->name[sizeof(f->name) - 1] = '\0';
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

#if DUST3D_DEBUG
static vertex *newVertexAtLine(mesh *m, point3d position, long line) {
    vertex *v = newVertex(m, position);
    v->line_ = line;
    return v;
}
#define newVertex(m, p) newVertexAtLine((m), (p), __LINE__)
#endif

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

static void deleteFace(mesh *m, face *f) {
    if (f->next) {
        f->next->previous = f->previous;
    }
    if (f->previous) {
        f->previous->next = f->next;
    }
    if (m->lastFace == f) {
        m->lastFace = f->previous;
    }
    if (m->firstFace == f) {
        m->firstFace = f->next;
    }
    free(f);
}

static void deleteHalfedge(mesh *m, halfedge *h) {
    free(h);
}

static void deleteEdge(mesh *m, halfedge *h) {
    deleteHalfedge(m, h->opposite);
    deleteHalfedge(m, h);
}

static halfedge *newHalfedge(void) {
    return (halfedge *)calloc(1, sizeof(halfedge));
}

mesh *halfedgeCreateMesh(void) {
    return (mesh *)calloc(1, sizeof(mesh));
}

#define pair(first, second) do {\
    halfedge *firstValue = (first);\
    halfedge *secondValue = (second);\
    (first)->opposite = (secondValue);\
    (second)->opposite = (firstValue);\
} while (0)

#define link(first, second) do {\
    halfedge *firstValue = (first);\
    halfedge *secondValue = (second);\
    (first)->next = (secondValue);\
    (second)->previous = (firstValue);\
} while (0)

#define swap(type, first, second) do {\
    type tmp = (first);\
    (first) = (second);\
    (second) = tmp;\
} while (0)

int halfedgeSaveMeshAsObj(mesh *m, const char *filename) {
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

int halfedgeSaveFaceAsObj(mesh *m, face *f, const char *filename) {
    halfedge *it = f->handle;
    int nextVertexIndex = 1;
    int nextFaceIndex = 1;
    vector3d normal;
    FILE *file;
    file = fopen(filename, "wb");
    if (!file) {
        fprintf(stderr, "Open file \"%s\" failed.\n", filename);
        return -1;
    }
    do {
        it->start->index = nextVertexIndex++;
        fprintf(file, "v %f %f %f\n", it->start->position.x, it->start->position.z, it->start->position.y);
        it = it->next;
    } while (it != f->handle);
    halfedgeFaceNormal(m, f, &normal);
    fprintf(file, "vn %f %f %f\n", normal.x, normal.z, normal.y);
    it = f->handle;
    fprintf(file, "f");
    do {
        fprintf(file, " %d//%d", it->start->index, nextFaceIndex);
        it = it->next;
    } while (it != f->handle);
    fprintf(file, "\n");
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

face *halfedgeChamferVertex(mesh *m, vertex *v, float ammount) {
    halfedge *itAdjecent = v->handle;
    halfedge *stopAdjecent = itAdjecent;
    createFaceContext *ctx = 0;
    halfedge *lastOutter = 0;
    halfedge *lastInner = 0;
    vertex *lastVertex = 0;
    vertex *firstVertex = 0;
    if (halfedgeIsBoundaryVertex(m, v)) {
        return 0;
    }
    ctx = halfedgeCreateFaceBegin(m);
    do {
        point3d segPoint;
        halfedge *current = itAdjecent;
        itAdjecent = itAdjecent->previous->opposite;
        vector3dLerp(&v->position, &current->opposite->start->position, ammount, &segPoint);
        lastVertex = newVertex(m, segPoint);
        if (!firstVertex) {
            firstVertex = lastVertex;
        }
        if (lastOutter) {
            lastOutter->start = lastVertex;
        }
        lastInner = halfedgeCreateFaceAddVertex(ctx, lastVertex);
        if (lastInner && lastOutter) {
            pair(lastInner, lastOutter);
        }
        lastOutter = newHalfedge();
        lastOutter->left = current->left;
        link(current->previous, lastOutter);
        link(lastOutter, current);
        current->start = lastVertex;
    } while (itAdjecent != stopAdjecent);
    lastInner = halfedgeCreateFaceEnd(ctx);
    lastOutter->start = firstVertex;
    pair(lastInner, lastOutter);
    deleteVertex(m, v);
    return lastInner->left;
}

int halfedgeIsBoundaryVertex(mesh *m, vertex *v) {
    halfedge *it = v->handle;
    if (!it->opposite) {
        return 1;
    }
    do {
        it = it->previous->opposite;
    } while (it && it != v->handle);
    return 0 == it;
}

static void changeFace(mesh *m, face *o, face *n) {
    halfedge *it = o->handle;
    do {
        it->left = n;
        it = it->next;
    } while (it != o->handle);
}

face *halfedgeChamferEdge(mesh *m, halfedge *h, float ammount) {
    vertex *rightVertex = h->start;
    vertex *leftVertex = h->opposite->start;

    face *leftFace = halfedgeChamferVertex(m, leftVertex, ammount);
    face *rightFace = halfedgeChamferVertex(m, rightVertex, ammount);

    vertex *lt = h->previous->start;
    vertex *rt = h->next->next->start;
    vertex *rb = h->opposite->previous->start;
    vertex *lb = h->opposite->next->next->start;

    halfedge *topOutter = newHalfedge();
    halfedge *bottomOutter = newHalfedge();
    halfedge *topInner = newHalfedge();
    halfedge *bottomInner = newHalfedge();

    topOutter->start = lt;
    topOutter->left = h->previous->previous->left;
    topOutter->left->handle = topOutter;
    link(h->previous->previous, topOutter);
    link(topOutter, h->next->next);

    bottomOutter->start = rb;
    bottomOutter->left = h->opposite->previous->previous->left;
    bottomOutter->left->handle = bottomOutter;
    link(h->opposite->previous->previous, bottomOutter);
    link(bottomOutter, h->opposite->next->next);

    topInner->start = rt;
    link(topInner, h->previous->opposite->next);
    link(h->next->opposite->previous, topInner);

    bottomInner->start = lb;
    link(bottomInner, h->opposite->previous->opposite->next);
    link(h->opposite->next->opposite->previous, bottomInner);

    pair(topInner, topOutter);
    pair(bottomInner, bottomOutter);

    h->previous->start->handle = h->previous->opposite->next;
    h->next->opposite->start->handle = h->next->opposite->previous->opposite;
    h->opposite->previous->start->handle = h->opposite->previous->opposite->next;
    h->opposite->next->opposite->start->handle = h->opposite->next->opposite->previous->opposite;
    deleteVertex(m, h->start);
    deleteVertex(m, h->opposite->start);
    deleteEdge(m, h->previous);
    deleteEdge(m, h->next);
    deleteEdge(m, h->opposite->previous);
    deleteEdge(m, h->opposite->next);
    deleteEdge(m, h);

    leftFace->handle = topInner;
    changeFace(m, leftFace, leftFace);
    deleteFace(m, rightFace);

    return leftFace;
}

void halfedgeVector3d(mesh *m, halfedge *h, vector3d *v) {
    *v = h->start->position;
    vector3dSub(v, &h->next->start->position);
}

halfedge *halfedgeEdgeLoopNext(mesh *m, halfedge *h) {
    halfedge *it = h;
    float selectedDegree = 0;
    halfedge *selectedHalfedge = 0;
    vector3d a, b;
    float degree;
    halfedgeVector3d(m, h, &a);
    do {
        halfedgeVector3d(m, it->next, &b);
        degree = vector3dAngle(&a, &b);
        if (selectedDegree <= selectedDegree) {
            selectedDegree = degree;
            selectedHalfedge = it->next;
        }
        it = it->next->opposite;
    } while (it && it != h);
    return selectedHalfedge;
}
