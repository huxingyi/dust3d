#ifndef SKELETON_TO_MESH_H
#define SKELETON_TO_MESH_H
#include <QObject>
#include <QList>
#include <vector>
#include <map>
#include "skeletoneditgraphicsview.h"
#include "mesh.h"
#include "skeletonsnapshot.h"

class SkeletonToMesh : public QObject 
{
    Q_OBJECT
public:
    SkeletonToMesh(SkeletonSnapshot *snapshot);
    ~SkeletonToMesh();
    Mesh *takeResultMesh();
signals:
    void finished();
public slots:
    void process();
private:
    Mesh *m_mesh;
    SkeletonSnapshot m_snapshot;
};

#endif
