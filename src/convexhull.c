#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "convexhull.h"
#include "math3d.h"
#include "util.h"

/*
Implementation of Gift wrapping method which describled in http://dccg.upc.edu/people/vera/wp-content/uploads/2014/11/GA2014-convexhulls3D-Roger-Hernando.pdf
*/

typedef struct convexhullItemKey {
    int p1;
    int p2;
} convexhullItemKey;

typedef struct convexhullItem {
    convexhullItemKey key;
    rbtreeNode node;
    vector3d baseNormal;
    int p3;
    int processed:1;
    struct convexhullItem *next;
} convexhullItem;

typedef struct convexhullFace3 {
    int p1;
    int p2;
    int p3;
} convexhullFace3;

typedef struct convexhullVertex {
    point3d position;
    void *sourcePlane;
    void *tag;
    int index;
    struct convexhullVertex *previous;
    struct convexhullVertex *next;
} convexhullVertex;

struct convexhull {
    array *vertexArray;
    array *generatedFace3Array;
    convexhullItem *itemList;
    convexhullVertex *firstWaitingVertex;
    convexhullVertex *lastWaitingVertex;
    rbtree itemMap;
};

int convexhullGetGeneratedFaceCount(convexhull *hull) {
    return arrayGetLength(hull->generatedFace3Array);
}

int convexhullGetNthGeneratedFace(convexhull *hull, int index, int *p1, int *p2, int *p3) {
    convexhullFace3 *face3 = (convexhullFace3 *)arrayGetItem(hull->generatedFace3Array, index);
    *p1 = face3->p1;
    *p2 = face3->p2;
    *p3 = face3->p3;
    return 0;
}

int convexhullGetVertexInfo(convexhull *hull, int p, point3d *point, void **tag) {
    convexhullVertex *vertex = (convexhullVertex *)arrayGetItem(hull->vertexArray, p);
    *point = vertex->position;
    *tag = vertex->tag;
    return 0;
}

static int convexhullItemIndexComparator(rbtree *tree, const void *first, const void *second) {
    return memcmp(first, second, sizeof(convexhullItemKey));
}

int convexhullAddPoint(convexhull *hull, point3d *point, void *sourcePlane, void *tag) {
    convexhullVertex *vertex = (convexhullVertex *)arrayNewItemClear(hull->vertexArray);
    vertex->position = *point;
    vertex->sourcePlane = sourcePlane;
    vertex->tag = tag;
    vertex->index = arrayGetLength(hull->vertexArray) - 1;
    return vertex->index;
}

int convexhullUpdatePointSourcePlane(convexhull *hull, int index, void *sourcePlane) {
    convexhullVertex *vertex = (convexhullVertex *)arrayGetItem(hull->vertexArray, index);
    vertex->sourcePlane = sourcePlane;
    return 0;
}

void *convexhullGetPointSourcePlane(convexhull *hull, int index) {
    convexhullVertex *vertex = (convexhullVertex *)arrayGetItem(hull->vertexArray, index);
    return vertex->sourcePlane;
}

static void linkAddedPoints(convexhull *hull) {
    int i;
    int total = arrayGetLength(hull->vertexArray);
    hull->firstWaitingVertex = hull->lastWaitingVertex = 0;
    for (i = 0; i < total; i++) {
        convexhullVertex *vertex = (convexhullVertex *)arrayGetItem(hull->vertexArray, i);
        addToDoubleLinkedListTail(vertex, hull->firstWaitingVertex, hull->lastWaitingVertex);
    }
}

convexhull *convexhullCreate(void) {
    convexhull *hull = (convexhull *)calloc(1, sizeof(convexhull));
    hull->vertexArray = arrayCreate(sizeof(convexhullVertex));
    hull->generatedFace3Array = arrayCreate(sizeof(convexhullFace3));
    rbtreeInit(&hull->itemMap, convexhullItem, node, key, convexhullItemIndexComparator);
    return hull;
}

void convexhullDestroy(convexhull *hull) {
    arrayDestroy(hull->vertexArray);
    arrayDestroy(hull->generatedFace3Array);
    freeList(convexhullItem, hull->itemList);
    free(hull);
}

static convexhullItem *findItem(convexhull *hull, int p1, int p2) {
    convexhullItemKey key = {p1, p2};
    return (convexhullItem *)rbtreeFind(&hull->itemMap, &key);
}

