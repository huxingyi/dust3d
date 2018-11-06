#include "riggerconstruct.h"
#include "animalrigger.h"

Rigger *newRigger(RigType rigType, const std::vector<QVector3D> &verticesPositions,
        const std::set<MeshSplitterTriangle> &inputTriangles)
{
    if (rigType == RigType::Animal)
        return new AnimalRigger(verticesPositions, inputTriangles);
    return nullptr;
}