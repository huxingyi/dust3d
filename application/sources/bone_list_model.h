#ifndef DUST3D_APPLICATION_BONE_LIST_MODEL_H_
#define DUST3D_APPLICATION_BONE_LIST_MODEL_H_

#include "document.h"
#include <QAbstractListModel>
#include <dust3d/base/uuid.h>
#include <unordered_map>

class BoneListModel : public QAbstractListModel {
    Q_OBJECT

public:
    BoneListModel(const Document* document, QObject* parent = nullptr);
    int rowCount(const QModelIndex& parent = QModelIndex()) const;
    int columnCount(const QModelIndex& parent = QModelIndex()) const;
    QVariant data(const QModelIndex& index, int role) const;
    const Document::Bone* modelIndexToBone(const QModelIndex& index) const;
    QModelIndex boneIdToIndex(const dust3d::Uuid& boneId) const;
    const dust3d::Uuid modelIndexToBoneId(const QModelIndex& index) const;
public slots:
    void reload();

private:
    const Document* m_document = nullptr;
    std::unordered_map<dust3d::Uuid, QModelIndex> m_boneIdToIndexMap;
};

#endif
