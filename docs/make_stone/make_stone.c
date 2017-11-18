#include <stdio.h>
#include "halfedge.h"

int main(int argc, char *argv[]) {
    mesh *m = halfedgeCreateMesh();
    face *f;
    printf("begin\n");
    halfedgeExtrudeFace(m, halfedgeCreatePlane(m, 1), 2);
    f = halfedgeChamferVertex(m, m->lastVertex, 0.2);
    //f = halfedgeChamferEdge(m, f->handle, 0.3);
    f = halfedgeChamferEdge(m, m->firstFace->handle, 0.3);
    //halfedgeSaveFaceAsObj(m, f, "face.obj");
    //halfedgeSaveFaceAsObj(m, f->handle->opposite->left, "face2.obj");
    //halfedgeSaveFaceAsObj(m, f->handle->opposite->previous->opposite->left, "face3.obj");
    //halfedgeSaveFaceAsObj(m, f->handle->opposite->next->opposite->left, "face4.obj");
    f = halfedgeChamferEdge(m, f->handle, 0.3);
    //f = halfedgeChamferVertex(m, m->lastVertex, 0.2);
    printf("saving\n");
    halfedgeSaveMeshAsObj(m, "stone.obj");
    printf("end\n");
    return 0;
}
