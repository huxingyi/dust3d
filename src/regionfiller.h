#ifndef DUST3D_REGION_FILLER_H
#define DUST3D_REGION_FILLER_H
#include <QVector3D>
#include <vector>
#include <set>

class RegionFiller
{
public:
    struct Node
    {
        QVector3D position;
        float radius = 0.0;
        size_t source = 0;
    };
    RegionFiller(const std::vector<Node> *vertices,
        const std::vector<std::vector<size_t>> *polylines);
    ~RegionFiller();
    bool fill();
    void fillWithoutPartition();
    const std::vector<Node> &getOldAndNewVertices();
    const std::vector<std::vector<size_t>> &getNewFaces();
private:
    const std::vector<Node> *m_sourceVertices = nullptr;
    std::vector<std::vector<size_t>> *m_sourcePolylines = nullptr;
    int m_sideNum = 0;
    int m_sumOfSegments = 0;
    std::vector<int> m_sideSegmentNums;
    std::vector<Node> m_oldAndNewVertices;
    std::vector<std::vector<size_t>> m_newFaces;
    std::vector<std::vector<std::vector<size_t>>> m_newRegions;
    std::set<size_t> m_centerSources;
    float averageRadius(size_t *maxNodeIndex=nullptr);
    void createPointsBetween(size_t fromIndex,
        size_t toIndex,
        size_t segments,
        std::vector<size_t> *newPointIndices);
    std::vector<size_t> createPointsBetween(size_t fromIndex,
        size_t toIndex,
        size_t segments);
    void collectEdgePoints(size_t polyline, int startPos, int stopPos,
        std::vector<size_t> *pointIndices);
    std::vector<size_t> collectEdgePoints(size_t polyline, int startPos, int stopPos);
    std::vector<size_t> reversed(const std::vector<size_t> &pointIndices);
    std::vector<size_t> createPointsToMapBetween(size_t fromIndex,
        size_t toIndex,
        size_t segments,
        std::map<std::pair<size_t, size_t>, std::vector<size_t>> *map);
    bool resolveOddSidedEvenSumOfSegments(int siUpperBound);
    bool resolveOddSidedOddSumOfSegments(int siUpperBound);
    bool resolveEvenSidedEvenSumOfSegmentsAndBothL2L2AreEven();
    bool resolveEvenSidedEvenSumOfSegmentsAndBothL2L2AreOdd();
    bool resolveEvenSideEvenSumOfSegmentsAndL1IsEvenL2IsOdd();
    bool resolveEvenSideEvenSumOfSegmentsAndL1IsOddL2IsEven();
    bool resolveQuadrilateralRegion();
    bool resolveQuadrilateralRegionWithIntegerSolution(int m, int n, int p, int q, bool pqSwapped);
    bool resolveQuadrilateralRegionMismatchSimilarCase(int m, int n, int p, int q, bool pqSwapped);
    bool resolveQuadrilateralRegionWithNonIntegerSolution(int m, int n, int p, int q, bool pqSwapped);
    bool resolveQuadrilateralRegionDirectCase();
    void convertRegionsToFaces();
    void prepare();
    bool createCoonsPatch(const std::vector<std::vector<size_t>> &region);
    bool convertRegionsToPatches();
    bool createCoonsPatchFrom(const std::vector<size_t> &c0,
        const std::vector<size_t> &c1,
        const std::vector<size_t> &d0,
        const std::vector<size_t> &d1,
        bool fillLastTriangle=false);
    bool createCoonsPatchThreeSidedRegion(const std::vector<std::vector<size_t>> &region);
    void convertPolylinesToFaces();
};

#endif

