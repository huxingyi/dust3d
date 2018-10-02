#ifndef SKELETON_DOCUMENT_H
#define SKELETON_DOCUMENT_H
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
#include "skeletonsnapshot.h"
#include "meshloader.h"
#include "meshgenerator.h"
#include "theme.h"
#include "texturegenerator.h"
#include "meshresultpostprocessor.h"
#include "ambientocclusionbaker.h"
#include "skeletonbonemark.h"
#include "riggenerator.h"
#include "rigtype.h"
#include "posepreviewsgenerator.h"
#include "curveutil.h"

class SkeletonNode
{
public:
    SkeletonNode(const QUuid &withId=QUuid()) :
        x(0),
        y(0),
        z(0),
        radius(0),
        boneMark(SkeletonBoneMark::None)
    {
        id = withId.isNull() ? QUuid::createUuid() : withId;
    }
    void setRadius(float toRadius)
    {
        if (toRadius < 0)
            toRadius = 0.005;
        else if (toRadius > 1)
            toRadius = 1;
        radius = toRadius;
    }
    QUuid id;
    QUuid partId;
    QString name;
    float x;
    float y;
    float z;
    float radius;
    SkeletonBoneMark boneMark;
    std::vector<QUuid> edgeIds;
};

class SkeletonEdge
{
public:
    SkeletonEdge(const QUuid &withId=QUuid())
    {
        id = withId.isNull() ? QUuid::createUuid() : withId;
    }
    QUuid id;
    QUuid partId;
    QString name;
    std::vector<QUuid> nodeIds;
    QUuid neighborOf(QUuid nodeId) const
    {
        if (nodeIds.size() != 2)
            return QUuid();
        return nodeIds[0] == nodeId ? nodeIds[1] : nodeIds[0];
    }
};

class SkeletonPart
{
public:
    ~SkeletonPart()
    {
        delete m_previewMesh;
    }
    QUuid id;
    QString name;
    bool visible;
    bool locked;
    bool subdived;
    bool disabled;
    bool xMirrored;
    bool zMirrored;
    float deformThickness;
    float deformWidth;
    bool rounded;
    QColor color;
    bool hasColor;
    QUuid componentId;
    std::vector<QUuid> nodeIds;
    bool dirty;
    bool wrapped;
    SkeletonPart(const QUuid &withId=QUuid()) :
        visible(true),
        locked(false),
        subdived(false),
        disabled(false),
        xMirrored(false),
        zMirrored(false),
        deformThickness(1.0),
        deformWidth(1.0),
        rounded(false),
        color(Theme::white),
        hasColor(false),
        dirty(true),
        wrapped(false)
    {
        id = withId.isNull() ? QUuid::createUuid() : withId;
    }
    void setDeformThickness(float toThickness)
    {
        if (toThickness < 0)
            toThickness = 0;
        else if (toThickness > 2)
            toThickness = 2;
        deformThickness = toThickness;
    }
    void setDeformWidth(float toWidth)
    {
        if (toWidth < 0)
            toWidth = 0;
        else if (toWidth > 2)
            toWidth = 2;
        deformWidth = toWidth;
    }
    bool deformThicknessAdjusted() const
    {
        return fabs(deformThickness - 1.0) >= 0.01;
    }
    bool deformWidthAdjusted() const
    {
        return fabs(deformWidth - 1.0) >= 0.01;
    }
    bool deformAdjusted() const
    {
        return deformThicknessAdjusted() || deformWidthAdjusted();
    }
    bool isEditVisible() const
    {
        return visible && !disabled;
    }
    void copyAttributes(const SkeletonPart &other)
    {
        visible = other.visible;
        locked = other.locked;
        subdived = other.subdived;
        disabled = other.disabled;
        xMirrored = other.xMirrored;
        zMirrored = other.zMirrored;
        deformThickness = other.deformThickness;
        deformWidth = other.deformWidth;
        rounded = other.rounded;
        color = other.color;
        hasColor = other.hasColor;
        wrapped = other.wrapped;
        componentId = other.componentId;
        dirty = other.dirty;
    }
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
    Q_DISABLE_COPY(SkeletonPart);
    MeshLoader *m_previewMesh = nullptr;
};

