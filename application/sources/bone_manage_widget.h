#ifndef DUST3D_APPLICATION_BONE_MANAGE_WIDGET_H_
#define DUST3D_APPLICATION_BONE_MANAGE_WIDGET_H_

#include "bone_structure.h"
#include "model_opengl_vertex.h"
#include "model_widget.h"
#include "rig_skeleton_mesh_generator.h"
#include "rig_skeleton_mesh_worker.h"
#include <QComboBox>
#include <QGroupBox>
#include <QMenu>
#include <QProgressBar>
#include <QPushButton>
#include <QShortcut>
#include <QStandardItemModel>
#include <QThread>
#include <QTreeView>
#include <QWidget>
#include <map>
#include <memory>
#include <vector>

class Document;
class SkeletonGraphicsWidget;

class BoneManageWidget : public QWidget {
    Q_OBJECT

signals:
    void boneSelectionChanged(const QString& boneName);

public slots:
    void showContextMenu(const QPoint& pos);
    void onRigTypeChanged(const QString& rigType);
    void onBoneSelectionChanged();
    void selectBoneEdges();
    void assignSelectedEdgesToBone();
    void rigSkeletonTemplateMeshReady();
    void onRigGenerationReady();
    void rigSkinningMeshReady();

public:
    BoneManageWidget(Document* document, QWidget* parent = nullptr);
    ~BoneManageWidget();
    void setSkeletonGraphicsWidget(SkeletonGraphicsWidget* graphicsWidget);
    void setWireframeVisible(bool visible);
    void setShortcutsEnabled(bool enabled);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    Document* m_document = nullptr;
    SkeletonGraphicsWidget* m_skeletonGraphicsWidget = nullptr;
    QComboBox* m_rigTypeComboBox = nullptr;
    QTreeView* m_boneTreeView = nullptr;
    QStandardItemModel* m_boneTreeModel = nullptr;
    ModelWidget* m_rigTemplateModelWidget = nullptr;
    QProgressBar* m_assignProgressBar = nullptr;
    QPushButton* m_assignButton = nullptr;
    QPushButton* m_selectBoneEdgesButton = nullptr;
    QGroupBox* m_rigTemplateGroupBox = nullptr;
    QGroupBox* m_rigSkinningGroupBox = nullptr;
    ModelWidget* m_rigSkinningModelWidget = nullptr;
    QShortcut* m_assignShortcutReturn = nullptr;
    QShortcut* m_assignShortcutEnter = nullptr;
    std::map<QString, RigStructure> m_rigStructures;
    QString m_selectedBoneName;
    QString m_hoveredBoneName;
    std::unique_ptr<RigSkeletonMeshWorker> m_rigTemplateMeshWorker;
    bool m_rigTemplateMeshObsolete = false;
    QString m_pendingRigType;
    QString m_pendingSelectedBoneName;
    std::unique_ptr<RigSkeletonMeshWorker> m_rigSkinningMeshWorker;
    bool m_rigSkinningMeshObsolete = false;

    static constexpr int BoneNameRole = Qt::UserRole + 1;

    void loadRigStructures();
    bool loadRigFromXml(const QString& filePath);
    void updateBoneTreeView(const QString& rigType);
    void updateBoneTreeLabels();
    void clearBoneAssignments(const QString& boneName);
    void clearBoneChainAssignments(const QString& boneName);
    void generateRigTemplateMesh(const QString& rigType, const QString& selectedBoneName = "");
    void generateRigSkinningMesh();
    void updateAssignButtonState();
    void updateAssignProgressBar();
};

#endif
