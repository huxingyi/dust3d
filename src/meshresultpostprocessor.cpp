#include <QGuiApplication>
#include "meshresultpostprocessor.h"
#include "uvunwrap.h"
#include "triangletangentresolve.h"
#include "anglesmooth.h"

MeshResultPostProcessor::MeshResultPostProcessor(const Outcome &outcome)
{
    m_outcome = new Outcome;
    *m_outcome = outcome;
}

MeshResultPostProcessor::~MeshResultPostProcessor()
{
    delete m_outcome;
}

Outcome *MeshResultPostProcessor::takePostProcessedOutcome()
{
    Outcome *outcome = m_outcome;
    m_outcome = nullptr;
    return outcome;
}

void MeshResultPostProcessor::poseProcess()
{
    if (!m_outcome->nodes.empty()) {
        {
            std::vector<std::vector<QVector2D>> triangleVertexUvs;
            std::set<int> seamVertices;
            uvUnwrap(*m_outcome, triangleVertexUvs, seamVertices);
            m_outcome->setTriangleVertexUvs(triangleVertexUvs);
        }
        
        {
            std::vector<QVector3D> triangleTangents;
            triangleTangentResolve(*m_outcome, triangleTangents);
            m_outcome->setTriangleTangents(triangleTangents);
        }
        
        {
            std::vector<QVector3D> inputVerticies;
            std::vector<std::tuple<size_t, size_t, size_t>> inputTriangles;
            std::vector<QVector3D> inputNormals;
            float thresholdAngleDegrees = 60;
            for (const auto &vertex: m_outcome->vertices)
                inputVerticies.push_back(vertex);
            for (size_t i = 0; i < m_outcome->triangles.size(); ++i) {
                const auto &triangle = m_outcome->triangles[i];
                const auto &triangleNormal = m_outcome->triangleNormals[i];
                inputTriangles.push_back(std::make_tuple((size_t)triangle[0], (size_t)triangle[1], (size_t)triangle[2]));
                inputNormals.push_back(triangleNormal);
            }
            std::vector<QVector3D> resultNormals;
            angleSmooth(inputVerticies, inputTriangles, inputNormals, thresholdAngleDegrees, resultNormals);
            std::vector<std::vector<QVector3D>> triangleVertexNormals;
            triangleVertexNormals.resize(m_outcome->triangles.size(), {
                QVector3D(), QVector3D(), QVector3D()
            });
            size_t index = 0;
            for (size_t i = 0; i < m_outcome->triangles.size(); ++i) {
                auto &normals = triangleVertexNormals[i];
                for (size_t j = 0; j < 3; ++j) {
                    if (index < resultNormals.size())
                        normals[j] = resultNormals[index];
                    ++index;
                }
            }
            m_outcome->setTriangleVertexNormals(triangleVertexNormals);
        }
    }
}

void MeshResultPostProcessor::process()
{
    poseProcess();

    this->moveToThread(QGuiApplication::instance()->thread());
    emit finished();
}
