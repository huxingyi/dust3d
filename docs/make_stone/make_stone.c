#include <stdio.h>
#include "halfedge.h"

int main(int argc, char *argv[]) {
    mesh *m = halfedgeCreateMesh();
    printf("begin\n");
    halfedgeExtrudeFace(m, halfedgeCreatePlane(m, 1), 2);
    halfedgeChamferVertex(m, m->firstVertex, 0.2, 0.8, 0.5);
    printf("saving\n");
    halfedgeSaveAsObj(m, "stone.obj");
    printf("end\n");
    return 0;
}
