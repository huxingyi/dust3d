#include "rig_skeleton_mesh_worker.h"
#include "rig_skeleton_mesh_generator.h"
#include "theme.h"

void RigSkeletonMeshWorker::process()
{
    // Generate the rig skeleton mesh
    RigSkeletonMeshGenerator meshGenerator;
    meshGenerator.setStartRadius(m_startRadius);
    if (nullptr != m_rigObject) {
        meshGenerator.setNormalizeRequired(false);
    }
    meshGenerator.generateMesh(m_rigStructure, m_selectedBoneName);

    // Store the generated mesh data
    m_vertices = meshGenerator.getVertices();
    m_faces = meshGenerator.getFaces();

    // Copy vertex properties if available
    const auto* props = meshGenerator.getVertexProperties();
    if (props) {
        m_vertexProperties = std::make_unique<std::vector<std::tuple<dust3d::Color, float, float>>>(*props);
    }

    // Build rig skeleton OpenGL vertex array
    if (!m_faces.empty()) {
        std::vector<std::vector<dust3d::Vector3>> triangleVertexNormals;
        for (const auto& face : m_faces) {
            if (face.size() >= 3) {
                dust3d::Vector3 faceNormal = dust3d::Vector3::normal(
                    m_vertices[face[0]], m_vertices[face[1]], m_vertices[face[2]]);
                triangleVertexNormals.push_back({ faceNormal, faceNormal, faceNormal });
            }
        }

        const auto* vertexProperties = m_vertexProperties.get();

        m_rigSkeletonVertices.resize(m_faces.size() * 3);
        int destIndex = 0;
        for (size_t i = 0; i < m_faces.size(); ++i) {
            for (int j = 0; j < 3; j++) {
                int vertexIndex = (int)m_faces[i][j];
                const dust3d::Vector3* srcVert = &m_vertices[vertexIndex];
                const dust3d::Vector3* srcNormal = &triangleVertexNormals[i][j];
                ModelOpenGLVertex* dest = &m_rigSkeletonVertices[destIndex];
                dest->posX = srcVert->x();
                dest->posY = srcVert->y();
                dest->posZ = srcVert->z();
                dest->normX = srcNormal->x();
                dest->normY = srcNormal->y();
                dest->normZ = srcNormal->z();
                dest->texU = 0;
                dest->texV = 0;
                dest->tangentX = 0;
                dest->tangentY = 0;
                dest->tangentZ = 0;
                if (nullptr == vertexProperties) {
                    dest->colorR = Theme::green.redF();
                    dest->colorG = Theme::green.greenF();
                    dest->colorB = Theme::green.blueF();
                    dest->alpha = 1.0;
                    dest->metalness = 0.0;
                    dest->roughness = 1.0;
                } else {
                    const auto& property = (*vertexProperties)[vertexIndex];
                    dest->colorR = std::get<0>(property).r();
                    dest->colorG = std::get<0>(property).g();
                    dest->colorB = std::get<0>(property).b();
                    dest->alpha = std::get<0>(property).alpha();
                    dest->metalness = std::get<1>(property);
                    dest->roughness = std::get<2>(property);
                }
                destIndex++;
            }
        }
    }

    // Build combined vertex array (rig skeleton + character mesh with weight coloring)
    if (!m_rigSkeletonVertices.empty()) {
        size_t rigVertexCount = m_rigSkeletonVertices.size();
        size_t meshVertexCount = 0;
        if (m_rigObject && !m_rigObject->triangles.empty()) {
            meshVertexCount = m_rigObject->triangles.size() * 3;
        }

        size_t totalVertexCount = rigVertexCount + meshVertexCount;
        m_combinedVertices = new ModelOpenGLVertex[totalVertexCount];
        m_combinedVertexCount = (int)totalVertexCount;

        for (size_t i = 0; i < rigVertexCount; ++i) {
            m_combinedVertices[i] = m_rigSkeletonVertices[i];
        }

        if (m_rigObject && !m_rigObject->triangles.empty()) {
            const auto* normals = m_rigObject->triangleVertexNormals();
            const dust3d::Vector3 defaultNormal(0, 0, 1);
            std::string weightBoneStd = m_weightBoneName.toStdString();
            bool hasBindings = !m_rigObject->vertexBone1.empty();

            size_t destIdx = rigVertexCount;
            for (size_t i = 0; i < m_rigObject->triangles.size(); ++i) {
                for (int j = 0; j < 3; j++) {
                    int vi = (int)m_rigObject->triangles[i][j];
                    ModelOpenGLVertex* dest = &m_combinedVertices[destIdx++];

                    dest->posX = m_rigObject->vertices[vi].x();
                    dest->posY = m_rigObject->vertices[vi].y();
                    dest->posZ = m_rigObject->vertices[vi].z();

                    const dust3d::Vector3* srcNormal = &defaultNormal;
                    if (normals)
                        srcNormal = &(*normals)[i][j];
                    dest->normX = srcNormal->x();
                    dest->normY = srcNormal->y();
                    dest->normZ = srcNormal->z();

                    dest->texU = 0;
                    dest->texV = 0;
                    dest->tangentX = 0;
                    dest->tangentY = 0;
                    dest->tangentZ = 0;
                    dest->metalness = 0.0;
                    dest->roughness = 1.0;

                    float weight = 0.0f;
                    if (hasBindings && !weightBoneStd.empty() && (size_t)vi < m_rigObject->vertexBone1.size()) {
                        const auto& b1 = m_rigObject->vertexBone1[vi];
                        if (b1.first == weightBoneStd) {
                            weight += b1.second;
                        }
                        if ((size_t)vi < m_rigObject->vertexBone2.size()) {
                            const auto& b2 = m_rigObject->vertexBone2[vi];
                            if (b2.first == weightBoneStd) {
                                weight += b2.second;
                            }
                        }
                    }

                    if (weight > 0.0f) {
                        dest->colorR = Theme::green.redF() * weight;
                        dest->colorG = Theme::green.greenF() * weight;
                        dest->colorB = Theme::green.blueF() * weight;
                        dest->alpha = 0.8f;
                    } else {
                        dest->colorR = 0.5f;
                        dest->colorG = 0.5f;
                        dest->colorB = 0.5f;
                        dest->alpha = 0.8f;
                    }
                }
            }
        }
    }

    emit finished();
}
