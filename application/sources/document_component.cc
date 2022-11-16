#include "document.h"

Document::Component::Component()
{
}

Document::Component::Component(const dust3d::Uuid& withId, const QString& linkData, const QString& linkDataType)
{
    id = withId.isNull() ? dust3d::Uuid::createUuid() : withId;
    if (!linkData.isEmpty()) {
        if ("partId" == linkDataType) {
            linkToPartId = dust3d::Uuid(linkData.toUtf8().constData());
        }
    }
}

QString Document::Component::linkData() const
{
    return linkToPartId.isNull() ? QString() : QString(linkToPartId.toString().c_str());
}

QString Document::Component::linkDataType() const
{
    return linkToPartId.isNull() ? QString() : QString("partId");
}

void Document::Component::addChild(dust3d::Uuid childId)
{
    if (m_childrenIdSet.find(childId) != m_childrenIdSet.end())
        return;
    m_childrenIdSet.insert(childId);
    childrenIds.push_back(childId);
}

void Document::Component::replaceChildWithOthers(const dust3d::Uuid& childId, const std::vector<dust3d::Uuid>& others)
{
    if (m_childrenIdSet.find(childId) == m_childrenIdSet.end())
        return;
    m_childrenIdSet.erase(childId);
    std::vector<dust3d::Uuid> candidates;
    for (const auto& it : others) {
        if (m_childrenIdSet.find(it) == m_childrenIdSet.end()) {
            m_childrenIdSet.insert(it);
            candidates.emplace_back(it);
        }
    }
    for (size_t i = 0; i < childrenIds.size(); ++i) {
        if (childId == childrenIds[i]) {
            size_t newAddSize = candidates.size() - 1;
            if (newAddSize > 0) {
                size_t oldSize = childrenIds.size();
                childrenIds.resize(childrenIds.size() + newAddSize);
                for (int j = (int)oldSize - 1; j > (int)i; --j) {
                    childrenIds[j + newAddSize] = childrenIds[j];
                }
            }
            for (size_t k = 0; k < candidates.size(); ++k)
                childrenIds[i + k] = candidates[k];
            break;
        }
    }
}

void Document::Component::removeChild(dust3d::Uuid childId)
{
    if (m_childrenIdSet.find(childId) == m_childrenIdSet.end())
        return;
    m_childrenIdSet.erase(childId);
    auto findResult = std::find(childrenIds.begin(), childrenIds.end(), childId);
    if (findResult != childrenIds.end())
        childrenIds.erase(findResult);
}

void Document::Component::replaceChild(dust3d::Uuid childId, dust3d::Uuid newId)
{
    if (m_childrenIdSet.find(childId) == m_childrenIdSet.end())
        return;
    if (m_childrenIdSet.find(newId) != m_childrenIdSet.end())
        return;
    m_childrenIdSet.erase(childId);
    m_childrenIdSet.insert(newId);
    auto findResult = std::find(childrenIds.begin(), childrenIds.end(), childId);
    if (findResult != childrenIds.end())
        *findResult = newId;
}

void Document::Component::moveChildUp(dust3d::Uuid childId)
{
    auto it = std::find(childrenIds.begin(), childrenIds.end(), childId);
    if (it == childrenIds.end()) {
        return;
    }

    auto index = std::distance(childrenIds.begin(), it);
    if (index == 0)
        return;
    std::swap(childrenIds[index - 1], childrenIds[index]);
}

void Document::Component::moveChildDown(dust3d::Uuid childId)
{
    auto it = std::find(childrenIds.begin(), childrenIds.end(), childId);
    if (it == childrenIds.end()) {
        return;
    }

    auto index = std::distance(childrenIds.begin(), it);
    if (index == (int)childrenIds.size() - 1)
        return;
    std::swap(childrenIds[index], childrenIds[index + 1]);
}

void Document::Component::moveChildToTop(dust3d::Uuid childId)
{
    auto it = std::find(childrenIds.begin(), childrenIds.end(), childId);
    if (it == childrenIds.end()) {
        return;
    }

    auto index = std::distance(childrenIds.begin(), it);
    if (index == 0)
        return;
    for (int i = index; i >= 1; i--)
        std::swap(childrenIds[i - 1], childrenIds[i]);
}

void Document::Component::moveChildToBottom(dust3d::Uuid childId)
{
    auto it = std::find(childrenIds.begin(), childrenIds.end(), childId);
    if (it == childrenIds.end()) {
        return;
    }

    auto index = std::distance(childrenIds.begin(), it);
    if (index == (int)childrenIds.size() - 1)
        return;
    for (int i = index; i <= (int)childrenIds.size() - 2; i++)
        std::swap(childrenIds[i], childrenIds[i + 1]);
}

void Document::Component::updatePreviewMesh(std::unique_ptr<ModelMesh> mesh)
{
    m_previewMesh = std::move(mesh);
    isPreviewMeshObsolete = true;
}

ModelMesh* Document::Component::takePreviewMesh() const
{
    if (nullptr == m_previewMesh)
        return nullptr;
    return new ModelMesh(*m_previewMesh);
}
