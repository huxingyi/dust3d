#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>
#include <cmath>
#include "projectfacestonodes.h"
#include "util.h"

class FacesToNodesProjector
{
public:
    FacesToNodesProjector(const std::vector<QVector3D> *vertices,
            const std::vector<std::vector<size_t>> *faces,
            const std::vector<std::pair<QVector3D, float>> *sourceNodes,
            std::vector<size_t> *faceSources) :
        m_vertices(vertices),
        m_faces(faces),
        m_sourceNodes(sourceNodes),
        m_faceSources(faceSources)
    {
    }
    bool intersectionTest(const std::vector<size_t> &face,
            const QVector3D &nodePosition, float nodeRadius,
            float *distance2) const
    {
        QVector3D faceCenter;
        for (const auto &it: face) {
            faceCenter += (*m_vertices)[it];
        }
        if (face.size() > 0)
            faceCenter /= face.size();
        const auto &A = faceCenter;
        const auto &C = nodePosition;
        auto B = -polygonNormal(*m_vertices, face);
        auto a = QVector3D::dotProduct(B, B);
        auto b = 2.0f * QVector3D::dotProduct(B, A - C);
        const auto &r = nodeRadius;
        auto c = QVector3D::dotProduct(A - C, A - C) - r * r;
        if (b * b - 4 * a * c <= 0)
            return false;
        *distance2 = (faceCenter - nodePosition).lengthSquared();
        return true;
    }
    void operator()(const tbb::blocked_range<size_t> &range) const
    {
        for (size_t i = range.begin(); i != range.end(); ++i) {
            std::vector<std::pair<size_t, float>> distance2WithNodes;
            for (size_t j = 0; j < m_sourceNodes->size(); ++j) {
                const auto &node = (*m_sourceNodes)[j];
                float distance2 = 0.0f;
                if (!intersectionTest((*m_faces)[i], node.first, node.second, &distance2))
                    continue;
                distance2WithNodes.push_back(std::make_pair(j, distance2));
            }
            if (distance2WithNodes.empty())
                continue;
            (*m_faceSources)[i] = std::min_element(distance2WithNodes.begin(), distance2WithNodes.end(), [](const std::pair<size_t, float> &first, const std::pair<size_t, float> &second) {
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
    faceSources->resize(faces.size());
    tbb::parallel_for(tbb::blocked_range<size_t>(0, faces.size()),
        FacesToNodesProjector(&vertices, &faces, &sourceNodes, faceSources));
}
