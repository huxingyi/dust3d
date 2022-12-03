#include "document.h"

Document::Bone::Bone(const dust3d::Uuid& withId)
{
    id = withId.isNull() ? dust3d::Uuid::createUuid() : withId;
}

void Document::Bone::updatePreviewMesh(std::unique_ptr<ModelMesh> mesh)
{
    m_previewMesh = std::move(mesh);
    isPreviewMeshObsolete = true;
}

ModelMesh* Document::Bone::takePreviewMesh() const
{
    if (nullptr == m_previewMesh)
        return nullptr;
    return new ModelMesh(*m_previewMesh);
}
