#ifndef DUST3D_DOCUMENT_H
#define DUST3D_DOCUMENT_H
#include <QObject>
#include <QUuid>
#include <vector>
#include <map>
#include <set>
#include <deque>
#include <QImage>
#include <cmath>
#include <algorithm>
#include <QOpenGLWidget>
#include "snapshot.h"
#include "meshloader.h"
#include "meshgenerator.h"
#include "theme.h"
#include "texturegenerator.h"
#include "meshresultpostprocessor.h"
#include "bonemark.h"
#include "riggenerator.h"
#include "rigtype.h"
#include "posepreviewsgenerator.h"
#include "texturetype.h"
#include "interpolationtype.h"
#include "jointnodetree.h"
#include "skeletondocument.h"
#include "combinemode.h"
#include "preferences.h"

class MaterialPreviewsGenerator;
class MotionsGenerator;

class HistoryItem
{
public:
    uint32_t hash;
    Snapshot snapshot;
};

class Component
{
public:
    Component()
    {
    }
    Component(const QUuid &withId, const QString &linkData=QString(), const QString &linkDataType=QString())
    {
        id = withId.isNull() ? QUuid::createUuid() : withId;
        if (!linkData.isEmpty()) {
            if ("partId" == linkDataType) {
                linkToPartId = QUuid(linkData);
            }
        }
    }
    QUuid id;
    QString name;
    QUuid linkToPartId;
    QUuid parentId;
    bool expanded = true;
    CombineMode combineMode = Preferences::instance().componentCombineMode();
    bool dirty = true;
    float smoothAll = 0.0;
    float smoothSeam = 0.0;
    std::vector<QUuid> childrenIds;
    QString linkData() const
    {
        return linkToPartId.isNull() ? QString() : linkToPartId.toString();
    }
    QString linkDataType() const
    {
        return linkToPartId.isNull() ? QString() : QString("partId");
    }
    void addChild(QUuid childId)
    {
        if (m_childrenIdSet.find(childId) != m_childrenIdSet.end())
            return;
        m_childrenIdSet.insert(childId);
        childrenIds.push_back(childId);
    }
    void removeChild(QUuid childId)
    {
        if (m_childrenIdSet.find(childId) == m_childrenIdSet.end())
            return;
        m_childrenIdSet.erase(childId);
        auto findResult = std::find(childrenIds.begin(), childrenIds.end(), childId);
        if (findResult != childrenIds.end())
            childrenIds.erase(findResult);
    }
    void replaceChild(QUuid childId, QUuid newId)
    {
        if (m_childrenIdSet.find(childId) == m_childrenIdSet.end())
            return;
        if (m_childrenIdSet.find(newId) != m_childrenIdSet.end())
            return;
        m_childrenIdSet.erase(childId);
        m_childrenIdSet.insert(newId);
        auto findResult = std::find(childrenIds.begin(), childrenIds.end(), childId);
        if (findResult != childrenIds.end())
            *findResult = newId;
    }
    void moveChildUp(QUuid childId)
    {
        auto it = std::find(childrenIds.begin(), childrenIds.end(), childId);
        if (it == childrenIds.end()) {
            qDebug() << "Child not found in list:" << childId;
            return;
        }
        
        auto index = std::distance(childrenIds.begin(), it);
        if (index == 0)
            return;
        std::swap(childrenIds[index - 1], childrenIds[index]);
    }
    void moveChildDown(QUuid childId)
    {
        auto it = std::find(childrenIds.begin(), childrenIds.end(), childId);
        if (it == childrenIds.end()) {
            qDebug() << "Child not found in list:" << childId;
            return;
        }
        
        auto index = std::distance(childrenIds.begin(), it);
        if (index == (int)childrenIds.size() - 1)
            return;
        std::swap(childrenIds[index], childrenIds[index + 1]);
    }
    void moveChildToTop(QUuid childId)
    {
        auto it = std::find(childrenIds.begin(), childrenIds.end(), childId);
        if (it == childrenIds.end()) {
            qDebug() << "Child not found in list:" << childId;
            return;
        }
        
        auto index = std::distance(childrenIds.begin(), it);
        if (index == 0)
            return;
        for (int i = index; i >= 1; i--)
            std::swap(childrenIds[i - 1], childrenIds[i]);
    }
    void moveChildToBottom(QUuid childId)
    {
        auto it = std::find(childrenIds.begin(), childrenIds.end(), childId);
        if (it == childrenIds.end()) {
            qDebug() << "Child not found in list:" << childId;
            return;
        }
        
        auto index = std::distance(childrenIds.begin(), it);
        if (index == (int)childrenIds.size() - 1)
            return;
        for (int i = index; i <= (int)childrenIds.size() - 2; i++)
            std::swap(childrenIds[i], childrenIds[i + 1]);
    }
    void setSmoothAll(float toSmoothAll)
    {
        if (toSmoothAll < 0)
            toSmoothAll = 0;
        else if (toSmoothAll > 1)
            toSmoothAll = 1;
        smoothAll = toSmoothAll;
    }
    void setSmoothSeam(float toSmoothSeam)
    {
        if (toSmoothSeam < 0)
            toSmoothSeam = 0;
        else if (toSmoothSeam > 1)
            toSmoothSeam = 1;
        smoothSeam = toSmoothSeam;
    }
    bool smoothAllAdjusted() const
    {
        return fabs(smoothAll - 0.0) >= 0.01;
    }
    bool smoothSeamAdjusted() const
    {
        return fabs(smoothSeam - 0.0) >= 0.01;
    }
    bool smoothAdjusted() const
    {
        return smoothAllAdjusted() || smoothSeamAdjusted();
    }
private:
    std::set<QUuid> m_childrenIdSet;
};

