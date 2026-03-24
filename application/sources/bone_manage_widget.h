#ifndef DUST3D_APPLICATION_BONE_MANAGE_WIDGET_H_
#define DUST3D_APPLICATION_BONE_MANAGE_WIDGET_H_

#include "bone_structure.h"
#include "model_opengl_vertex.h"
#include "model_widget.h"
#include "rig_skeleton_mesh_generator.h"
#include "rig_skeleton_mesh_worker.h"
#include <QComboBox>
#include <QMenu>
#include <QPushButton>
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

public slots:
    void showContextMenu(const QPoint& pos);
    void onRigTypeChanged(const QString& rigType);
    void onBoneSelectionChanged();
    void assignSelectedEdgesToBone();
    void onRigSkeletonTemplateMeshReady();
    void onRigTemplateMeshGenerationThreadFinished();
    void onRigGenerationReady();
    void onRigSkinningMeshReady();
    void onRigSkinningMeshThreadFinished();

public:
    BoneManageWidget(Document* document, QWidget* parent = nullptr);
    ~BoneManageWidget();
    void setSkeletonGraphicsWidget(SkeletonGraphicsWidget* graphicsWidget);

private:
    Document* m_document = nullptr;
    SkeletonGraphicsWidget* m_skeletonGraphicsWidget = nullptr;
    QComboBox* m_rigTypeComboBox = nullptr;
    QTreeView* m_boneTreeView = nullptr;
    QStandardItemModel* m_boneTreeModel = nullptr;
    ModelWidget* m_rigTemplateModelWidget = nullptr;
    QPushButton* m_assignButton = nullptr;
    ModelWidget* m_rigSkinningModelWidget = nullptr;
    std::unique_ptr<QMenu> m_contextMenu;
    std::map<QString, RigStructure> m_rigStructures;
    QString m_selectedBoneName;
    std::unique_ptr<RigSkeletonMeshWorker> m_rigTemplateMeshWorker;
    QThread* m_rigTemplateMeshGenerationThread = nullptr;
    bool m_rigTemplateMeshObsolete = false;
    QString m_pendingRigType;
    QString m_pendingSelectedBoneName;
    std::unique_ptr<RigSkeletonMeshWorker> m_rigSkinningMeshWorker;
    QThread* m_rigSkinningMeshGenerationThread = nullptr;
    bool m_rigSkinningMeshObsolete = false;

    void loadRigStructures();
    bool loadRigFromXml(const QString& filePath);
    void updateBoneTreeView(const QString& rigType);
    void generateRigTemplateMesh(const QString& rigType, const QString& selectedBoneName = "");
    void generateRigSkinningMesh();
};

#endif
