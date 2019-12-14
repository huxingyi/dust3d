#ifndef DUST3D_STITCHER_H
#define DUST3D_STITCHER_H
#include <QVector3D>
#include <vector>
#include "meshwrapper.h"

class MeshStitcher
{
public:
    ~MeshStitcher();
    void setVertices(const std::vector<QVector3D> *vertices);
    bool stitch(const std::vector<std::pair<std::vector<size_t>, QVector3D>> &edgeLoops);
    const std::vector<std::vector<size_t>> &newlyGeneratedFaces();
    void getFailedEdgeLoops(std::vector<size_t> &failedEdgeLoops);

private:
    const std::vector<QVector3D> *m_positions;
    std::vector<std::vector<size_t>> m_newlyGeneratedFaces;
    MeshWrapper *m_wrapper = nullptr;
    
    bool stitchByQuads(const std::vector<std::pair<std::vector<size_t>, QVector3D>> &edgeLoops);
};

#endif

