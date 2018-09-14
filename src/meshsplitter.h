#ifndef MESH_SPLITTER_H
#define MESH_SPLITTER_H
#include <set>

class MeshSplitterTriangle
{
public:
    int indicies[3] = {0, 0, 0};
    
    bool operator<(const MeshSplitterTriangle &other) const
    {
        return std::make_tuple(indicies[0], indicies[1], indicies[2]) <
            std::make_tuple(other.indicies[0], other.indicies[1], other.indicies[2]);
    }
};

class MeshSplitter
{
public:
    static bool split(const std::set<MeshSplitterTriangle> &input, 
        const std::set<MeshSplitterTriangle> &splitter,
        std::set<MeshSplitterTriangle> &firstGroup,
        std::set<MeshSplitterTriangle> &secondGroup);
};

#endif
