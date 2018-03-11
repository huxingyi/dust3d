#include "skeletontomesh.h"
#include "meshlite.h"

// Modified from https://wiki.qt.io/QThreads_general_usage

SkeletonToMesh::SkeletonToMesh(SkeletonEditGraphicsView *graphicsView) :
    m_mesh(NULL)
{
}

SkeletonToMesh::~SkeletonToMesh()
{
    delete m_mesh;
}

Mesh *SkeletonToMesh::takeResultMesh()
{
    Mesh *mesh = m_mesh;
    m_mesh = NULL;
    return mesh;
}

void SkeletonToMesh::process()
{
    void *lite = meshlite_create_context();
    int first = meshlite_import(lite, "../assets/cube.obj");
    int second = meshlite_import(lite, "../assets/ball.obj");
    meshlite_scale(lite, first, 0.65);
    int merged = meshlite_union(lite, first, second);
    int triangulate = meshlite_triangulate(lite, merged);
    //meshlite_export(lite, triangulate, "/Users/jeremy/testlib.obj");
    m_mesh = new Mesh(lite, triangulate);
    emit finished();
}
