#include <stdio.h>
#include "halfedge.h"

int main(int argc, char *argv[]) {
    mesh *m = halfedgeCreateMesh();
    face *f;
    printf("begin\n");
    halfedgeExtrudeFace(m, halfedgeCreatePlane(m, 1), 2);
    halfedgeChamferVertex(m, m->lastVertex, 0.2);
    f = halfedgeChamferEdge(m, m->firstFace->handle, 0.3);
    printf("saving\n");
    halfedgeSaveMeshAsObj(m, "stone.obj");
    printf("end\n");
    return 0;
}
