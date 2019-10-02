#include "riggerconstruct.h"
#include "animalrigger.h"

Rigger *newRigger(RigType rigType, const std::vector<QVector3D> &verticesPositions,
        const std::set<MeshSplitterTriangle> &inputTriangles,
        const std::vector<std::pair<std::pair<size_t, size_t>, std::pair<size_t, size_t>>> &triangleLinks)
{
    if (rigType == RigType::Animal)
        return new AnimalRigger(verticesPositions, inputTriangles, triangleLinks);
    return nullptr;
}
