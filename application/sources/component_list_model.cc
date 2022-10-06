#include <QAbstractListModel>
#include <dust3d/base/debug.h>
#include "component_list_model.h"
#include "document.h"

ComponentListModel::ComponentListModel(const Document *document, QObject *parent):
    QAbstractListModel(parent),
    m_document(document)
{
    connect(m_document, &Document::componentPreviewPixmapChanged, [this](const dust3d::Uuid &componentId) {
        auto findIndex = this->m_componentIdToIndexMap.find(componentId);
        if (findIndex != this->m_componentIdToIndexMap.end()) {
            //dust3dDebug << "dataChanged:" << findIndex->second.row();
            emit this->dataChanged(findIndex->second, findIndex->second);
        }
    });
    connect(m_document, &Document::cleanup, [this]() {
        this->setListingComponentId(dust3d::Uuid());
        this->reload();
    });
    connect(m_document, &Document::componentChildrenChanged, [this](const dust3d::Uuid &componentId) {
        if (componentId != this->listingComponentId())
            return;
        this->reload();
    });
}

void ComponentListModel::reload()
{
    beginResetModel();
    m_componentIdToIndexMap.clear();
    const SkeletonComponent *listingComponent = m_document->findComponent(m_listingComponentId);
    if (nullptr != listingComponent) {
        for (int i = 0; i < (int)listingComponent->childrenIds.size(); ++i) {
            m_componentIdToIndexMap[listingComponent->childrenIds[i]] = createIndex(i, 0);
        }
    }
    endResetModel();
    emit layoutChanged();
}

QModelIndex ComponentListModel::componentIdToIndex(const dust3d::Uuid &componentId) const
{
    auto findIndex = m_componentIdToIndexMap.find(componentId);
    if (findIndex == m_componentIdToIndexMap.end())
        return QModelIndex();
    return findIndex->second;
}

int ComponentListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    const SkeletonComponent *listingComponent = m_document->findComponent(m_listingComponentId);
    if (nullptr == listingComponent)
        return 0;
    return (int)listingComponent->childrenIds.size();
}

int ComponentListModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return 1;
}

const SkeletonComponent *ComponentListModel::modelIndexToComponent(const QModelIndex &index) const
{
    const SkeletonComponent *listingComponent = m_document->findComponent(m_listingComponentId);
    if (nullptr == listingComponent)
        return nullptr;
    if (index.row() >= (int)listingComponent->childrenIds.size()) {
        dust3dDebug << "Component list row is out of range, size:" << listingComponent->childrenIds.size() << "row:" << index.row();
        return nullptr;
    }
    const auto &componentId = listingComponent->childrenIds[index.row()];
    const SkeletonComponent *component = m_document->findComponent(componentId);
    if (nullptr == component) {
        dust3dDebug << "Component not found:" << componentId.toString();
        return nullptr;
    }
    return component;
}

const dust3d::Uuid ComponentListModel::modelIndexToComponentId(const QModelIndex &index) const
{
    const SkeletonComponent *listingComponent = m_document->findComponent(m_listingComponentId);
    if (nullptr == listingComponent)
        return dust3d::Uuid();
    if (index.row() >= (int)listingComponent->childrenIds.size()) {
        dust3dDebug << "Component list row is out of range, size:" << listingComponent->childrenIds.size() << "row:" << index.row();
        return dust3d::Uuid();
    }
    const auto &componentId = listingComponent->childrenIds[index.row()];
    return componentId;
}

QVariant ComponentListModel::data(const QModelIndex &index, int role) const
{
    switch (role) {
    case Qt::ToolTipRole: {
            const SkeletonComponent *component = modelIndexToComponent(index);
            if (nullptr != component) {
                return component->name;
            }
        }
        break;
    case Qt::DecorationRole: {
            const SkeletonComponent *component = modelIndexToComponent(index);
            if (nullptr != component) {
                if (0 == component->previewPixmap.width()) {
                    static QPixmap s_emptyPixmap;
                    if (0 == s_emptyPixmap.width()) {
                        QImage image((int)Theme::partPreviewImageSize, (int)Theme::partPreviewImageSize, QImage::Format_ARGB32);
                        image.fill(Qt::transparent);
                        s_emptyPixmap = QPixmap::fromImage(image);
                    }
                    return s_emptyPixmap;
                }
                return component->previewPixmap;
            }
        }
        break;
    }
    return QVariant();
}

void ComponentListModel::setListingComponentId(const dust3d::Uuid &componentId)
{
    if (m_listingComponentId == componentId)
        return;
    m_listingComponentId = componentId;
    reload();
    emit listingComponentChanged(m_listingComponentId);
}

const dust3d::Uuid ComponentListModel::listingComponentId() const
{
    return m_listingComponentId;
}
