#include <stdlib.h>
#include "selector.h"
#include "array.h"
#include "math3d.h"

static int sortFace(const void *first, const void *second) {
    face *firstFace = ((face **)first)[0];
    face *secondFace = ((face **)second)[0];
    point3d firstCenter;
    point3d secondCenter;
    halfedgeFaceCenter(firstFace, &firstCenter);
    halfedgeFaceCenter(secondFace, &secondCenter);
    return compareTwoPoints(&firstCenter, &secondCenter);
}

face *selectorGetNthFace(mesh *m, int n) {
    face *itFace;
    for (itFace = m->firstFace; itFace; itFace = itFace->next) {
        if (itFace->order == n) {
            return itFace;
        }
    }
    return 0;
}

static int sortHalfedge(const void *first, const void *second) {
    halfedge *firstHandle = ((halfedge **)first)[0];
    halfedge *secondHandle = ((halfedge **)second)[0];
    return compareTwoPoints(&firstHandle->start->position, &secondHandle->start->position);
}

halfedge *selectorGetNthHalfedge(face *f, int n) {
    halfedge *h = f->handle;
    halfedge *stop = h;
    do {
        if (h->order == n) {
            return h;
        }
        h = h->next;
    } while (h != stop);
    return 0;
}

int selectorArrangeFaceHalfedgeOrders(face *f) {
    halfedge *h = f->handle;
    halfedge *stop = h;
    array *halfedgeArray = arrayCreate(sizeof(halfedge *));
    int i;
    do {
        *(halfedge **)arrayNewItem(halfedgeArray) = h;
        h = h->next;
    } while (h != stop);
    qsort(arrayGetItem(halfedgeArray, 0), arrayGetLength(halfedgeArray), arrayGetNodeSize(halfedgeArray), sortHalfedge);
    for (i = 0; i < arrayGetLength(halfedgeArray); i++) {
        halfedge *h = *(halfedge **)arrayGetItem(halfedgeArray, i);
        h->order = i;
    }
    arrayDestroy(halfedgeArray);
    return 0;
}

int selectorArrangeMesh(mesh *m) {
    face *itFace;
    array *faceArray = arrayCreate(sizeof(face *));
    int i;
    for (itFace = m->firstFace; itFace; itFace = itFace->next) {
        *(face **)arrayNewItem(faceArray) = itFace;
    }
    qsort(arrayGetItem(faceArray, 0), arrayGetLength(faceArray), arrayGetNodeSize(faceArray), sortFace);
    for (i = 0; i < arrayGetLength(faceArray); i++) {
        face *f = *(face **)arrayGetItem(faceArray, i);
        f->order = i;
        selectorArrangeFaceHalfedgeOrders(f);
    }
    arrayDestroy(faceArray);
    return 0;
}
