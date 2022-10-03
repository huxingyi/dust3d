#ifndef DUST3D_APPLICATION_COMPONENT_LIST_MODEL_H_
#define DUST3D_APPLICATION_COMPONENT_LIST_MODEL_H_

#include <QAbstractListModel>
#include <dust3d/base/uuid.h>

class Document;
class SkeletonComponent;

class ComponentListModel: public QAbstractListModel
{
    Q_OBJECT
public:
    ComponentListModel(const Document *document, QObject *parent=nullptr);
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role) const;
    const SkeletonComponent *modelIndexToComponent(const QModelIndex &index) const;
public slots:
    void setListingComponentId(const dust3d::Uuid &componentId);
private:
    const Document *m_document = nullptr;
    dust3d::Uuid m_listingComponentId;
};

#endif