static convexhullItem *addItem(convexhull *hull, int p1, int p2, vector3d *baseNormal) {
    convexhullItem *item;
    convexhullVertex *v1 = (convexhullVertex *)arrayGetItem(hull->vertexArray, p1);
    convexhullVertex *v2 = (convexhullVertex *)arrayGetItem(hull->vertexArray, p2);
    if ((hull->itemList && v1->sourcePlane == v2->sourcePlane) || findItem(hull, p1, p2) || findItem(hull, p2, p1)) {
        return 0;
    }
    item = (convexhullItem *)calloc(1, sizeof(convexhullItem));
    item->p3 = -1;
    item->baseNormal = *baseNormal;
    item->key.p1 = p1;
    item->key.p2 = p2;
    rbtreeInsert(&hull->itemMap, item);
    addToList(item, hull->itemList);
    return item;
}

static void calculateNormal(convexhull *hull, int p1, int p2, int p3, vector3d *normal) {
    convexhullVertex *v1 = (convexhullVertex *)arrayGetItem(hull->vertexArray, p1);
    convexhullVertex *v2 = (convexhullVertex *)arrayGetItem(hull->vertexArray, p2);
    convexhullVertex *v3 = (convexhullVertex *)arrayGetItem(hull->vertexArray, p3);
    vector3dNormal(&v1->position, &v2->position, &v3->position, normal);
}

int convexhullAddStartup(convexhull *hull, int p1, int p2, vector3d *baseNormal) {
    addItem(hull, p1, p2, baseNormal);
    return 0;
}

static void calculateFaceVector(convexhull *hull, int p1, int p2, vector3d *baseNormal, vector3d *result) {
    convexhullVertex *v1 = (convexhullVertex *)arrayGetItem(hull->vertexArray, p1);
    convexhullVertex *v2 = (convexhullVertex *)arrayGetItem(hull->vertexArray, p2);
    vector3d seg;
    vector3dSegment(&v1->position, &v2->position, &seg);
    vector3dCrossProduct(&seg, baseNormal, result);
}

static float angleOfBaseFaceAndPoint(convexhull *hull, convexhullItem *baseItem, int p) {
    convexhullVertex *v1 = (convexhullVertex *)arrayGetItem(hull->vertexArray, baseItem->key.p1);
    convexhullVertex *v2 = (convexhullVertex *)arrayGetItem(hull->vertexArray, baseItem->key.p2);
    convexhullVertex *vp = (convexhullVertex *)arrayGetItem(hull->vertexArray, p);
    vector3d vd1, vd2, normal;
    if (v1->sourcePlane == v2->sourcePlane && v1->sourcePlane == vp->sourcePlane) {
        return 0;
    }
    if (findItem(hull, baseItem->key.p2, p) ||
        findItem(hull, p, baseItem->key.p2) ||
        findItem(hull, p, baseItem->key.p1) ||
        findItem(hull, baseItem->key.p1, p)) {
        return 0;
    }
    calculateFaceVector(hull, baseItem->key.p2, baseItem->key.p1, &baseItem->baseNormal, &vd1);
    vector3dNormal(&v2->position, &v1->position, &vp->position, &normal);
    calculateFaceVector(hull, baseItem->key.p2, baseItem->key.p1, &normal, &vd2);
    return math3dAngleOfTwoVectors(&vd1, &vd2);
}

static int findBestVertexOnTheLeft(convexhull *hull, convexhullItem *baseItem) {
    float maxAngle = 0;
    float angle = 0;
    convexhullVertex *it;
    int result = -1;
    convexhullVertex *choosen = 0;
    for (it = hull->firstWaitingVertex; it; it = it->next) {
        angle = angleOfBaseFaceAndPoint(hull, baseItem, it->index);
        if (angle > maxAngle) {
            maxAngle = angle;
            choosen = it;
            result = it->index;
        }
    }
    if (choosen) {
        removeFromDoubleLinkedList(choosen, hull->firstWaitingVertex, hull->lastWaitingVertex);
    }
    return result;
}

static convexhullItem *pickItem(convexhull *hull) {
    convexhullItem *item;
    for (item = hull->itemList; item; item = item->next) {
        if (item->processed) {
            continue;
        }
        return item;
    }
    return 0;
}

int convexhullGenerate(convexhull *hull) {
    convexhullItem *item;
    int p3;
    convexhullFace3 *face3;
    vector3d baseNormal;
    linkAddedPoints(hull);
    int faceNum = 0;
    while ((item=pickItem(hull))) {
        item->processed = 1;
        p3 = findBestVertexOnTheLeft(hull, item);
        if (-1 == p3) {
            continue;
        }
        item->p3 = p3;
        calculateNormal(hull, item->key.p1, item->key.p2, p3, &baseNormal);
        face3 = (convexhullFace3 *)arrayNewItemClear(hull->generatedFace3Array);
        face3->p1 = item->key.p1;
        face3->p2 = item->key.p2;
        face3->p3 = p3;
        addItem(hull, p3, item->key.p2, &baseNormal);
        addItem(hull, item->key.p1, p3, &baseNormal);
        faceNum++;
    }
    return 0;
}
