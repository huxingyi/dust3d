#include "selector.h"

face *selectorGetNthFace(mesh *m, int n) {
    face *itFace;
    for (itFace = m->firstFace; itFace; itFace = itFace->next, --n) {
        if (n <= 0) {
            return itFace;
        }
    }
    return 0;
}

halfedge *selectorGetNthHalfedge(face *f, int n) {
    halfedge *h = f->handle;
    halfedge *stop = h;
    do {
        if (n <= 0) {
            return h;
        }
        h = h->next;
        --n;
    } while (h != stop);
    return 0;
}
