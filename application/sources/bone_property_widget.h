#ifndef DUST3D_APPLICATION_BONE_PROPERTY_WIDGET_H_
#define DUST3D_APPLICATION_BONE_PROPERTY_WIDGET_H_

#include "document.h"
#include <QWidget>
#include <dust3d/base/uuid.h>

class BonePropertyWidget : public QWidget {
    Q_OBJECT
signals:
    void groupOperationAdded();

public:
    BonePropertyWidget(Document* document,
        const std::vector<dust3d::Uuid>& boneIds,
        QWidget* parent = nullptr);
public slots:

private:
    Document* m_document = nullptr;
    std::vector<dust3d::Uuid> m_boneIds;
    dust3d::Uuid m_boneId;
    const Document::Bone* m_bone = nullptr;
    void prepareBoneIds();
};

#endif
