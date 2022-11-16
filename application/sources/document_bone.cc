#include "document.h"

Document::Bone::Bone(const dust3d::Uuid& withId)
{
    id = withId.isNull() ? dust3d::Uuid::createUuid() : withId;
}
