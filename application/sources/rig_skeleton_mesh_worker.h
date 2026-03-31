#ifndef DUST3D_APPLICATION_RIG_SKELETON_MESH_WORKER_H_
#define DUST3D_APPLICATION_RIG_SKELETON_MESH_WORKER_H_

#include "bone_structure.h"
#include "model_opengl_vertex.h"
#include <QObject>
#include <dust3d/base/color.h>
#include <dust3d/base/object.h>
#include <dust3d/base/vector3.h>
#include <memory>
#include <tuple>
#include <vector>

// Worker class for threaded mesh generation
class RigSkeletonMeshWorker : public QObject {
    Q_OBJECT
public:
    ~RigSkeletonMeshWorker()
    {
        delete m_rigObject;
        delete[] m_combinedVertices;
    }

    void setParameters(const RigStructure& rigStructure, const QString& selectedBoneName, double startRadius)
    {
        m_rigStructure = rigStructure;
        m_selectedBoneName = selectedBoneName;
        m_startRadius = startRadius;
    }

    void setRigObject(dust3d::Object* rigObject, const QString& weightBoneName)
    {
        m_rigObject = rigObject;
        m_weightBoneName = weightBoneName;
    }

    const std::vector<dust3d::Vector3>& getVertices() const { return m_vertices; }
    const std::vector<std::vector<size_t>>& getFaces() const { return m_faces; }
    const std::vector<std::tuple<dust3d::Color, float, float>>* getVertexProperties() const { return m_vertexProperties.get(); }
    const std::vector<ModelOpenGLVertex>& getRigSkeletonVertices() const { return m_rigSkeletonVertices; }
    int getCombinedVertexCount() const { return m_combinedVertexCount; }
    ModelOpenGLVertex* takeCombinedVertices()
    {
        ModelOpenGLVertex* result = m_combinedVertices;
        m_combinedVertices = nullptr;
        return result;
    }
    dust3d::Object* takeRigObject()
    {
        dust3d::Object* result = m_rigObject;
        m_rigObject = nullptr;
        return result;
    }

signals:
    void finished();

public slots:
    void process();

private:
    RigStructure m_rigStructure;
    QString m_selectedBoneName;
    double m_startRadius = 0.05;
    std::vector<dust3d::Vector3> m_vertices;
    std::vector<std::vector<size_t>> m_faces;
    std::unique_ptr<std::vector<std::tuple<dust3d::Color, float, float>>> m_vertexProperties;
    dust3d::Object* m_rigObject = nullptr;
    QString m_weightBoneName;
    std::vector<ModelOpenGLVertex> m_rigSkeletonVertices;
    ModelOpenGLVertex* m_combinedVertices = nullptr;
    int m_combinedVertexCount = 0;
};

#endif
