#ifndef DUST3D_RECOMBINER_H
#define DUST3D_RECOMBINER_H
#include <QVector3D>
#include <vector>
#include <set>
#include <map>
#include "meshcombiner.h"

class MeshRecombiner
{
public:
    void setVertices(const std::vector<QVector3D> *vertices,
        const std::vector<std::pair<MeshCombiner::Source, size_t>> *verticesSourceIndices);
    void setFaces(const std::vector<std::vector<size_t>> *faces);
    const std::vector<QVector3D> &regeneratedVertices();
    const std::vector<std::pair<MeshCombiner::Source, size_t>> &regeneratedVerticesSourceIndices();
    const std::vector<std::vector<size_t>> &regeneratedFaces();
    bool recombine();
    
private:
    const std::vector<QVector3D> *m_vertices = nullptr;
    const std::vector<std::pair<MeshCombiner::Source, size_t>> *m_verticesSourceIndices = nullptr;
    const std::vector<std::vector<size_t>> *m_faces = nullptr;
    std::vector<QVector3D> m_regeneratedVertices;
    std::vector<std::pair<MeshCombiner::Source, size_t>> m_regeneratedVerticesSourceIndices;
    std::vector<std::vector<size_t>> m_regeneratedFaces;
    std::map<std::pair<size_t, size_t>, size_t> m_halfEdgeToFaceMap;
    std::map<size_t, size_t> m_facesInSeamArea;
    std::set<size_t> m_goodSeams;
    
    bool addFaceToHalfEdgeToFaceMap(size_t faceIndex,
        std::map<std::pair<size_t, size_t>, size_t> &halfEdgeToFaceMap);
    bool buildHalfEdgeToFaceMap(std::map<std::pair<size_t, size_t>, size_t> &halfEdgeToFaceMap);
    bool convertHalfEdgesToEdgeLoops(const std::vector<std::pair<size_t, size_t>> &halfEdges,
        std::vector<std::vector<size_t>> *edgeLoops);
    size_t splitSeamVerticesToIslands(const std::map<size_t, std::vector<size_t>> &seamEdges,
        std::map<size_t, size_t> *vertexToIslandMap);
    void copyNonSeamFacesAsRegenerated();
    size_t adjustTrianglesFromSeam(std::vector<size_t> &edgeLoop, size_t seamIndex);
    size_t otherVertexOfTriangle(const std::vector<size_t> &face, const std::vector<size_t> &indices);
    bool bridge(const std::vector<size_t> &first, const std::vector<size_t> &second);
    size_t nearestIndex(const QVector3D &position, const std::vector<size_t> &edgeLoop);
    void removeReluctantVertices();
    void fillPairs(const std::vector<size_t> &small, const std::vector<size_t> &large);
};

#endif
