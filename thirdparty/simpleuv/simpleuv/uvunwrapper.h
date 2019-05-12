#ifndef SIMPLEUV_UV_UNWRAPPER_H
#define SIMPLEUV_UV_UNWRAPPER_H
#include <vector>
#include <map>
#include <QRectF>
#include <simpleuv/meshdatatype.h>

namespace simpleuv 
{

class UvUnwrapper
{
public:
    void setMesh(const Mesh &mesh);
    void unwrap();
    const std::vector<FaceTextureCoords> &getFaceUvs() const;
    const std::vector<QRectF> &getChartRects() const;
    const std::vector<int> &getChartSourcePartitions() const;

private:
    void partition();
    void splitPartitionToIslands(const std::vector<size_t> &group, std::vector<std::vector<size_t>> &islands);
    void unwrapSingleIsland(const std::vector<size_t> &group, int sourcePartition, bool skipCheckHoles=false);
    void parametrizeSingleGroup(const std::vector<Vertex> &verticies,
        const std::vector<Face> &faces,
        std::map<size_t, size_t> &localToGlobalFacesMap,
        size_t faceNumToChart,
        int sourcePartition);
    bool fixHolesExceptTheLongestRing(const std::vector<Vertex> &verticies, std::vector<Face> &faces, size_t *remainingHoleNum=nullptr);
    void makeSeamAndCut(const std::vector<Vertex> &verticies,
        const std::vector<Face> &faces,
        std::map<size_t, size_t> &localToGlobalFacesMap,
        std::vector<size_t> &firstGroup, std::vector<size_t> &secondGroup);
    void calculateSizeAndRemoveInvalidCharts();
    void packCharts();
    void finalizeUv();
    void buildEdgeToFaceMap(const std::vector<size_t> &group, std::map<std::pair<size_t, size_t>, size_t> &edgeToFaceMap);
    void buildEdgeToFaceMap(const std::vector<Face> &faces, std::map<std::pair<size_t, size_t>, size_t> &edgeToFaceMap);
    double distanceBetweenVertices(const Vertex &first, const Vertex &second);
    void triangulateRing(const std::vector<Vertex> &verticies,
        std::vector<Face> &faces, const std::vector<size_t> &ring);
    void calculateFaceTextureBoundingBox(const std::vector<FaceTextureCoords> &faceTextureCoords,
        float &left, float &top, float &right, float &bottom);

    Mesh m_mesh;
    std::vector<FaceTextureCoords> m_faceUvs;
    std::map<int, std::vector<size_t>> m_partitions;
    std::vector<std::pair<std::vector<size_t>, std::vector<FaceTextureCoords>>> m_charts;
    std::vector<std::pair<float, float>> m_chartSizes;
    std::vector<QRectF> m_chartRects;
    std::vector<int> m_chartSourcePartitions;
};

}

#endif
