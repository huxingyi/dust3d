#ifndef DUST3D_WRAPPER_H
#define DUST3D_WRAPPER_H
#include <QVector3D>
#include <vector>
#include <map>
#include <deque>

class MeshWrapper
{
public:
    void setVertices(const std::vector<QVector3D> *vertices);
    void wrap(const std::vector<std::pair<std::vector<size_t>, QVector3D>> &edgeLoops);
    const std::vector<std::vector<size_t>> &newlyGeneratedFaces();
    bool finished();
    void getFailedEdgeLoops(std::vector<size_t> &failedEdgeLoops);

private:
    struct WrapItemKey
    {
        size_t p1;
        size_t p2;
        
        bool operator <(const WrapItemKey &right) const
        {
            if (p1 < right.p1)
                return true;
            if (p1 > right.p1)
                return false;
            if (p2 < right.p2)
                return true;
            if (p2 > right.p2)
                return false;
            return false;
        }
    };
    
    struct WrapItem
    {
        QVector3D baseNormal;
        size_t p1;
        size_t p2;
        size_t p3;
        bool processed;
    };
    
    struct Face3
    {
        size_t p1;
        size_t p2;
        size_t p3;
        QVector3D normal;
        size_t index;
    };
    
    struct Face4
    {
        size_t p1;
        size_t p2;
        size_t p3;
        size_t p4;
    };
    
    struct SourceVertex
    {
        QVector3D position;
        size_t sourcePlane;
        size_t index;
        size_t tag;
    };
    
    std::vector<WrapItem> m_items;
    std::map<WrapItemKey, size_t> m_itemsMap;
    std::deque<size_t> m_itemsList;
    const std::vector<QVector3D> *m_positions;
    std::vector<size_t> m_candidates;
    std::vector<SourceVertex> m_sourceVertices;
    std::vector<Face3> m_generatedFaces;
    std::map<WrapItemKey, std::pair<size_t, bool>> m_generatedFaceEdgesMap;
    std::map<size_t, std::vector<size_t>> m_generatedVertexEdgesMap;
    bool m_finalizeFinished = false;
    std::vector<std::vector<size_t>> m_newlyGeneratedfaces;
    
    void addCandidateVertices(const std::vector<size_t> &vertices, const QVector3D &planeNormal, size_t planeId);
    size_t addSourceVertex(const QVector3D &position, size_t sourcePlane, size_t tag);
    void addStartup(size_t p1, size_t p2, const QVector3D &baseNormal);
    QVector3D calculateFaceVector(size_t p1, size_t p2, const QVector3D &baseNormal);
    void addItem(size_t p1, size_t p2, const QVector3D &baseNormal);
    std::pair<size_t, bool> findItem(size_t p1, size_t p2);
    bool isEdgeGenerated(size_t p1, size_t p2);
    float angleOfBaseFaceAndPoint(size_t itemIndex, size_t vertexIndex);
    std::pair<size_t, bool> findBestVertexOnTheLeft(size_t itemIndex);
    std::pair<size_t, bool> peekItem();
    bool isEdgeClosed(size_t p1, size_t p2);
    bool isVertexClosed(size_t vertexIndex);
    void generate();
    size_t anotherVertexIndexOfFace3(const Face3 &f, size_t p1, size_t p2);
    std::pair<size_t, bool> findPairFace3(const Face3 &f, std::map<size_t, bool> &usedIds, std::vector<Face4> &q);
    void finalize();
    bool almostEqual(const QVector3D &v1, const QVector3D &v2);
};

#endif
