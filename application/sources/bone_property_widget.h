#ifndef DUST3D_APPLICATION_BONE_PROPERTY_WIDGET_H_
#define DUST3D_APPLICATION_BONE_PROPERTY_WIDGET_H_

#include "document.h"
#include "int_number_widget.h"
#include <QWidget>
#include <dust3d/base/uuid.h>

class QLineEdit;
class QComboBox;

class BonePropertyWidget : public QWidget {
    Q_OBJECT
signals:
    void renameBone(const dust3d::Uuid& boneId, const QString& name);
    void groupOperationAdded();
    void pickBoneJoints(const dust3d::Uuid& boneId, size_t joints);
    void setBoneAttachment(const dust3d::Uuid& boneId, const dust3d::Uuid& toBoneId, int toBoneJointIndex);

public:
    BonePropertyWidget(Document* document,
        const std::vector<dust3d::Uuid>& boneIds,
        QWidget* parent = nullptr);
private slots:
    void nameEditChanged();
    void updateBoneJointComboBox();
    void synchronizeBoneAttachmentFromControls();

private:
    Document* m_document = nullptr;
    std::vector<dust3d::Uuid> m_boneIds;
    dust3d::Uuid m_boneId;
    const Document::Bone* m_bone = nullptr;
    QLineEdit* m_nameEdit = nullptr;
    QComboBox* m_parentBoneComboBox = nullptr;
    QComboBox* m_parentJointComboBox = nullptr;
    IntNumberWidget* m_jointsWidget = nullptr;
    void prepareBoneIds();
    dust3d::Uuid editingParentBoneId();
    int editingParentJointIndex();
};

#endif