enum class SkeletonProfile
{
    Unknown = 0,
    Main,
    Side
};

class SkeletonHistoryItem
{
public:
    SkeletonSnapshot snapshot;
};

enum class SkeletonDocumentEditMode
{
    Add = 0,
    Select,
    Drag,
    ZoomIn,
    ZoomOut
};

class SkeletonComponent
{
public:
    SkeletonComponent()
    {
    }
    SkeletonComponent(const QUuid &withId, const QString &linkData=QString(), const QString &linkDataType=QString())
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
    bool inverse = false;
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

class SkeletonPose
{
public:
    SkeletonPose()
    {
    }
    ~SkeletonPose()
    {
        delete m_previewMesh;
    }
    QUuid id;
    QString name;
    bool dirty = true;
    std::map<QString, std::map<QString, QString>> parameters;
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
    Q_DISABLE_COPY(SkeletonPose);
    MeshLoader *m_previewMesh = nullptr;
};

class SkeletonMotion
{
public:
    SkeletonMotion()
    {
    }
    QUuid id;
    QString name;
    bool dirty = true;
    std::vector<HermiteControlNode> controlNodes;
    std::vector<std::pair<float, QUuid>> keyframes; //std::pair<timeslot:0~1, poseId>
private:
    Q_DISABLE_COPY(SkeletonMotion);
};

enum class SkeletonDocumentToSnapshotFor
{
    Document = 0,
    Nodes,
    Poses,
    Motions
};

class SkeletonDocument : public QObject
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
    void resultSkeletonChanged();
    void resultTextureChanged();
    void resultBakedTextureChanged();
    void postProcessedResultChanged();
    void resultRigChanged();
    void rigChanged();
    void partLockStateChanged(QUuid partId);
    void partVisibleStateChanged(QUuid partId);
    void partSubdivStateChanged(QUuid partId);
    void partDisableStateChanged(QUuid partId);
    void partXmirrorStateChanged(QUuid partId);
    void partZmirrorStateChanged(QUuid partId);
    void partDeformThicknessChanged(QUuid partId);
    void partDeformWidthChanged(QUuid partId);
    void partRoundStateChanged(QUuid partId);
    void partColorStateChanged(QUuid partId);
    void partWrapStateChanged(QUuid partId);
    void componentInverseStateChanged(QUuid partId);
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
    void poseAdded(QUuid poseId);
    void poseRemoved(QUuid);
    void poseListChanged();
    void poseNameChanged(QUuid poseId);
    void poseParametersChanged(QUuid poseId);
    void posePreviewChanged(QUuid poseId);
    void motionAdded(QUuid motionId);
    void motionRemoved(QUuid motionId);
    void motionListChanged();
    void motionNameChanged(QUuid motionId);
    void motionControlNodesChanged(QUuid motionId);
    void motionKeyframesChanged(QUuid motionId);
public: // need initialize
    float originX;
    float originY;
    float originZ;
    SkeletonDocumentEditMode editMode;
    bool xlocked;
    bool ylocked;
    bool zlocked;
    bool radiusLocked;
    QImage *textureGuideImage;
    QImage *textureImage;
    QImage *textureBorderImage;
    QImage *textureAmbientOcclusionImage;
    QImage *textureColorImage;
    RigType rigType;
    bool weldEnabled;
public:
    SkeletonDocument();
    ~SkeletonDocument();
    std::map<QUuid, SkeletonPart> partMap;
    std::map<QUuid, SkeletonNode> nodeMap;
    std::map<QUuid, SkeletonEdge> edgeMap;
    std::map<QUuid, SkeletonComponent> componentMap;
    std::map<QUuid, SkeletonPose> poseMap;
    std::vector<QUuid> poseIdList;
    std::map<QUuid, SkeletonMotion> motionMap;
    std::vector<QUuid> motionIdList;
    SkeletonComponent rootComponent;
    QImage turnaround;
    QImage preview;
    void toSnapshot(SkeletonSnapshot *snapshot, const std::set<QUuid> &limitNodeIds=std::set<QUuid>(),
        SkeletonDocumentToSnapshotFor forWhat=SkeletonDocumentToSnapshotFor::Document,
        const std::set<QUuid> &limitPoseIds=std::set<QUuid>(),
        const std::set<QUuid> &limitMotionIds=std::set<QUuid>()) const;
    void fromSnapshot(const SkeletonSnapshot &snapshot);
    void addFromSnapshot(const SkeletonSnapshot &snapshot, bool fromPaste=true);
    const SkeletonNode *findNode(QUuid nodeId) const;
    const SkeletonEdge *findEdge(QUuid edgeId) const;
    const SkeletonPart *findPart(QUuid partId) const;
    const SkeletonEdge *findEdgeByNodes(QUuid firstNodeId, QUuid secondNodeId) const;
    const SkeletonComponent *findComponent(QUuid componentId) const;
    const SkeletonComponent *findComponentParent(QUuid componentId) const;
    QUuid findComponentParentId(QUuid componentId) const;
    const SkeletonPose *findPose(QUuid poseId) const;
    const SkeletonMotion *findMotion(QUuid motionId) const;
    MeshLoader *takeResultMesh();
    MeshLoader *takeResultTextureMesh();
    MeshLoader *takeResultRigWeightMesh();
    const std::vector<AutoRiggerBone> *resultRigBones() const;
    const std::map<int, AutoRiggerVertexWeights> *resultRigWeights() const;
    void updateTurnaround(const QImage &image);
    void setSharedContextWidget(QOpenGLWidget *sharedContextWidget);
    bool hasPastableNodesInClipboard() const;
    bool hasPastablePosesInClipboard() const;
    bool hasPastableMotionsInClipboard() const;
    bool undoable() const;
    bool redoable() const;
    bool isNodeEditable(QUuid nodeId) const;
    bool isEdgeEditable(QUuid edgeId) const;
    bool originSettled() const;
    const MeshResultContext &currentPostProcessedResultContext() const;
    bool isExportReady() const;
    bool isPostProcessResultObsolete() const;
    void findAllNeighbors(QUuid nodeId, std::set<QUuid> &neighbors) const;
    void collectComponentDescendantParts(QUuid componentId, std::vector<QUuid> &partIds) const;
    void collectComponentDescendantComponents(QUuid componentId, std::vector<QUuid> &componentIds) const;
    const std::vector<QString> &resultRigMissingMarkNames() const;
    const std::vector<QString> &resultRigErrorMarkNames() const;
    const MeshResultContext &currentRiggedResultContext() const;
    bool currentRigSucceed() const;
public slots:
    void removeNode(QUuid nodeId);
    void removeEdge(QUuid edgeId);
    void removePart(QUuid partId);
    void addNode(float x, float y, float z, float radius, QUuid fromNodeId);
    void scaleNodeByAddRadius(QUuid nodeId, float amount);
    void moveNodeBy(QUuid nodeId, float x, float y, float z);
    void setNodeOrigin(QUuid nodeId, float x, float y, float z);
    void setNodeRadius(QUuid nodeId, float radius);
    void setNodeBoneMark(QUuid nodeId, SkeletonBoneMark mark);
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
    void bakeAmbientOcclusionTexture();
    void ambientOcclusionTextureReady();
    void generateRig();
    void rigReady();
    void generatePosePreviews();
    void posePreviewsReady();
    void setPartLockState(QUuid partId, bool locked);
    void setPartVisibleState(QUuid partId, bool visible);
    void setPartSubdivState(QUuid partId, bool subdived);
    void setPartDisableState(QUuid partId, bool disabled);
    void setPartXmirrorState(QUuid partId, bool mirrored);
    void setPartZmirrorState(QUuid partId, bool mirrored);
    void setPartDeformThickness(QUuid partId, float thickness);
    void setPartDeformWidth(QUuid partId, float width);
    void setPartRoundState(QUuid partId, bool rounded);
    void setPartColorState(QUuid partId, bool hasColor, QColor color);
    void setPartWrapState(QUuid partId, bool wrapped);
    void setComponentInverseState(QUuid componentId, bool inverse);
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
    void undo();
    void redo();
    void paste();
    void batchChangeBegin();
    void batchChangeEnd();
    void reset();
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
    void addPose(QString name, std::map<QString, std::map<QString, QString>> parameters);
    void removePose(QUuid poseId);
    void setPoseParameters(QUuid poseId, std::map<QString, std::map<QString, QString>> parameters);
    void renamePose(QUuid poseId, QString name);
    void addMotion(QString name, std::vector<HermiteControlNode> controlNodes, std::vector<std::pair<float, QUuid>> keyframes);
    void removeMotion(QUuid motionId);
    void setMotionControlNodes(QUuid motionId, std::vector<HermiteControlNode> controlNodes);
    void setMotionKeyframes(QUuid motionId, std::vector<std::pair<float, QUuid>> keyframes);
    void renameMotion(QUuid motionId, QString name);
private:
    void splitPartByNode(std::vector<std::vector<QUuid>> *groups, QUuid nodeId);
    void joinNodeAndNeiborsToGroup(std::vector<QUuid> *group, QUuid nodeId, std::set<QUuid> *visitMap, QUuid noUseEdgeId=QUuid());
    void splitPartByEdge(std::vector<std::vector<QUuid>> *groups, QUuid edgeId);
    bool isPartReadonly(QUuid partId) const;
    QUuid createNode(float x, float y, float z, float radius, QUuid fromNodeId);
    void settleOrigin();
    void checkExportReadyState();
    void removePartDontCareComponent(QUuid partId);
    void addPartToComponent(QUuid partId, QUuid componentId);
    bool isDescendantComponent(QUuid componentId, QUuid suspiciousId);
    void removeComponentRecursively(QUuid componentId);
    void resetDirtyFlags();
    void markAllDirty();
    void removeRigResults();
private: // need initialize
    bool m_isResultMeshObsolete;
    MeshGenerator *m_meshGenerator;
    MeshLoader *m_resultMesh;
    int m_batchChangeRefCount;
    MeshResultContext *m_currentMeshResultContext;
    bool m_isTextureObsolete;
    TextureGenerator *m_textureGenerator;
    bool m_isPostProcessResultObsolete;
    MeshResultPostProcessor *m_postProcessor;
    MeshResultContext *m_postProcessedResultContext;
    MeshLoader *m_resultTextureMesh;
    unsigned long long m_textureImageUpdateVersion;
    AmbientOcclusionBaker *m_ambientOcclusionBaker;
    unsigned long long m_ambientOcclusionBakedImageUpdateVersion;
    QOpenGLWidget *m_sharedContextWidget;
    QUuid m_currentCanvasComponentId;
    bool m_allPositionRelatedLocksEnabled;
    bool m_smoothNormal;
    RigGenerator *m_rigGenerator;
    MeshLoader *m_resultRigWeightMesh;
    std::vector<AutoRiggerBone> *m_resultRigBones;
    std::map<int, AutoRiggerVertexWeights> *m_resultRigWeights;
    bool m_isRigObsolete;
    MeshResultContext *m_riggedResultContext;
    PosePreviewsGenerator *m_posePreviewsGenerator;
    bool m_currentRigSucceed;
private:
    static unsigned long m_maxSnapshot;
    std::deque<SkeletonHistoryItem> m_undoItems;
    std::deque<SkeletonHistoryItem> m_redoItems;
    GeneratedCacheContext m_generatedCacheContext;
    std::vector<QString> m_resultRigMissingMarkNames;
    std::vector<QString> m_resultRigErrorMarkNames;
};

#endif
