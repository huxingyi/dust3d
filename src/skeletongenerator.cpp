#include <set>
#include <QGuiApplication>
#include <QDebug>
#include <vector>
#include "skeletongenerator.h"
#include "positionmap.h"
#include "meshlite.h"
#include "jointnodetree.h"

SkeletonGenerator::SkeletonGenerator(const MeshResultContext &meshResultContext) :
    m_resultSkeletonMesh(nullptr)
{
    m_meshResultContext = new MeshResultContext;
    *m_meshResultContext = meshResultContext;
}

SkeletonGenerator::~SkeletonGenerator()
{
    delete m_resultSkeletonMesh;
    delete m_meshResultContext;
}

MeshLoader *SkeletonGenerator::takeResultSkeletonMesh()
{
    MeshLoader *resultSkeletonMesh = m_resultSkeletonMesh;
    m_resultSkeletonMesh = nullptr;
    return resultSkeletonMesh;
}

MeshResultContext *SkeletonGenerator::takeResultContext()
{
    MeshResultContext *resultContext = m_meshResultContext;
    m_meshResultContext = nullptr;
    return resultContext;
}

MeshLoader *SkeletonGenerator::createSkeletonMesh()
{
    void *meshliteContext = meshlite_create_context();
    int sklt = meshlite_skeletonmesh_create(meshliteContext);
    
    JointNodeTree jointNodeTree(*m_meshResultContext);
    for (const auto &joint: jointNodeTree.joints()) {
        for (const auto &childIndex: joint.children) {
            const auto &child = jointNodeTree.joints()[childIndex];
            meshlite_skeletonmesh_add_bone(meshliteContext, sklt,
                joint.position.x(), joint.position.y(), joint.position.z(),
                child.position.x(), child.position.y(), child.position.z());
        }
    }

    int meshId = meshlite_skeletonmesh_generate_mesh(meshliteContext, sklt);
    MeshLoader *skeletonMesh = new MeshLoader(meshliteContext, meshId, -1, Theme::green);
    
    meshlite_destroy_context(meshliteContext);
    
    return skeletonMesh;
}

void SkeletonGenerator::generate()
{
    Q_ASSERT(nullptr == m_resultSkeletonMesh);
    m_resultSkeletonMesh = createSkeletonMesh();
}

void SkeletonGenerator::process()
{
    generate();
    
    this->moveToThread(QGuiApplication::instance()->thread());
    emit finished();
}
