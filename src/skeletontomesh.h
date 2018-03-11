#ifndef SKELETON_TO_MESH_H
#define SKELETON_TO_MESH_H
#include <QObject>
#include <QList>
#include "skeletoneditgraphicsview.h"
#include "mesh.h"

class SkeletonToMesh : public QObject 
{
    Q_OBJECT
public:
    SkeletonToMesh(SkeletonEditGraphicsView *graphicsView);
    ~SkeletonToMesh();
    Mesh *takeResultMesh();
signals:
    void finished();
public slots:
    void process();
private:
    Mesh *m_mesh;
};

#endif
