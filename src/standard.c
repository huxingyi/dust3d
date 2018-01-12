#include "standard.h"

mesh *standardCreateBoxMesh(standardBox *box) {
    mesh *m = halfedgeCreateMesh();
    face *f = halfedgeCreatePlane(m, box->width, box->length);
    halfedgeExtrudeFace(m, f, box->height);
    return m;
}
