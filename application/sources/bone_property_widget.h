#ifndef DUST3D_APPLICATION_BONE_PROPERTY_WIDGET_H_
#define DUST3D_APPLICATION_BONE_PROPERTY_WIDGET_H_

#include "document.h"
#include "int_number_widget.h"
#include <QWidget>
#include <dust3d/base/uuid.h>

class QLineEdit;

class BonePropertyWidget : public QWidget {
    Q_OBJECT
signals:
    void renameBone(const dust3d::Uuid& boneId, const QString& name);
    void groupOperationAdded();
    void pickBoneJoints();

public:
    BonePropertyWidget(Document* document,
        const std::vector<dust3d::Uuid>& boneIds,
        QWidget* parent = nullptr);
private slots:
    void nameEditChanged();

private:
    Document* m_document = nullptr;
    std::vector<dust3d::Uuid> m_boneIds;
    dust3d::Uuid m_boneId;
    const Document::Bone* m_bone = nullptr;
    QLineEdit* m_nameEdit = nullptr;
    IntNumberWidget* m_jointsWidget = nullptr;
    void prepareBoneIds();
};

#endif
