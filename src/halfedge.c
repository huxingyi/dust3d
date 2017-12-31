#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "halfedge.h"
#include "matrix.h"
#include "vector3d.h"
#include "math3d.h"
#include "rbtree.h"
#include "array.h"
#include "util.h"
#include "color.h"

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
    assert(firstValue->start != secondValue->start);\
} while (0)

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
    f->color = WHITE;
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

typedef struct edge {
    vertex *lo;
    vertex *hi;
} edge;

static int edgeComparator(rbtree *tree, const void *firstKey, const void *secondKey) {
    return memcmp(firstKey, secondKey, sizeof(edge));
}

#define makeEdgeFromHalfedge(e, h) do {                 \
    (e)->lo = (h)->start;                               \
    (e)->hi = (h)->next->start;                         \
    if ((e)->lo > (e)->hi) {                            \
        swap(vertex *, (e)->lo, (e)->hi);               \
    }                                                   \
} while (0)

#define makeEdgeFromVertices(e, v1, v2) do {             \
    (e)->lo = (v1);                                     \
    (e)->hi = (v2);                                     \
    if ((e)->lo > (e)->hi) {                            \
        swap(vertex *, (e)->lo, (e)->hi);               \
    }                                                   \
} while (0)

static void *findEdgeFromMap(rbtree *t, vertex *v1, vertex *v2) {
    edge key;
    makeEdgeFromVertices(&key, v1, v2);
    return rbtreeFind(t, &key);
}

mesh *halfedgeCreateMesh(void) {
    return (mesh *)calloc(1, sizeof(mesh));
}

void halfedgeResetMesh(mesh *m) {
    face *itFace;
    vertex *itVertex;
    if (!halfedgeIsEmptyMesh(m)) {
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
    }
}

void halfedgeDestroyMesh(mesh *m) {
    halfedgeResetMesh(m);
    free(m);
}

int halfedgeIsEmptyMesh(mesh *m) {
    return !m->firstVertex;
}

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

typedef struct createFaceContext {
    mesh *m;
    face *left;
    vertex *previousVertex;
    vertex *currentVertex;
    vertex *firstVertex;
    halfedge *previousHalfedge;
} createFaceContext;

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
    halfedge *handle = 0;
    if (ctx->left) {
        handle = newHalfedge();
        ctx->currentVertex = ctx->firstVertex;
        handle->next = ctx->left->handle;
        halfedgeCreateFaceAddHalfedge(ctx, handle);
        ctx->left->handle->previous = handle;
    }
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

face *halfedgeCreatePlane(mesh *m, float width, float depth) {
    float x = width / 2;
    float y = depth / 2;
    point3d points[] = {
        {-x, -y, 0}, {x, -y, 0}, {x, y, 0}, {-x, y, 0}
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
            point3d *arr[3] = {points[(num - 3) % 3], points[(num - 2) % 3], points[(num - 1) % 3]};
            if (!isPointOnSegment(arr[1], arr[0], arr[2])) {
                vector3dNormal(arr[0], arr[1], arr[2], &v);
                vector3dAdd(normal, &v);
                total++;
            }
        }
        it = it->next;
    } while (it != stop);
    vector3dDivide(normal, total);
    return 0;
}

int halfedgeFaceCenter(mesh *m, face *f, point3d *point) {
    halfedge *it = f->handle;
    halfedge *stop = it;
    int total = 0;
    point->x = point->y = point->z = 0;
    do {
        vector3dAdd(point, &it->start->position);
        total++;
        it = it->next;
    } while (it != stop);
    vector3dDivide(point, total);
    return 0;
}

