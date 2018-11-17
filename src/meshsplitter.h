#ifndef DUST3D_MESH_SPLITTER_H
#define DUST3D_MESH_SPLITTER_H
#include <set>

class MeshSplitterTriangle
{
public:
    int indices[3] = {0, 0, 0};
    
    bool operator<(const MeshSplitterTriangle &other) const
    {
        return std::make_tuple(indices[0], indices[1], indices[2]) <
            std::make_tuple(other.indices[0], other.indices[1], other.indices[2]);
    }
};

class MeshSplitter
{
public:
    static bool split(const std::set<MeshSplitterTriangle> &input, 
        std::set<MeshSplitterTriangle> &splitter,
        std::set<MeshSplitterTriangle> &firstGroup,
        std::set<MeshSplitterTriangle> &secondGroup,
        bool expandSplitter=false);
};

#endif
