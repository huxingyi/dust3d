#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "halfedge.h"
#include "matrix.h"
#include "vector3d.h"
#include "geometry.h"
#include "rbtree.h"
#include "array.h"

#define min(x, y) (((x) < (y)) ? (x) : (y))

static void addFace(mesh *m, face *f) {
    f->next = 0;
    if (m->lastFace) {
        m->lastFace->next = f;
    } else {
        m->firstFace = f;
    }
    f->previous = m->lastFace;
    m->lastFace = f;
}

static face *newFace(mesh *m) {
    face *f = (face *)calloc(1, sizeof(face));
    addFace(m, f);
    return f;
}

static void setFaceName(face *f, const char *name) {
    strncpy(f->name, name, sizeof(f->name) - 1);
    f->name[sizeof(f->name) - 1] = '\0';
}

static void addVertex(mesh *m, vertex *v) {
    v->next = 0;
    if (m->lastVertex) {
        m->lastVertex->next = v;
    } else {
        m->firstVertex = v;
    }
    v->previous = m->lastVertex;
    m->lastVertex = v;
}

static vertex *newVertex(mesh *m, point3d *position) {
    vertex *v = (vertex *)calloc(1, sizeof(vertex));
    v->position = *position;
    addVertex(m, v);
    return v;
}

#if DUST3D_DEBUG
static vertex *newVertexAtLine(mesh *m, point3d *position, long line) {
    vertex *v = newVertex(m, position);
    v->line_ = line;
    return v;
}
#define newVertex(m, p) newVertexAtLine((m), (p), __LINE__)
#endif

static void removeVertex(mesh *m, vertex *v) {
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
}

static void deleteVertex(mesh *m, vertex *v) {
    removeVertex(m, v);
    free(v);
}

static void removeFace(mesh *m, face *f) {
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
}

static void deleteFace(mesh *m, face *f) {
    removeFace(m, f);
    free(f);
}