class Pose
{
public:
    Pose()
    {
    }
    ~Pose()
    {
        delete m_previewMesh;
    }
    QUuid id;
    QString name;
    bool dirty = true;
    QUuid turnaroundImageId;
    std::vector<std::pair<std::map<QString, QString>, std::map<QString, std::map<QString, QString>>>> frames; // pair<attributes, parameters>
    void updatePreviewMesh(MeshLoader *previewMesh)
    {
        delete m_previewMesh;
        m_previewMesh = previewMesh;
    }
    MeshLoader *takePreviewMesh() const
    {
        if (nullptr == m_previewMesh)
            return nullptr;
        return new MeshLoader(*m_previewMesh);
    }
private:
    Q_DISABLE_COPY(Pose);
    MeshLoader *m_previewMesh = nullptr;
};

enum class MotionClipType
{
    Pose,
    Interpolation,
    Motion
};

class MotionClip
{
public:
    MotionClip()
    {
    }
    MotionClip(const QString &linkData, const QString &linkDataType)
    {
        if ("poseId" == linkDataType) {
            clipType = MotionClipType::Pose;
            linkToId = QUuid(linkData);
        } else if ("InterpolationType" == linkDataType) {
            clipType = MotionClipType::Interpolation;
            interpolationType = InterpolationTypeFromString(linkData.toUtf8().constData());
        } else if ("motionId" == linkDataType) {
            clipType = MotionClipType::Motion;
            linkToId = QUuid(linkData);
        }
    }
    QString linkDataType() const
    {
        if (MotionClipType::Pose == clipType)
            return "poseId";
        if (MotionClipType::Interpolation == clipType)
            return "InterpolationType";
        if (MotionClipType::Motion == clipType)
            return "motionId";
        return "poseId";
    }
    QString linkData() const
    {
        if (MotionClipType::Pose == clipType)
            return linkToId.toString();
        if (MotionClipType::Interpolation == clipType)
            return InterpolationTypeToString(interpolationType);
        if (MotionClipType::Motion == clipType)
            return linkToId.toString();
        return linkToId.toString();
    }
    float duration = 0.0;
    MotionClipType clipType = MotionClipType::Pose;
    QUuid linkToId;
    InterpolationType interpolationType;
};

