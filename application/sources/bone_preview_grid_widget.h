#ifndef DUST3D_APPLICATION_BONE_PREVIEW_GRID_WIDGET_H_
#define DUST3D_APPLICATION_BONE_PREVIEW_GRID_WIDGET_H_

#include "bone_list_model.h"
#include "document.h"
#include "preview_grid_view.h"
#include <QAbstractListModel>
#include <dust3d/base/uuid.h>
#include <memory>

class BonePreviewGridWidget : public PreviewGridView {
    Q_OBJECT
signals:
    void unselectAllOnCanvas();
    void selectNodeOnCanvas(const dust3d::Uuid& nodeId);

public:
    BonePreviewGridWidget(Document* document, QWidget* parent = nullptr);
    BoneListModel* boneListModel();
    std::vector<const Document::Bone*> getSelectedBones() const;
    std::vector<dust3d::Uuid> getSelectedBoneIds() const;

private:
    std::unique_ptr<BoneListModel> m_boneListModel;
    Document* m_document = nullptr;
};

#endif
