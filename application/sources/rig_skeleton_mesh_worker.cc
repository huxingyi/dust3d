#include "rig_skeleton_mesh_worker.h"
#include "rig_skeleton_mesh_generator.h"

void RigSkeletonMeshWorker::process()
{
    // Generate the rig skeleton mesh
    RigSkeletonMeshGenerator meshGenerator;
    meshGenerator.setStartRadius(m_startRadius);
    meshGenerator.generateMesh(m_rigStructure, m_selectedBoneName);

    // Store the generated mesh data
    m_vertices = meshGenerator.getVertices();
    m_faces = meshGenerator.getFaces();
    
    // Copy vertex properties if available
    const auto* props = meshGenerator.getVertexProperties();
    if (props) {
        m_vertexProperties = std::make_unique<std::vector<std::tuple<dust3d::Color, float, float>>>(*props);
    }

    emit finished();
}