class Motion
{
public:
    Motion()
    {
    }
    ~Motion()
    {
        releasePreviewMeshs();
    }
    QUuid id;
    QString name;
    bool dirty = true;
    std::vector<MotionClip> clips;
    std::vector<std::pair<float, JointNodeTree>> jointNodeTrees;
    void updatePreviewMeshs(std::vector<std::pair<float, MeshLoader *>> &previewMeshs)
    {
        releasePreviewMeshs();
        m_previewMeshs = previewMeshs;
        previewMeshs.clear();
    }
    MeshLoader *takePreviewMesh() const
    {
        if (m_previewMeshs.empty())
            return nullptr;
        int middle = std::max((int)m_previewMeshs.size() / 2 - 1, (int)0);
        return new MeshLoader(*m_previewMeshs[middle].second);
    }
private:
    Q_DISABLE_COPY(Motion);
    void releasePreviewMeshs()
    {
        for (const auto &item: m_previewMeshs) {
            delete item.second;
        }
        m_previewMeshs.clear();
    }
    std::vector<std::pair<float, MeshLoader *>> m_previewMeshs;
};

class MaterialMap
{
public:
    TextureType forWhat;
    QUuid imageId;
};

class MaterialLayer
{
public:
    std::vector<MaterialMap> maps;
};

class Material
{
public:
    Material()
    {
    }
    ~Material()
    {
        delete m_previewMesh;
    }
    QUuid id;
    QString name;
    bool dirty = true;
    std::vector<MaterialLayer> layers;
    void updatePreviewMesh(MeshLoader *previewMesh)
    {
        delete m_previewMesh;
        m_previewMesh = previewMesh;
    }
    MeshLoader *takePreviewMesh() const
    {
        if (nullptr == m_previewMesh)
            return nullptr;
        return new MeshLoader(*m_previewMesh);
    }
private:
    Q_DISABLE_COPY(Material);
    MeshLoader *m_previewMesh = nullptr;
};

enum class DocumentToSnapshotFor
{
    Document = 0,
    Nodes,
    Materials,
    Poses,
    Motions
};

