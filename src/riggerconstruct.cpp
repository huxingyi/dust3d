#include "riggerconstruct.h"
#include "tetrapodrigger.h"
#include "genericrigger.h"

Rigger *newRigger(RigType rigType, const std::vector<QVector3D> &verticesPositions,
        const std::set<MeshSplitterTriangle> &inputTriangles)
{
    if (rigType == RigType::Tetrapod)
        return new TetrapodRigger(verticesPositions, inputTriangles);
    else if (rigType == RigType::Generic)
        return new GenericRigger(verticesPositions, inputTriangles);
    return nullptr;
}