#ifndef DUST3D_APPLICATION_RIG_SKELETON_MESH_WORKER_H_
#define DUST3D_APPLICATION_RIG_SKELETON_MESH_WORKER_H_

#include <QObject>
#include <dust3d/base/vector3.h>
#include <dust3d/base/color.h>
#include <vector>
#include <tuple>
#include <memory>
#include "bone_structure.h"

// Worker class for threaded mesh generation
class RigSkeletonMeshWorker : public QObject {
    Q_OBJECT
public:
    void setParameters(const RigStructure& rigStructure, const QString& selectedBoneName, double startRadius)
    {
        m_rigStructure = rigStructure;
        m_selectedBoneName = selectedBoneName;
        m_startRadius = startRadius;
    }

    const std::vector<dust3d::Vector3>& getVertices() const { return m_vertices; }
    const std::vector<std::vector<size_t>>& getFaces() const { return m_faces; }
    const std::vector<std::tuple<dust3d::Color, float, float>>* getVertexProperties() const { return m_vertexProperties.get(); }

signals:
    void finished();

public slots:
    void process();

private:
    RigStructure m_rigStructure;
    QString m_selectedBoneName;
    double m_startRadius = 0.02;
    std::vector<dust3d::Vector3> m_vertices;
    std::vector<std::vector<size_t>> m_faces;
    std::unique_ptr<std::vector<std::tuple<dust3d::Color, float, float>>> m_vertexProperties;
};

#endif