static void updateFace(halfedge *h, face *f) {
    halfedge *stop = h;
    do {
        h->left = f;
        h = h->next;
    } while (h != stop);
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

void halfedgeDestroyMesh(mesh *m) {
    face *itFace;
    vertex *itVertex;
    itFace = m->firstFace;
    while (itFace) {
        halfedge *itHandle = itFace->handle;
        do {
            halfedge *delHandle = itHandle;
            itHandle = itHandle->next;
            deleteHalfedge(m, delHandle);
        } while (itHandle != itFace->handle);
        face *delFace = itFace;
        itFace = itFace->next;
        deleteFace(m, delFace);
    }
    itVertex = m->firstVertex;
    while (itVertex) {
        vertex *delVertex = itVertex;
        itVertex = itVertex->next;
        deleteVertex(m, delVertex);
    }
    free(m);
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

int halfedgeSaveEdgeLoopAsObj(mesh *m, halfedge *h, const char *filename) {
    halfedge *it = h;
    FILE *file;
    int nextVertexIndex = 1;
    int i = 0;
    file = fopen(filename, "wb");
    if (!file) {
        fprintf(stderr, "Open file \"%s\" failed.\n", filename);
        return -1;
    }
    do {
        it->start->index = nextVertexIndex++;
        fprintf(file, "v %f %f %f\n", it->start->position.x, it->start->position.z, it->start->position.y);
        it = halfedgeEdgeLoopNext(m, it);
    } while (it != h);
    fprintf(file, "f");
    for (i = 1; i < nextVertexIndex; ++i) {
        fprintf(file, " %d", i);
    }
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

halfedge *halfedgeCreateFaceAddPoint(createFaceContext *ctx, point3d *point) {
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

face *halfedgeCreateFaceFromPoints(mesh *m, point3d *counterClockWisedPoints, int count) {
    int i;
    createFaceContext *ctx;
    ctx = halfedgeCreateFaceBegin(m);
    for (i = 0; i < count; i++) {
        halfedgeCreateFaceAddPoint(ctx, &counterClockWisedPoints[i]);
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
        it->start = newVertex(m, &it->start->position);
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
        halfedgeCreateFaceAddPoint(ctx, &it->start->position);
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

face *halfedgeChamferVertex(mesh *m, vertex *v, float amount) {
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
        vector3dLerp(&v->position, &current->opposite->start->position, amount, &segPoint);
        lastVertex = newVertex(m, &segPoint);
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

face *halfedgeChamferEdge(mesh *m, halfedge *h, float amount) {
    vertex *rightVertex = h->start;
    vertex *leftVertex = h->opposite->start;

    face *leftFace = halfedgeChamferVertex(m, leftVertex, amount);
    face *rightFace = halfedgeChamferVertex(m, rightVertex, amount);

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
        if (!selectedHalfedge || selectedDegree < selectedDegree) {
            selectedDegree = degree;
            selectedHalfedge = it->next;
        }
        it = it->next->opposite;
    } while (it && it != h);
    return selectedHalfedge;
}

face *halfedgeFillFaceAlongOtherSideBorder(mesh *m, halfedge *h) {
    halfedge *newHandle = 0;
    face *f = newFace(m);
    halfedge *it = h;
    halfedge *lastHandle = 0;
    halfedge *firstHandle = 0;
    vertex *firstVertex = 0;
    do {
        newHandle = newHalfedge();
        pair(newHandle, it);
        if (lastHandle) {
            link(newHandle, lastHandle);
            lastHandle->start = it->start;
        } else {
            firstHandle = newHandle;
            firstVertex = it->start;
        }
        newHandle->left = f;
        lastHandle = newHandle;
        it = it->next->opposite->next;
    } while (it != h);
    link(firstHandle, lastHandle);
    lastHandle->start = firstVertex;
    f->handle = firstHandle;
    return f;
}

mesh *halfedgeSplitMeshBySide(mesh *m) {
    mesh *frontMesh = halfedgeCreateMesh();
    face *it;
    vertex *itVertex;
    for (it = m->firstFace; it; ) {
        if (it->handle->start->front) {
            face *f = it;
            it = it->next;
            removeFace(m, f);
            addFace(frontMesh, f);
        } else {
            it = it->next;
        }
    }
    for (itVertex = m->firstVertex; itVertex; ) {
        if (itVertex->front) {
            vertex *v = itVertex;
            itVertex = itVertex->next;
            removeVertex(m, v);
            addVertex(frontMesh, v);
        } else {
            itVertex = itVertex->next;
        }
    }
    return frontMesh;
}

typedef struct splitEdgeKey {
    vertex *lo;
    vertex *hi;
} splitEdgeKey;

typedef struct splitEdge {
    splitEdgeKey key;
    rbtreeNode node;
    halfedge *ha;
    halfedge *hb;
    vertex *va;
    vertex *vb;
    struct splitEdge *next;
} splitEdge;

static int splitEdgeComparator(rbtree *t, const void *firstKey, const void *secondKey) {
    return memcmp(firstKey, secondKey, sizeof(splitEdgeKey));
}

#define makeSplitEdgeKey(k, h) do {         \
    (k)->lo = (h)->start;                   \
    (k)->hi = (h)->next->start;             \
    if ((k)->lo > (k)->hi) {                \
        swap(vertex *, (k)->lo, (k)->hi);   \
    }                                       \
} while (0)

typedef struct sliceIntersection {
    point3d cross[2];
    halfedge *handles[2];
} sliceIntersection;

mesh *halfedgeSliceMeshByFace(mesh *m, point3d *facePoint, vector3d *faceNormal) {
    face *it = 0;
    rbtree t;
    int i;
    splitEdge *seLink = 0;
    halfedge *side0AnyNewEdge = 0;
    halfedge *side1AnyNewEdge = 0;
    array *siArray = arrayCreate(sizeof(sliceIntersection));
    rbtreeInit(&t, splitEdge, node, key, splitEdgeComparator);
    for (it = m->firstFace; it; it = it->next) {
        halfedge *h = it->handle;
        do {
            h->start->front = (GEOMETRY_FRONT ==
                geometryPointPlaneLocation(&h->start->position, facePoint, faceNormal));
            h = h->next;
        } while (h != it->handle);
    }
    for (it = m->firstFace; it; it = it->next) {
        halfedge *h = it->handle;
        point3d cross[3];
        halfedge *handles[3];
        int count = 0;
        do {
            if (h->start->front != h->next->start->front) {
                int geo = geometryPointPlaneIntersection(&h->start->position,
                    &h->next->start->position, facePoint, faceNormal, &cross[count]);
                if (GEOMETRY_INTERSECT == geo) {
                    handles[count] = h;
                    count++;
                    if (3 == count) {
                        break;
                    }
                } else {
                    arrayDestroy(siArray);
                    return halfedgeCreateMesh();
                }
            }
            h = h->next;
        } while (h != it->handle);
        if (count > 0) {
            if (2 == count) {
                sliceIntersection *newSi = (sliceIntersection *)arrayNewItemClear(siArray);
                newSi->cross[0] = cross[0];
                newSi->cross[1] = cross[1];
                newSi->handles[0] = handles[0];
                newSi->handles[1] = handles[1];
            } else {
                arrayDestroy(siArray);
                return halfedgeCreateMesh();
            }
        }
    }
    for (i = 0; i < arrayGetLength(siArray); i++) {
        vertex *v0a;
        vertex *v0b;
        vertex *v1a;
        vertex *v1b;
        face *f;
        halfedge *h0;
        halfedge *h1;
        halfedge *h0b;
        halfedge *h1b;
        splitEdgeKey key0;
        splitEdge *se0 = 0;
        splitEdgeKey key1;
        splitEdge *se1 = 0;
        sliceIntersection *inter = (sliceIntersection *)arrayGetItem(siArray, i);
        
        makeSplitEdgeKey(&key0, inter->handles[0]);
        se0 = rbtreeFind(&t, &key0);
        if (se0) {
            v0a = se0->vb;
            v0b = se0->va;
        } else {
            v0a = newVertex(m, &inter->cross[0]);
            v0b = newVertex(m, &inter->cross[0]);
        }
        
        makeSplitEdgeKey(&key1, inter->handles[1]);
        se1 = rbtreeFind(&t, &key1);
        if (se1) {
            v1a = se1->vb;
            v1b = se1->va;
        } else {
            v1a = newVertex(m, &inter->cross[1]);
            v1b = newVertex(m, &inter->cross[1]);
        }

        f = newFace(m);
        v0a->front = inter->handles[0]->start->front;
        v0b->front = !v0a->front;
        v1a->front = inter->handles[1]->start->front;
        v1b->front = !v1a->front;
        h0 = newHalfedge();
        side0AnyNewEdge = h0;
        h0->start = v0a;
        v0a->handle = h0;
        h0->left = inter->handles[0]->left;
        h1 = newHalfedge();
        side1AnyNewEdge = h1;
        h1->start = v1a;
        v1a->handle = h1;
        h0b = newHalfedge();
        h0b->start = v0b;
        v0b->handle = h0b;
        h1b = newHalfedge();
        h1b->left = inter->handles[0]->left;
        h1b->start = v1b;
        v1b->handle = h1b;
        link(h0b, inter->handles[0]->next);
        link(h1b, inter->handles[1]->next);
        link(inter->handles[0], h0);
        link(h0, h1b);
        link(inter->handles[1], h1);
        link(h1, h0b);
        f->handle = h1;
        updateFace(h1, f);

        if (se0) {
            pair(inter->handles[0], se0->hb);
            pair(h0b, se0->ha);
        } else {
            se0 = (splitEdge *)calloc(1, sizeof(splitEdge));
            se0->key = key0;
            se0->ha = inter->handles[0];
            se0->hb = h0b;
            se0->va = v0a;
            se0->vb = v0b;
            rbtreeInsert(&t, se0);
            se0->next = seLink;
            seLink = se0;
        }

        if (se1) {
            pair(inter->handles[1], se1->hb);
            pair(h1b, se1->ha);
        } else {
            se1 = (splitEdge *)calloc(1, sizeof(splitEdge));
            se1->key = key1;
            se1->ha = inter->handles[1];
            se1->hb = h1b;
            se1->va = v1a;
            se1->vb = v1b;
            rbtreeInsert(&t, se1);
            se1->next = seLink;
            seLink = se1;
        }
    }
    arrayDestroy(siArray);
    if (side0AnyNewEdge) {
        halfedgeFillFaceAlongOtherSideBorder(m, side0AnyNewEdge);
    }
    if (side1AnyNewEdge) {
        halfedgeFillFaceAlongOtherSideBorder(m, side1AnyNewEdge);
    }
    while (seLink) {
        splitEdge *se = seLink;
        seLink = seLink->next;
        free(se);
    }
    return halfedgeSplitMeshBySide(m);
}
