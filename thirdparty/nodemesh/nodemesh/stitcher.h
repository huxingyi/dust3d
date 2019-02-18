#ifndef NODEMESH_STITCHER_H
#define NODEMESH_STITCHER_H
#include <QVector3D>
#include <vector>
#include <nodemesh/wrapper.h>

namespace nodemesh
{

class Stitcher
{
public:
    ~Stitcher();
    void setVertices(const std::vector<QVector3D> *vertices);
    bool stitch(const std::vector<std::pair<std::vector<size_t>, QVector3D>> &edgeLoops);
    const std::vector<std::vector<size_t>> &newlyGeneratedFaces();
    void getFailedEdgeLoops(std::vector<size_t> &failedEdgeLoops);

private:
    const std::vector<QVector3D> *m_positions;
    std::vector<std::vector<size_t>> m_newlyGeneratedFaces;
    Wrapper *m_wrapper = nullptr;
    
    bool stitchByQuads(const std::vector<std::pair<std::vector<size_t>, QVector3D>> &edgeLoops);
};

}


#endif

