#ifndef SKELETON_GENERATOR_H
#define SKELETON_GENERATOR_H
#include <QObject>
#include "meshresultcontext.h"
#include "meshloader.h"

// https://raw.githubusercontent.com/KhronosGroup/glTF/master/specification/2.0/figures/gltfOverview-2.0.0a.png

class SkeletonGenerator : public QObject
{
    Q_OBJECT
public:
    SkeletonGenerator(const MeshResultContext &meshResultContext);
    ~SkeletonGenerator();
    MeshLoader *takeResultSkeletonMesh();
    MeshResultContext *takeResultContext();
signals:
    void finished();
public slots:
    void process();
private:
    void combineAllBmeshSkeletons();
    MeshLoader *createSkeletonMesh();
private:
    MeshResultContext *m_meshResultContext;
    MeshLoader *m_resultSkeletonMesh;
};

#endif
