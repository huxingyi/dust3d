#include <QAbstractListModel>
#include <dust3d/base/debug.h>
#include "component_list_model.h"
#include "document.h"

ComponentListModel::ComponentListModel(const Document *document, QObject *parent):
    QAbstractListModel(parent),
    m_document(document)
{
    connect(m_document, &Document::componentPreviewPixmapChanged, [this](const dust3d::Uuid &componentId) {
        // FIXME: dont refresh the whole layout
        emit this->layoutChanged();
    });
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
    emit listingComponentChanged(m_listingComponentId);
    emit layoutChanged();
}

const dust3d::Uuid ComponentListModel::listingComponentId() const
{
    return m_listingComponentId;
}