class Document : public SkeletonDocument
{
    Q_OBJECT
signals:
    void partAdded(QUuid partId);
    void nodeAdded(QUuid nodeId);
    void edgeAdded(QUuid edgeId);
    void partRemoved(QUuid partId);
    void componentNameChanged(QUuid componentId);
    void componentChildrenChanged(QUuid componentId);
    void componentRemoved(QUuid componentId);
    void componentAdded(QUuid componentId);
    void componentExpandStateChanged(QUuid componentId);
    void componentSmoothAllChanged(QUuid componentId);
    void componentSmoothSeamChanged(QUuid componentId);
    void nodeRemoved(QUuid nodeId);
    void edgeRemoved(QUuid edgeId);
    void nodeRadiusChanged(QUuid nodeId);
    void nodeBoneMarkChanged(QUuid nodeId);
    void nodeOriginChanged(QUuid nodeId);
    void edgeChanged(QUuid edgeId);
    void partPreviewChanged(QUuid partId);
    void resultMeshChanged();
    void turnaroundChanged();
    void editModeChanged();
    void skeletonChanged();
    //void resultSkeletonChanged();
    void resultTextureChanged();
    //void resultBakedTextureChanged();
    void postProcessedResultChanged();
    void resultRigChanged();
    void rigChanged();
    void partLockStateChanged(QUuid partId);
    void partVisibleStateChanged(QUuid partId);
    void partSubdivStateChanged(QUuid partId);
    void partDisableStateChanged(QUuid partId);
    void partXmirrorStateChanged(QUuid partId);
    //void partZmirrorStateChanged(QUuid partId);
    void partBaseChanged(QUuid partId);
    void partDeformThicknessChanged(QUuid partId);
    void partDeformWidthChanged(QUuid partId);
    void partRoundStateChanged(QUuid partId);
    void partColorStateChanged(QUuid partId);
    void partCutRotationChanged(QUuid partId);
    void partCutFaceChanged(QUuid partId);
    void partMaterialIdChanged(QUuid partId);
    void partChamferStateChanged(QUuid partId);
    void partTargetChanged(QUuid partId);
    void partColorSolubilityChanged(QUuid partId);
    void componentCombineModeChanged(QUuid componentId);
    void cleanup();
    void originChanged();
    void xlockStateChanged();
    void ylockStateChanged();
    void zlockStateChanged();
    void radiusLockStateChanged();
    void checkPart(QUuid partId);
    void partChecked(QUuid partId);
    void partUnchecked(QUuid partId);
    void enableBackgroundBlur();
    void disableBackgroundBlur();
    void exportReady();
    void uncheckAll();
    void checkNode(QUuid nodeId);
    void checkEdge(QUuid edgeId);
    void optionsChanged();
    void rigTypeChanged();
    void posesChanged();
    void motionsChanged();
    void poseAdded(QUuid poseId);
    void poseRemoved(QUuid);
    void poseListChanged();
    void poseNameChanged(QUuid poseId);
    void poseFramesChanged(QUuid poseId);
    void poseTurnaroundImageIdChanged(QUuid poseId);
    void posePreviewChanged(QUuid poseId);
    void motionAdded(QUuid motionId);
    void motionRemoved(QUuid motionId);
    void motionListChanged();
    void motionNameChanged(QUuid motionId);
    void motionClipsChanged(QUuid motionId);
    void motionPreviewChanged(QUuid motionId);
    void motionResultChanged(QUuid motionId);
    void materialAdded(QUuid materialId);
    void materialRemoved(QUuid materialId);
    void materialListChanged();
    void materialNameChanged(QUuid materialId);
    void materialLayersChanged(QUuid materialId);
    void materialPreviewChanged(QUuid materialId);
    void meshGenerating();
    void postProcessing();
    void textureGenerating();
    void textureChanged();
public: // need initialize
    QImage *textureGuideImage;
    QImage *textureImage;
    QImage *textureBorderImage;
    QImage *textureColorImage;
    QImage *textureNormalImage;
    QImage *textureMetalnessRoughnessAmbientOcclusionImage;
    QImage *textureMetalnessImage;
    QImage *textureRoughnessImage;
    QImage *textureAmbientOcclusionImage;
    RigType rigType;
    bool weldEnabled;
public:
    Document();
    ~Document();
    std::map<QUuid, Component> componentMap;
    std::map<QUuid, Material> materialMap;
    std::vector<QUuid> materialIdList;
    std::map<QUuid, Pose> poseMap;
    std::vector<QUuid> poseIdList;
    std::map<QUuid, Motion> motionMap;
    std::vector<QUuid> motionIdList;
    Component rootComponent;
    QImage preview;
    bool undoable() const override;
    bool redoable() const override;
    bool hasPastableNodesInClipboard() const override;
    bool originSettled() const override;
    bool isNodeEditable(QUuid nodeId) const override;
    bool isEdgeEditable(QUuid edgeId) const override;
    void copyNodes(std::set<QUuid> nodeIdSet) const override;
    void toSnapshot(Snapshot *snapshot, const std::set<QUuid> &limitNodeIds=std::set<QUuid>(),
        DocumentToSnapshotFor forWhat=DocumentToSnapshotFor::Document,
        const std::set<QUuid> &limitPoseIds=std::set<QUuid>(),
        const std::set<QUuid> &limitMotionIds=std::set<QUuid>(),
        const std::set<QUuid> &limitMaterialIds=std::set<QUuid>()) const;
    void fromSnapshot(const Snapshot &snapshot);
    void addFromSnapshot(const Snapshot &snapshot, bool fromPaste=true);
    const Component *findComponent(QUuid componentId) const;
    const Component *findComponentParent(QUuid componentId) const;
    QUuid findComponentParentId(QUuid componentId) const;
    const Material *findMaterial(QUuid materialId) const;
    const Pose *findPose(QUuid poseId) const;
    const Motion *findMotion(QUuid motionId) const;
    MeshLoader *takeResultMesh();
    bool isMeshGenerationSucceed();
    MeshLoader *takeResultTextureMesh();
    MeshLoader *takeResultRigWeightMesh();
    const std::vector<RiggerBone> *resultRigBones() const;
    const std::map<int, RiggerVertexWeights> *resultRigWeights() const;
    void updateTurnaround(const QImage &image);
    void setSharedContextWidget(QOpenGLWidget *sharedContextWidget);
    bool hasPastableMaterialsInClipboard() const;
    bool hasPastablePosesInClipboard() const;
    bool hasPastableMotionsInClipboard() const;
    const Outcome &currentPostProcessedOutcome() const;
    bool isExportReady() const;
    bool isPostProcessResultObsolete() const;
    void collectComponentDescendantParts(QUuid componentId, std::vector<QUuid> &partIds) const;
    void collectComponentDescendantComponents(QUuid componentId, std::vector<QUuid> &componentIds) const;
    const std::vector<std::pair<QtMsgType, QString>> &resultRigMessages() const;
    const Outcome &currentRiggedOutcome() const;
    bool currentRigSucceed() const;
    bool isMeshGenerating() const;
public slots:
    void undo() override;
    void redo() override;
    void paste() override;
    void removeNode(QUuid nodeId);
    void removeEdge(QUuid edgeId);
    void removePart(QUuid partId);
    void addNodeWithId(QUuid nodeId, float x, float y, float z, float radius, QUuid fromNodeId);
    void addNode(float x, float y, float z, float radius, QUuid fromNodeId);
    void scaleNodeByAddRadius(QUuid nodeId, float amount);
    void moveNodeBy(QUuid nodeId, float x, float y, float z);
    void setNodeOrigin(QUuid nodeId, float x, float y, float z);
    void setNodeRadius(QUuid nodeId, float radius);
    void setNodeBoneMark(QUuid nodeId, BoneMark mark);
    void switchNodeXZ(QUuid nodeId);
    void moveOriginBy(float x, float y, float z);
    void addEdge(QUuid fromNodeId, QUuid toNodeId);
    void setEditMode(SkeletonDocumentEditMode mode);
    void uiReady();
    void generateMesh();
    void regenerateMesh();
    void meshReady();
    void generateTexture();
    void textureReady();
    void postProcess();
    void postProcessedMeshResultReady();
    void generateRig();
    void rigReady();
    void generatePosePreviews();
    void posePreviewsReady();
    void generateMaterialPreviews();
    void materialPreviewsReady();
    void generateMotions();
    void motionsReady();
    void setPartLockState(QUuid partId, bool locked);
    void setPartVisibleState(QUuid partId, bool visible);
    void setPartSubdivState(QUuid partId, bool subdived);
    void setPartDisableState(QUuid partId, bool disabled);
    void setPartXmirrorState(QUuid partId, bool mirrored);
    //void setPartZmirrorState(QUuid partId, bool mirrored);
    void setPartBase(QUuid partId, PartBase base);
    void setPartDeformThickness(QUuid partId, float thickness);
    void setPartDeformWidth(QUuid partId, float width);
    void setPartRoundState(QUuid partId, bool rounded);
    void setPartColorState(QUuid partId, bool hasColor, QColor color);
    void setPartCutRotation(QUuid partId, float cutRotation);
    void setPartCutFace(QUuid partId, CutFace cutFace);
    void setPartCutFaceLinkedId(QUuid partId, QUuid linkedId);
    void setPartMaterialId(QUuid partId, QUuid materialId);
    void setPartChamferState(QUuid partId, bool chamfered);
    void setPartTarget(QUuid partId, PartTarget target);
    void setPartColorSolubility(QUuid partId, float solubility);
    void setComponentCombineMode(QUuid componentId, CombineMode combineMode);
    void moveComponentUp(QUuid componentId);
    void moveComponentDown(QUuid componentId);
    void moveComponentToTop(QUuid componentId);
    void moveComponentToBottom(QUuid componentId);
    void renameComponent(QUuid componentId, QString name);
    void removeComponent(QUuid componentId);
    void addComponent(QUuid parentId);
    void moveComponent(QUuid componentId, QUuid toParentId);
    void setCurrentCanvasComponentId(QUuid componentId);
    void createNewComponentAndMoveThisIn(QUuid componentId);
    void createNewChildComponent(QUuid parentComponentId);
    void setComponentExpandState(QUuid componentId, bool expanded);
    void setComponentSmoothAll(QUuid componentId, float toSmoothAll);
    void setComponentSmoothSeam(QUuid componentId, float toSmoothSeam);
    void hideOtherComponents(QUuid componentId);
    void lockOtherComponents(QUuid componentId);
    void hideAllComponents();
    void showAllComponents();
    void collapseAllComponents();
    void expandAllComponents();
    void lockAllComponents();
    void unlockAllComponents();
    void hideDescendantComponents(QUuid componentId);
    void showDescendantComponents(QUuid componentId);
    void lockDescendantComponents(QUuid componentId);
    void unlockDescendantComponents(QUuid componentId);
    void saveSnapshot();
    void batchChangeBegin();
    void batchChangeEnd();
    void reset();
    void clearHistories();
    void silentReset();
    void breakEdge(QUuid edgeId);
    void setXlockState(bool locked);
    void setYlockState(bool locked);
    void setZlockState(bool locked);
    void setRadiusLockState(bool locked);
    void enableAllPositionRelatedLocks();
    void disableAllPositionRelatedLocks();
    void toggleSmoothNormal();
    void enableWeld(bool enabled);
    void setRigType(RigType toRigType);
    void addPose(QUuid poseId, QString name, std::vector<std::pair<std::map<QString, QString>, std::map<QString, std::map<QString, QString>>>> frames,
        QUuid turnaroundImageId);
    void removePose(QUuid poseId);
    void setPoseFrames(QUuid poseId, std::vector<std::pair<std::map<QString, QString>, std::map<QString, std::map<QString, QString>>>> frames);
    void setPoseTurnaroundImageId(QUuid poseId, QUuid imageId);
    void renamePose(QUuid poseId, QString name);
    void addMotion(QUuid motionId, QString name, std::vector<MotionClip> clips);
    void removeMotion(QUuid motionId);
    void setMotionClips(QUuid motionId, std::vector<MotionClip> clips);
    void renameMotion(QUuid motionId, QString name);
    void addMaterial(QUuid materialId, QString name, std::vector<MaterialLayer>);
    void removeMaterial(QUuid materialId);
    void setMaterialLayers(QUuid materialId, std::vector<MaterialLayer> layers);
    void renameMaterial(QUuid materialId, QString name);
    void applyPreferencePartColorChange();
    void applyPreferenceFlatShadingChange();
private:
    void splitPartByNode(std::vector<std::vector<QUuid>> *groups, QUuid nodeId);
    void joinNodeAndNeiborsToGroup(std::vector<QUuid> *group, QUuid nodeId, std::set<QUuid> *visitMap, QUuid noUseEdgeId=QUuid());
    void splitPartByEdge(std::vector<std::vector<QUuid>> *groups, QUuid edgeId);
    bool isPartReadonly(QUuid partId) const;
    QUuid createNode(QUuid nodeId, float x, float y, float z, float radius, QUuid fromNodeId);
    void settleOrigin();
    void checkExportReadyState();
    void removePartDontCareComponent(QUuid partId);
    void addPartToComponent(QUuid partId, QUuid componentId);
    bool isDescendantComponent(QUuid componentId, QUuid suspiciousId);
    void removeComponentRecursively(QUuid componentId);
    void resetDirtyFlags();
    void markAllDirty();
    void removeRigResults();
    void updateLinkedPart(QUuid oldPartId, QUuid newPartId);
private: // need initialize
    bool m_isResultMeshObsolete;
    MeshGenerator *m_meshGenerator;
    MeshLoader *m_resultMesh;
    bool m_isMeshGenerationSucceed;
    int m_batchChangeRefCount;
    Outcome *m_currentOutcome;
    bool m_isTextureObsolete;
    TextureGenerator *m_textureGenerator;
    bool m_isPostProcessResultObsolete;
    MeshResultPostProcessor *m_postProcessor;
    Outcome *m_postProcessedOutcome;
    MeshLoader *m_resultTextureMesh;
    unsigned long long m_textureImageUpdateVersion;
    QOpenGLWidget *m_sharedContextWidget;
    QUuid m_currentCanvasComponentId;
    bool m_allPositionRelatedLocksEnabled;
    bool m_smoothNormal;
    RigGenerator *m_rigGenerator;
    MeshLoader *m_resultRigWeightMesh;
    std::vector<RiggerBone> *m_resultRigBones;
    std::map<int, RiggerVertexWeights> *m_resultRigWeights;
    bool m_isRigObsolete;
    Outcome *m_riggedOutcome;
    PosePreviewsGenerator *m_posePreviewsGenerator;
    bool m_currentRigSucceed;
    MaterialPreviewsGenerator *m_materialPreviewsGenerator;
    MotionsGenerator *m_motionsGenerator;
private:
    static unsigned long m_maxSnapshot;
    std::deque<HistoryItem> m_undoItems;
    std::deque<HistoryItem> m_redoItems;
    GeneratedCacheContext m_generatedCacheContext;
    std::vector<std::pair<QtMsgType, QString>> m_resultRigMessages;
};

#endif
