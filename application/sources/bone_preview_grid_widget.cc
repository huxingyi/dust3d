#include "bone_preview_grid_widget.h"
#include "bone_list_model.h"
#include <memory>

BonePreviewGridWidget::BonePreviewGridWidget(Document* document, QWidget* parent)
    : PreviewGridView(parent)
    , m_document(document)
{
    m_boneListModel = std::make_unique<BoneListModel>(m_document);
    setModel(m_boneListModel.get());

    connect(this, &BonePreviewGridWidget::doubleClicked, [this](const QModelIndex& index) {
        auto boneId = this->boneListModel()->modelIndexToBoneId(index);
        for (const auto& nodeIt : m_document->nodeMap) {
            if (nodeIt.second.boneIds.end() == nodeIt.second.boneIds.find(boneId))
                continue;
            emit this->selectNodeOnCanvas(nodeIt.first);
        }
    });
}

BoneListModel* BonePreviewGridWidget::boneListModel()
{
    return m_boneListModel.get();
}

std::vector<const Document::Bone*> BonePreviewGridWidget::getSelectedBones() const
{
    std::vector<const Document::Bone*> bones;
    QModelIndexList selected = selectionModel()->selectedIndexes();
    for (const auto& it : selected) {
        const auto& bone = m_boneListModel->modelIndexToBone(it);
        if (nullptr == bone)
            continue;
        bones.push_back(bone);
    }
    return bones;
}

std::vector<dust3d::Uuid> BonePreviewGridWidget::getSelectedBoneIds() const
{
    std::vector<dust3d::Uuid> boneIds;
    QModelIndexList selected = selectionModel()->selectedIndexes();
    for (const auto& it : selected) {
        const auto& boneId = m_boneListModel->modelIndexToBoneId(it);
        if (boneId.isNull())
            continue;
        boneIds.push_back(boneId);
    }
    return boneIds;
}
