#include "bone_list_model.h"
#include <QAbstractListModel>
#include <dust3d/base/debug.h>

BoneListModel::BoneListModel(const Document* document, QObject* parent)
    : QAbstractListModel(parent)
    , m_document(document)
{
    connect(m_document, &Document::bonePreviewPixmapChanged, [this](const dust3d::Uuid& boneId) {
        auto findIndex = this->m_boneIdToIndexMap.find(boneId);
        if (findIndex != this->m_boneIdToIndexMap.end()) {
            emit this->dataChanged(findIndex->second, findIndex->second);
        }
    });
    connect(m_document, &Document::cleanup, this, &BoneListModel::reload);
    connect(m_document, &Document::boneIdListChanged, this, &BoneListModel::reload);
}

void BoneListModel::reload()
{
    beginResetModel();
    m_boneIdToIndexMap.clear();
    for (int i = 0; i < (int)m_document->boneIdList.size(); ++i) {
        m_boneIdToIndexMap[m_document->boneIdList[i]] = createIndex(i, 0);
    }
    endResetModel();
    emit layoutChanged();
}

QModelIndex BoneListModel::boneIdToIndex(const dust3d::Uuid& boneId) const
{
    auto findIndex = m_boneIdToIndexMap.find(boneId);
    if (findIndex == m_boneIdToIndexMap.end())
        return QModelIndex();
    return findIndex->second;
}

int BoneListModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return (int)m_document->boneIdList.size();
}

int BoneListModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return 1;
}

const Document::Bone* BoneListModel::modelIndexToBone(const QModelIndex& index) const
{
    if (index.row() >= (int)m_document->boneIdList.size()) {
        dust3dDebug << "Bone list row is out of range, size:" << m_document->boneIdList.size() << "row:" << index.row();
        return nullptr;
    }
    const auto& boneId = m_document->boneIdList[index.row()];
    const Document::Bone* bone = m_document->findBone(boneId);
    if (nullptr == bone) {
        dust3dDebug << "Bone not found:" << boneId.toString();
        return nullptr;
    }
    return bone;
}

const dust3d::Uuid BoneListModel::modelIndexToBoneId(const QModelIndex& index) const
{
    if (index.row() >= (int)m_document->boneIdList.size()) {
        dust3dDebug << "Bone list row is out of range, size:" << m_document->boneIdList.size() << "row:" << index.row();
        return dust3d::Uuid();
    }
    const auto& boneId = m_document->boneIdList[index.row()];
    return boneId;
}

QVariant BoneListModel::data(const QModelIndex& index, int role) const
{
    switch (role) {
    case Qt::ToolTipRole: {
        const Document::Bone* bone = modelIndexToBone(index);
        if (nullptr != bone) {
            return bone->name;
        }
    } break;
    case Qt::DecorationRole: {
        const Document::Bone* bone = modelIndexToBone(index);
        if (nullptr != bone) {
            if (0 == bone->previewPixmap.width()) {
                static QPixmap s_emptyPixmap;
                if (0 == s_emptyPixmap.width()) {
                    QImage image((int)Theme::partPreviewImageSize, (int)Theme::partPreviewImageSize, QImage::Format_ARGB32);
                    image.fill(Qt::transparent);
                    s_emptyPixmap = QPixmap::fromImage(image);
                }
                return s_emptyPixmap;
            }
            return bone->previewPixmap;
        }
    } break;
    }
    return QVariant();
}
