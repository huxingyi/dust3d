#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>
#include <cmath>
#include <iostream>
#include "projectfacestonodes.h"
#include "util.h"

class FacesToNearestNodesProjector
{
public:
    FacesToNearestNodesProjector(const std::vector<QVector3D> *vertices,
            const std::vector<std::vector<size_t>> *faces,
            const std::vector<std::pair<QVector3D, float>> *sourceNodes,
            std::vector<size_t> *faceSources) :
        m_vertices(vertices),
        m_faces(faces),
        m_sourceNodes(sourceNodes),
        m_faceSources(faceSources)
    {
    }
    bool test(const std::vector<size_t> &face,
            const QVector3D &nodePosition,
            float nodeRadius,
            float *distance) const
    {
        QVector3D faceCenter;
        for (const auto &it: face) {
            faceCenter += (*m_vertices)[it];
        }
        if (face.size() > 0)
            faceCenter /= face.size();
        auto ray = (nodePosition - faceCenter).normalized();
        auto inversedFaceNormal = -polygonNormal(*m_vertices, face);
        *distance = (faceCenter - nodePosition).length();
        if (*distance > nodeRadius * 1.5f)
            return false;
        if (QVector3D::dotProduct(ray, inversedFaceNormal) < 0.707) { // 45 degrees
            if (*distance > nodeRadius * 1.01f)
                return false;
        }
        return true;
    }
    void operator()(const tbb::blocked_range<size_t> &range) const
    {
        for (size_t i = range.begin(); i != range.end(); ++i) {
            std::vector<std::pair<size_t, float>> distanceWithNodes;
            for (size_t j = 0; j < m_sourceNodes->size(); ++j) {
                const auto &node = (*m_sourceNodes)[j];
                float distance = 0.0f;
                if (!test((*m_faces)[i], node.first, node.second, &distance))
                    continue;
                distanceWithNodes.push_back(std::make_pair(j, distance));
            }
            if (distanceWithNodes.empty())
                continue;
            (*m_faceSources)[i] = std::min_element(distanceWithNodes.begin(), distanceWithNodes.end(), [](const std::pair<size_t, float> &first, const std::pair<size_t, float> &second) {
                return first.second < second.second;
            })->first;
        }
    }
private:
    const std::vector<QVector3D> *m_vertices = nullptr;
    const std::vector<std::vector<size_t>> *m_faces = nullptr;
    const std::vector<std::pair<QVector3D, float>> *m_sourceNodes = nullptr;
    std::vector<size_t> *m_faceSources = nullptr;
};

void projectFacesToNodes(const std::vector<QVector3D> &vertices,
    const std::vector<std::vector<size_t>> &faces,
    const std::vector<std::pair<QVector3D, float>> &sourceNodes,
    std::vector<size_t> *faceSources)
{
    // Resolve the faces's source nodes
    faceSources->resize(faces.size(), std::numeric_limits<size_t>::max());
    tbb::parallel_for(tbb::blocked_range<size_t>(0, faces.size()),
        FacesToNearestNodesProjector(&vertices, &faces, &sourceNodes, faceSources));
}