int halfedgeFlipFace(mesh *m, face *f) {
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
    return 0;
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

int halfedgeTransformMesh(mesh *m, matrix *mat) {
    vertex *v;
    for (v = m->firstVertex; v; v = v->next) {
        matrixTransformXYZ(mat, &v->position.x, &v->position.y, &v->position.z);
    }
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
    *v = h->next->start->position;
    vector3dSub(v, &h->start->position);
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
    //halfedgeFixHoles(m);
    //halfedgeFixHoles(frontMesh);
    return frontMesh;
}

typedef struct splitEdge {
    edge key;
    rbtreeNode node;
    halfedge *ha;
    halfedge *hb;
    vertex *va;
    vertex *vb;
    struct splitEdge *next;
} splitEdge;

typedef struct sliceIntersection {
    point3d cross[2];
    halfedge *handles[2];
} sliceIntersection;

mesh *halfedgeSliceMeshByPlane(mesh *m, point3d *facePoint, vector3d *faceNormal, int coincidenceAsFront) {
    face *it = 0;
    rbtree t;
    int i;
    splitEdge *seLink = 0;
    array *siArray = arrayCreate(sizeof(sliceIntersection));
    rbtreeInit(&t, splitEdge, node, key, edgeComparator);
    for (it = m->firstFace; it; it = it->next) {
        halfedge *h = it->handle;
        do {
            int side = identifyPointOnWhichSideOfPlane(&h->start->position, facePoint, faceNormal);
            h->start->front = coincidenceAsFront ? (SIDE_BACK != side) : (SIDE_FRONT == side);
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
                if (0 == intersectionOfSegmentAndPlane(&h->start->position,
                    &h->next->start->position, facePoint, faceNormal, &cross[count])) {
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
        edge key0;
        splitEdge *se0 = 0;
        edge key1;
        splitEdge *se1 = 0;
        sliceIntersection *inter = (sliceIntersection *)arrayGetItem(siArray, i);
        
        makeEdgeFromHalfedge(&key0, inter->handles[0]);
        se0 = rbtreeFind(&t, &key0);
        if (se0) {
            v0a = se0->vb;
            v0b = se0->va;
        } else {
            v0a = newVertex(m, &inter->cross[0]);
            v0b = newVertex(m, &inter->cross[0]);
        }
        
        makeEdgeFromHalfedge(&key1, inter->handles[1]);
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
        h0->start = v0a;
        v0a->handle = h0;
        h0->left = inter->handles[0]->left;
        h1 = newHalfedge();
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
    freeList(splitEdge, seLink);
    return halfedgeSplitMeshBySide(m);
}

typedef struct edgeMapNode {
    rbtreeNode node;
    edge key;
    halfedge *h;
    struct edgeMapNode *next;
} edgeMapNode;

static edgeMapNode *addEdgeToMap(rbtree *t, vertex *v1, vertex *v2) {
    edgeMapNode *mapNode = (edgeMapNode *)calloc(1, sizeof(edgeMapNode));
    makeEdgeFromVertices(&mapNode->key, v1, v2);
    rbtreeInsert(t, mapNode);
    return mapNode;
}

typedef struct objPoint {
    point3d position;
    vertex *v;
} objPoint;

int halfedgeImportObj(mesh *m, const char *filename) {
    array *pointArray = 0;
    FILE *fp = 0;
    char line[512];
    int result = -1;
    rbtree edgeMap;
    edgeMapNode *edgeList = 0;
    createFaceContext *createContext = 0;
    rbtreeInit(&edgeMap, edgeMapNode, node, key, edgeComparator);
    fp = fopen(filename, "rb");
    if (!fp) {
        fprintf(stderr, "Open file \"%s\" failed.\n", filename);
        goto tail;
    }
    pointArray = arrayCreate(sizeof(objPoint));
    while (fgets(line, sizeof(line), fp)) {
        if ('v' == line[0] && ' ' == line[1]) {
            objPoint *p = (objPoint *)arrayNewItemClear(pointArray);
            if (3 != sscanf(line, "v %f %f %f", &p->position.x, &p->position.y, &p->position.z)) {
                fprintf(stderr, "Invalid line \"%s\".\n", line);
                goto tail;
            }
        } else if ('f' == line[0] && ' ' == line[1]) {
            char *space = &line[1];
            halfedge *h;
            halfedge *stop;
            createContext = halfedgeCreateFaceBegin(m);
            do {
                int index = -1;
                objPoint *p;
                sscanf(space, " %d", &index);
                if (index <= 0 || index > arrayGetLength(pointArray)) {
                    fprintf(stderr, "Invalid line \"%s\".\n", line);
                    goto tail;
                }
                p = (objPoint *)arrayGetItem(pointArray, index - 1);
                if (!p->v) {
                    p->v = newVertex(m, &p->position);
                }
                halfedgeCreateFaceAddVertex(createContext, p->v);
                space = strchr(space + 1, ' ');
            } while (space);
            h = halfedgeCreateFaceEnd(createContext);
            stop = h;
            do {
                edgeMapNode *edgeNode = (edgeMapNode *)findEdgeFromMap(&edgeMap, h->start, h->next->start);
                if (edgeNode) {
                    pair(edgeNode->h, h);
                } else {
                    edgeNode = addEdgeToMap(&edgeMap, h->start, h->next->start);
                    edgeNode->h = h;
                    addToList(edgeNode, edgeList);
                }
                h = h->next;
            } while (h != stop);
            createContext = 0;
        }
    }
    result = 0;
tail:
    if (fp) {
        fclose(fp);
    }
    if (pointArray) {
        arrayDestroy(pointArray);
    }
    if (createContext) {
        halfedgeCreateFaceEnd(createContext);
    }
    freeList(edgeMapNode, edgeList);
    return result;
}

typedef struct vertexCloneMapNode {
    rbtreeNode node;
    vertex *key;
    vertex *v;
    struct vertexCloneMapNode *next;
} vertexCloneMapNode;

int halfedgeConcatMesh(mesh *m, mesh *src) {
    face *srcFace;
    rbtree edgeMap;
    rbtree vertexCloneMap;
    edgeMapNode *edgeList = 0;
    createFaceContext *createContext = 0;
    vertexCloneMapNode *vertexCloneList = 0;
    rbtreeInit(&edgeMap, edgeMapNode, node, key, edgeComparator);
    rbtreeInit(&vertexCloneMap, vertexCloneMapNode, node, key, rbtreePointerComparator);
    for (srcFace = src->firstFace; srcFace; srcFace = srcFace->next) {
        halfedge *h;
        halfedge *stop;
        halfedge *hSrc = srcFace->handle;
        halfedge *stopSrc = hSrc;
        createContext = halfedgeCreateFaceBegin(m);
        do {
            vertexCloneMapNode *vc = (vertexCloneMapNode *)rbtreeFind(&vertexCloneMap, &hSrc->start);
            if (!vc) {
                vc = (vertexCloneMapNode *)calloc(1, sizeof(vertexCloneMapNode));
                vc->key = hSrc->start;
                vc->v = newVertex(m, &vc->key->position);
                addToList(vc, vertexCloneList);
                rbtreeInsert(&vertexCloneMap, vc);
            }
            halfedgeCreateFaceAddVertex(createContext, vc->v);
            hSrc = hSrc->next;
        } while (hSrc != stopSrc);
        h = halfedgeCreateFaceEnd(createContext);
        stop = h;
        do {
            edgeMapNode *edgeNode = (edgeMapNode *)findEdgeFromMap(&edgeMap, h->start, h->next->start);
            if (edgeNode) {
                pair(edgeNode->h, h);
            } else {
                edgeNode = addEdgeToMap(&edgeMap, h->start, h->next->start);
                edgeNode->h = h;
                addToList(edgeNode, edgeList);
            }
            h = h->next;
        } while (h != stop);
    }
    freeList(vertexCloneMapNode, vertexCloneList);
    freeList(edgeMapNode, edgeList);
    return 0;
}

mesh *halfedgeCloneMesh(mesh *m) {
    mesh *clone = halfedgeCreateMesh();
    halfedgeConcatMesh(clone, m);
    return clone;
}

typedef struct holeVertex {
    rbtreeNode node;
    vertex *cone;
    struct holeVertex *edge0;
    struct holeVertex *edge1;
    int processed;
    struct holeVertex *next;
} holeVertex;

int halfedgeFixHoles(mesh *m) {
    rbtree holeVertexMap;
    holeVertex *holeVertexLink = 0;
    holeVertex *itHv;
    rbtree edgeMap;
    edgeMapNode *edgeList = 0;
    createFaceContext *createContext = 0;
    face *itFace;
    int holeVertices = 0;
    rbtreeInit(&edgeMap, edgeMapNode, node, key, edgeComparator);
    rbtreeInit(&holeVertexMap, holeVertex, node, cone, rbtreePointerComparator);
    for (itFace = m->firstFace; itFace; itFace = itFace->next) {
        halfedge *h = itFace->handle;
        do {
            edgeMapNode *edgeNode;
            if (!h->opposite) {
                holeVertex *hv;
                holeVertex *hv0;
                assert(h->start != h->next->start);
                assert(0 == findEdgeFromMap(&edgeMap, h->start, h->next->start));
                edgeNode = addEdgeToMap(&edgeMap, h->start, h->next->start);
                edgeNode->h = h;
                addToList(edgeNode, edgeList);
                hv = rbtreeFind(&holeVertexMap, &h->start);
                if (!hv) {
                    hv = (holeVertex *)calloc(1, sizeof(holeVertex));
                    hv->cone = h->start;
                    addToList(hv, holeVertexLink);
                    rbtreeInsert(&holeVertexMap, hv);
                }
                hv0 = rbtreeFind(&holeVertexMap, &h->next->start);
                if (!hv0) {
                    hv0 = (holeVertex *)calloc(1, sizeof(holeVertex));
                    hv0->cone = h->next->start;
                    addToList(hv0, holeVertexLink);
                    rbtreeInsert(&holeVertexMap, hv0);
                }
                assert(hv != hv0);
                hv->edge0 = hv0;
                hv0->edge1 = hv;
                holeVertices++;
            }
            h = h->next;
        } while (h != itFace->handle);
    }
    for (itHv = holeVertexLink; itHv; itHv = itHv->next) {
        halfedge *h;
        halfedge *stop;
        holeVertex *subItHv;
        int vertices = 0;
        if (itHv->processed) {
            continue;
        }
        subItHv = itHv;
        do {
            if (!subItHv) {
                break;
            }
            if (subItHv->processed) {
                break;
            }
            if (!subItHv->edge0) {
                break;
            }
            if (subItHv->edge0->edge1 != subItHv) {
                break;
            }
            vertices++;
            subItHv->processed = 1;
            subItHv = subItHv->edge1;
        } while (subItHv != itHv);
        if (subItHv != itHv || vertices < 3) {
            continue;
        }
        createContext = halfedgeCreateFaceBegin(m);
        subItHv = itHv;
        do {
            halfedgeCreateFaceAddVertex(createContext, subItHv->cone);
            subItHv->processed = 1;
            subItHv = subItHv->edge1;
        } while (subItHv != itHv);
        h = halfedgeCreateFaceEnd(createContext);
        stop = h;
        do {
            edgeMapNode *edgeNode = (edgeMapNode *)findEdgeFromMap(&edgeMap, h->start, h->next->start);
            if (edgeNode) {
                pair(edgeNode->h, h);
            } else {
                edgeNode = addEdgeToMap(&edgeMap, h->start, h->next->start);
                edgeNode->h = h;
                addToList(edgeNode, edgeList);
            }
            h = h->next;
        } while (h != stop);
    }
    freeList(holeVertex, holeVertexLink);
    freeList(edgeMapNode, edgeList);
    return 0;
}

int halfedgeFixPairing(mesh *m) {
    rbtree edgeMap;
    edgeMapNode *edgeList = 0;
    face *itFace;
    rbtreeInit(&edgeMap, edgeMapNode, node, key, edgeComparator);
    for (itFace = m->firstFace; itFace; itFace = itFace->next) {
        halfedge *h = itFace->handle;
        halfedge *stop = h;
        do {
            edgeMapNode *edgeNode = (edgeMapNode *)findEdgeFromMap(&edgeMap, h->start, h->next->start);
            if (edgeNode) {
                pair(edgeNode->h, h);
            } else {
                edgeNode = addEdgeToMap(&edgeMap, h->start, h->next->start);
                edgeNode->h = h;
                addToList(edgeNode, edgeList);
            }
            //assert(h->start != h->next->start);
            h = h->next;
        } while (h != stop);
    }
    freeList(edgeMapNode, edgeList);
    return 0;
}

int halfedgeIsWatertight(mesh *m) {
    face *itFace;
    for (itFace = m->firstFace; itFace; itFace = itFace->next) {
        halfedge *h = itFace->handle;
        halfedge *stop = h;
        do {
            if (!h->opposite) {
                return 0;
            }
            h = h->next;
        } while (h != stop);
    }
    return 1;
}

int halfedgeJoinMesh(mesh *m, mesh *subm) {
    face *it;
    vertex *itVertex;
    for (it = subm->firstFace; it; ) {
        face *f = it;
        it = it->next;
        removeFace(subm, f);
        addFace(m, f);
    }
    for (itVertex = subm->firstVertex; itVertex; ) {
        vertex *v = itVertex;
        itVertex = itVertex->next;
        removeVertex(subm, v);
        addVertex(m, v);
    }
    halfedgeDestroyMesh(subm);
    return 0;
}

/*
int halfedgeIntersectMesh(mesh *m, mesh *by) {
    face *sliceFace;
    for (sliceFace = by->firstFace; sliceFace; sliceFace = sliceFace->next) {
        vector3d normal;
        mesh *otherSideMesh;
        halfedgeFaceNormal(by, sliceFace, &normal);
        otherSideMesh = halfedgeSliceMeshByPlane(m, &sliceFace->handle->start->position, &normal, 0);
        halfedgeDestroyMesh(otherSideMesh);
    }
    return 0;
}*/

int halfedgeFlipMesh(mesh *m) {
    face *itFace;
    for (itFace = m->firstFace; itFace; itFace = itFace->next) {
        halfedgeFlipFace(m, itFace);
    }
    return 0;
}

mesh *halfedgeSplitMeshByMesh(mesh *m, mesh *by) {
    face *sliceFace;
    mesh *outside = halfedgeCreateMesh();
    for (sliceFace = by->firstFace; sliceFace; sliceFace = sliceFace->next) {
        vector3d normal;
        mesh *frontMesh;
        halfedgeFaceNormal(by, sliceFace, &normal);
        frontMesh = halfedgeSliceMeshByPlane(m, &sliceFace->handle->start->position, &normal, 0);
        halfedgeJoinMesh(outside, frontMesh);
    }
    return outside;
}

typedef struct vertexMapNode {
    rbtreeNode node;
    vertex *v;
    struct vertexMapNode *next;
} vertexMapNode;

static int vertexComparator(rbtree *tree, const void *firstKey, const void *secondKey) {
    vertex *firstV = ((vertex **)firstKey)[0];
    vertex *secondV = ((vertex **)secondKey)[0];
    float offsetX = firstV->position.x - secondV->position.x;
    float offsetY = firstV->position.y - secondV->position.y;
    float offsetZ = firstV->position.z - secondV->position.z;
    if (fabs(offsetX) > EPSILON_DISTANCE) {
        if (offsetX < 0) {
            return -1;
        } else {
            return 1;
        }
    }
    if (fabs(offsetY) > EPSILON_DISTANCE) {
        if (offsetY < 0) {
            return -1;
        } else {
            return 1;
        }
    }
    if (fabs(offsetZ) > EPSILON_DISTANCE) {
        if (offsetZ < 0) {
            return -1;
        } else {
            return 1;
        }
    }
    return 0;
}

int halfedgeCombineDuplicateVertices(mesh *m) {
    rbtree vertexMap;
    face *itFace;
    vertexMapNode *vertexList = 0;
    rbtreeInit(&vertexMap, vertexMapNode, node, v, vertexComparator);
    for (itFace = m->firstFace; itFace; itFace = itFace->next) {
        halfedge *h = itFace->handle;
        halfedge *stop = h;
        do {
            vertexMapNode *node = rbtreeFind(&vertexMap, &h->start);
            if (!node) {
                node = (vertexMapNode *)calloc(1, sizeof(vertexMapNode));
                node->v = h->start;
                addToList(node, vertexList);
                rbtreeInsert(&vertexMap, node);
            } else if (node->v != h->start) {
                removeVertex(m, h->start);
                h->start = node->v;
            }
            h = h->next;
        } while (h != stop);
    }
    freeList(vertexMapNode, vertexList);
    return 0;
}

static void splitHalfedge(mesh *m, halfedge *h, vertex *v) {
    halfedge *newHandle = newHalfedge();
    newHandle->start = v;
    newHandle->left = h->left;
#if DUST3D_DEBUG
    //h->left->color = CHOCOLATE;
    //h->color = LAWN_GREEN;
    //newHandle->color = BROWN;
#endif
    /*
    printf("<%f,%f,%f> ---- <%f,%f,%f> --- <%f,%f,%f>\n",
        h->start->position.x,
        h->start->position.y,
        h->start->position.z,
        v->position.x,
        v->position.y,
        v->position.z,
        h->next->start->position.x,
        h->next->start->position.y,
        h->next->start->position.z);*/
    link(newHandle, h->next);
    link(h, newHandle);
}

typedef struct halfedgeNode {
    halfedge *h;
    struct halfedgeNode *next;
} halfedgeNode;

int halfedgeFixTjunction(mesh *m) {
    face *itFace;
    halfedgeNode *boundaryList = 0;
    halfedgeNode *itNode;
    halfedgeNode *node;
    for (itFace = m->firstFace; itFace; itFace = itFace->next) {
        halfedge *h = itFace->handle;
        halfedge *stop = h;
        do {
            if (!h->opposite) {
                node = (halfedgeNode *)calloc(1, sizeof(halfedgeNode));
                node->h = h;
                addToList(node, boundaryList);
            }
            h = h->next;
        } while (h != stop);
    }
    for (itNode = boundaryList; itNode;) {
        halfedgeNode *subItNode;
        int splitted = 0;
        for (subItNode = boundaryList; subItNode; subItNode = subItNode->next) {
            if (subItNode == itNode) {
                continue;
            }
            if (isPointOnSegment(&subItNode->h->start->position, &itNode->h->start->position, &itNode->h->next->start->position)) {
                //subItNode->h->color = DARK_SLATE_GRAY;
                splitHalfedge(m, itNode->h, subItNode->h->start);
                splitted = 1;
                break;
            }
            if (isPointOnSegment(&subItNode->h->next->start->position, &itNode->h->start->position, &itNode->h->next->start->position)) {
                //subItNode->h->next->color = DARK_SLATE_GRAY;
                splitHalfedge(m, itNode->h, subItNode->h->next->start);
                splitted = 1;
                break;
            }
        }
        if (!splitted) {
            itNode = itNode->next;
            continue;
        }
        node = (halfedgeNode *)calloc(1, sizeof(halfedgeNode));
        node->h = itNode->h->next;
        node->next = itNode->next;
        itNode->next = node;
    }
    freeList(halfedgeNode, boundaryList);
    return 0;
}

int halfedgeFixAll(mesh *m) {
    halfedgeCombineDuplicateVertices(m);
    halfedgeFixTjunction(m);
    halfedgeFixPairing(m);
    //halfedgeFixHoles(m);
    return 0;
}

int halfedgeUnionMesh(mesh *m, mesh *by) {
    mesh *cloneBy = halfedgeCloneMesh(by);
    mesh *otherOutside = halfedgeSplitMeshByMesh(cloneBy, m);
    mesh *myOutside = halfedgeSplitMeshByMesh(m, by);
    halfedgeDestroyMesh(cloneBy);
    halfedgeResetMesh(m);
    halfedgeJoinMesh(m, otherOutside);
    halfedgeJoinMesh(m, myOutside);
    halfedgeFixAll(m);
    return 0;
}

int halfedgeDiffMesh(mesh *m, mesh *by) {
    mesh *cloneBy = halfedgeCloneMesh(by);
    mesh *otherOutside = halfedgeSplitMeshByMesh(cloneBy, m);
    mesh *myOutside = halfedgeSplitMeshByMesh(m, by);
    mesh *otherInside = cloneBy;
    halfedgeDestroyMesh(otherOutside);
    halfedgeFlipMesh(otherInside);
    halfedgeResetMesh(m);
    halfedgeJoinMesh(m, myOutside);
    halfedgeJoinMesh(m, otherInside);
    halfedgeFixAll(m);
    return 0;
}

int halfedgeIntersectMesh(mesh *m, mesh *by) {
    mesh *cloneM = halfedgeCloneMesh(m);
    mesh *cloneBy = halfedgeCloneMesh(by);
    mesh *otherOutside = halfedgeSplitMeshByMesh(cloneBy, m);
    mesh *myOutside = halfedgeSplitMeshByMesh(cloneM, by);
    mesh *otherInside = cloneBy;
    mesh *myInside = cloneM;
    halfedgeDestroyMesh(otherOutside);
    halfedgeDestroyMesh(myOutside);
    halfedgeResetMesh(m);
    halfedgeJoinMesh(m, otherInside);
    halfedgeJoinMesh(m, myInside);
    halfedgeFixAll(m);
    return 0;
}
