#include "document.h"

Document::Edge::Edge(const dust3d::Uuid& withId)
{
    id = withId.isNull() ? dust3d::Uuid::createUuid() : withId;
}

dust3d::Uuid Document::Edge::neighborOf(dust3d::Uuid nodeId) const
{
    if (nodeIds.size() != 2)
        return dust3d::Uuid();
    return nodeIds[0] == nodeId ? nodeIds[1] : nodeIds[0];
}
