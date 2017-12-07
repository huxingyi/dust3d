#include <stdio.h>
#include "halfedge.h"

int main(int argc, char *argv[]) {
    mesh *m = halfedgeCreateMesh();
    face *f;
    printf("begin\n");
    halfedgeExtrudeFace(m, halfedgeCreatePlane(m, 1), 2);
    //f = halfedgeChamferVertex(m, m->lastVertex, 0.2);
    //f = halfedgeChamferEdge(m, f->handle, 0.3);
    //f = halfedgeChamferEdge(m, m->firstFace->handle, 0.3);
    //halfedgeSaveFaceAsObj(m, f, "face.obj");
    //halfedgeSaveFaceAsObj(m, f->handle->opposite->left, "face2.obj");
    //halfedgeSaveFaceAsObj(m, f->handle->opposite->previous->opposite->left, "face3.obj");
    //halfedgeSaveFaceAsObj(m, f->handle->opposite->next->opposite->left, "face4.obj");
    //f = halfedgeChamferEdge(m, f->handle->opposite->next, 0.3);
    //f = halfedgeChamferVertex(m, m->lastVertex, 0.2);
    printf("saving\n");
    {
        point3d splitPoint = {0, 0, 0};
        vector3d splitVector = {0, 1, 0};
        mesh *m2 = halfedgeSlice(m, &splitPoint, &splitVector);
        halfedgeSaveMeshAsObj(m2, "stone2.obj");
    }
    halfedgeSaveMeshAsObj(m, "stone.obj");
    //halfedgeSaveEdgeLoopAsObj(m, m->firstFace->handle->opposite, "edgeloop.obj");
    printf("end\n");
    return 0;
}
