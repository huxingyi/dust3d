#ifndef DUST3D_APPLICATION_COMPONENT_LIST_MODEL_H_
#define DUST3D_APPLICATION_COMPONENT_LIST_MODEL_H_

#include "document.h"
#include <QAbstractListModel>
#include <dust3d/base/uuid.h>
#include <unordered_map>

class ComponentListModel : public QAbstractListModel {
    Q_OBJECT
signals:
    void listingComponentChanged(const dust3d::Uuid& componentId);

public:
    ComponentListModel(const Document* document, QObject* parent = nullptr);
    int rowCount(const QModelIndex& parent = QModelIndex()) const;
    int columnCount(const QModelIndex& parent = QModelIndex()) const;
    QVariant data(const QModelIndex& index, int role) const;
    const Document::Component* modelIndexToComponent(const QModelIndex& index) const;
    QModelIndex componentIdToIndex(const dust3d::Uuid& componentId) const;
    const dust3d::Uuid modelIndexToComponentId(const QModelIndex& index) const;
    const dust3d::Uuid listingComponentId() const;
public slots:
    void setListingComponentId(const dust3d::Uuid& componentId);
    void reload();

private:
    const Document* m_document = nullptr;
    dust3d::Uuid m_listingComponentId;
    std::unordered_map<dust3d::Uuid, QModelIndex> m_componentIdToIndexMap;
};

#endif
