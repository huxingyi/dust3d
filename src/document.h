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
#include <QPolygon>
#include "snapshot.h"
#include "model.h"
#include "theme.h"
#include "texturegenerator.h"
#include "meshresultpostprocessor.h"
#include "bonemark.h"
#include "riggenerator.h"
#include "rigtype.h"
#include "texturetype.h"
#include "jointnodetree.h"
#include "skeletondocument.h"
#include "combinemode.h"
#include "polycount.h"
#include "preferences.h"
#include "paintmode.h"
#include "proceduralanimation.h"
#include "componentlayer.h"
#include "clothforce.h"
#include "texturepainter.h"

class MaterialPreviewsGenerator;
class MotionsGenerator;
class ScriptRunner;
class MeshGenerator;
class GeneratedCacheContext;

class HistoryItem
{
public:
    Snapshot snapshot;
};

class Component
{
public:
    static const float defaultClothStiffness;
    static const size_t defaultClothIteration;
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
    PolyCount polyCount = PolyCount::Original;
    ComponentLayer layer = ComponentLayer::Body;
    float clothStiffness = defaultClothStiffness;
    ClothForce clothForce = ClothForce::Gravitational;
    float clothOffset = 0.0f;
    size_t clothIteration = defaultClothIteration;
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
    bool clothStiffnessAdjusted() const
    {
        return fabs(clothStiffness - Component::defaultClothStiffness) >= 0.01;
    }
    bool clothIterationAdjusted() const
    {
        return clothIteration != defaultClothIteration;
    }
    bool clothForceAdjusted() const
    {
        return ClothForce::Gravitational != clothForce;
    }
    bool clothOffsetAdjusted() const
    {
        return fabs(clothOffset - 0.0) >= 0.01;
    }
private:
    std::set<QUuid> m_childrenIdSet;
};

class Motion
{
public:
    Motion()
    {
    }
    ~Motion()
    {
        delete m_previewMesh;
    }
    QUuid id;
    QString name;
    bool dirty = true;
    std::map<QString, QString> parameters;
    std::vector<std::pair<float, JointNodeTree>> jointNodeTrees;
    void updatePreviewMesh(Model *mesh)
    {
        delete m_previewMesh;
        m_previewMesh = mesh;
    }
    Model *takePreviewMesh() const
    {
        if (nullptr == m_previewMesh)
            return nullptr;
        
        return new Model(*m_previewMesh);
    }
private:
    Q_DISABLE_COPY(Motion);
    Model *m_previewMesh = nullptr;
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
    float tileScale = 1.0;
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
    void updatePreviewMesh(Model *previewMesh)
    {
        delete m_previewMesh;
        m_previewMesh = previewMesh;
    }
    Model *takePreviewMesh() const
    {
        if (nullptr == m_previewMesh)
            return nullptr;
        return new Model(*m_previewMesh);
    }
private:
    Q_DISABLE_COPY(Material);
    Model *m_previewMesh = nullptr;
};

enum class DocumentToSnapshotFor
{
    Document = 0,
    Nodes,
    Materials,
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
    void componentPolyCountChanged(QUuid componentId);
    void componentLayerChanged(QUuid componentId);
    void componentClothStiffnessChanged(QUuid componentId);
    void componentClothIterationChanged(QUuid componentId);
    void componentClothForceChanged(QUuid componentId);
    void componentClothOffsetChanged(QUuid componentId);
    void nodeRemoved(QUuid nodeId);
    void edgeRemoved(QUuid edgeId);
    void nodeRadiusChanged(QUuid nodeId);
    void nodeBoneMarkChanged(QUuid nodeId);
    void nodeColorStateChanged(QUuid nodeId);
    void nodeCutRotationChanged(QUuid nodeId);
    void nodeCutFaceChanged(QUuid nodeId);
    void nodeOriginChanged(QUuid nodeId);
    void edgeReversed(QUuid edgeId);
    void partPreviewChanged(QUuid partId);
    void resultMeshChanged();
    void resultPartPreviewsChanged();
    void paintedMeshChanged();
    void turnaroundChanged();
    void editModeChanged();
    void paintModeChanged();
    void skeletonChanged();
    //void resultSkeletonChanged();
    void resultTextureChanged();
    void resultColorTextureChanged();
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
    void partDeformUnifyStateChanged(QUuid partId);
    void partDeformMapImageIdChanged(QUuid partId);
    void partDeformMapScaleChanged(QUuid partId);
    void partRoundStateChanged(QUuid partId);
    void partColorStateChanged(QUuid partId);
    void partCutRotationChanged(QUuid partId);
    void partCutFaceChanged(QUuid partId);
    void partMaterialIdChanged(QUuid partId);
    void partChamferStateChanged(QUuid partId);
    void partTargetChanged(QUuid partId);
    void partColorSolubilityChanged(QUuid partId);
    void partMetalnessChanged(QUuid partId);
    void partRoughnessChanged(QUuid partId);
    void partHollowThicknessChanged(QUuid partId);
    void partCountershadeStateChanged(QUuid partId);
    void partSmoothStateChanged(QUuid partId);
    void partGridStateChanged(QUuid partId);
    void componentCombineModeChanged(QUuid componentId);
    void cleanup();
    void cleanupScript();
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
    void motionsChanged();
    void motionAdded(QUuid motionId);
    void motionRemoved(QUuid motionId);
    void motionNameChanged(QUuid motionId);
    void motionParametersChanged(QUuid motionId);
    void motionPreviewChanged(QUuid motionId);
    void motionResultChanged(QUuid motionId);
    void motionListChanged();
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
    void scriptChanged();
    void scriptModifiedFromExternal();
    void mergedVaraiblesChanged();
    void scriptRunning();
    void scriptErrorChanged();
    void scriptConsoleLogChanged();
    void mouseTargetChanged();
    void mousePickRadiusChanged();
    void objectLockStateChanged();
public: // need initialize
    QImage *textureImage;
    QByteArray *textureImageByteArray;
    QImage *textureNormalImage;
    QByteArray *textureNormalImageByteArray;
    QImage *textureMetalnessImage;
    QByteArray *textureMetalnessImageByteArray;
    QImage *textureRoughnessImage;
    QByteArray *textureRoughnessImageByteArray;
    QImage *textureAmbientOcclusionImage;
    QByteArray *textureAmbientOcclusionImageByteArray;
    RigType rigType;
    bool weldEnabled;
    PolyCount polyCount;
    QColor brushColor;
    bool objectLocked;
    float brushMetalness = Model::m_defaultMetalness;
    float brushRoughness = Model::m_defaultRoughness;
public:
    Document();
    ~Document();
    std::map<QUuid, Component> componentMap;
    std::map<QUuid, Material> materialMap;
    std::vector<QUuid> materialIdList;
    std::map<QUuid, Motion> motionMap;
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
        const std::set<QUuid> &limitMotionIds=std::set<QUuid>(),
        const std::set<QUuid> &limitMaterialIds=std::set<QUuid>()) const;
    void fromSnapshot(const Snapshot &snapshot);
    enum class SnapshotSource
    {
        Unknown,
        Paste,
        Import
    };
    void addFromSnapshot(const Snapshot &snapshot, enum SnapshotSource source=SnapshotSource::Paste);
    const Component *findComponent(QUuid componentId) const;
    const Component *findComponentParent(QUuid componentId) const;
    QUuid findComponentParentId(QUuid componentId) const;
    const Material *findMaterial(QUuid materialId) const;
    const Motion *findMotion(QUuid motionId) const;
    Model *takeResultMesh();
    Model *takePaintedMesh();
    bool isMeshGenerationSucceed();
    Model *takeResultTextureMesh();
    Model *takeResultRigWeightMesh();
    const std::vector<RigBone> *resultRigBones() const;
    const std::map<int, RigVertexWeights> *resultRigWeights() const;
    void updateTurnaround(const QImage &image);
    void updateTextureImage(QImage *image);
    void updateTextureNormalImage(QImage *image);
    void updateTextureMetalnessImage(QImage *image);
    void updateTextureRoughnessImage(QImage *image);
    void updateTextureAmbientOcclusionImage(QImage *image);
    bool hasPastableMaterialsInClipboard() const;
    bool hasPastableMotionsInClipboard() const;
    const Object &currentPostProcessedObject() const;
    bool isExportReady() const;
    bool isPostProcessResultObsolete() const;
    void collectComponentDescendantParts(QUuid componentId, std::vector<QUuid> &partIds) const;
    void collectComponentDescendantComponents(QUuid componentId, std::vector<QUuid> &componentIds) const;
    const std::vector<std::pair<QtMsgType, QString>> &resultRigMessages() const;
    const Object &currentRiggedObject() const;
    bool currentRigSucceed() const;
    bool isMeshGenerating() const;
    bool isPostProcessing() const;
    bool isTextureGenerating() const;
    const QString &script() const;
    const std::map<QString, std::map<QString, QString>> &variables() const;
    const QString &scriptError() const;
    const QString &scriptConsoleLog() const;
    const QVector3D &mouseTargetPosition() const;
    float mousePickRadius() const;
public slots:
    void undo() override;
    void redo() override;
    void paste() override;
    void removeNode(QUuid nodeId);
    void removeEdge(QUuid edgeId);
    void removePart(QUuid partId);
    void addPartByPolygons(const QPolygonF &mainProfile, const QPolygonF &sideProfile, const QSizeF &canvasSize);
    void addNodeWithId(QUuid nodeId, float x, float y, float z, float radius, QUuid fromNodeId);
    void addNode(float x, float y, float z, float radius, QUuid fromNodeId);
    void scaleNodeByAddRadius(QUuid nodeId, float amount);
    void moveNodeBy(QUuid nodeId, float x, float y, float z);
    void setNodeOrigin(QUuid nodeId, float x, float y, float z);
    void setNodeRadius(QUuid nodeId, float radius);
    void setNodeBoneMark(QUuid nodeId, BoneMark mark);
    void setNodeCutRotation(QUuid nodeId, float cutRotation);
    void setNodeCutFace(QUuid nodeId, CutFace cutFace);
    void setNodeCutFaceLinkedId(QUuid nodeId, QUuid linkedId);
    void clearNodeCutFaceSettings(QUuid nodeId);
    void switchNodeXZ(QUuid nodeId);
    void moveOriginBy(float x, float y, float z);
    void addEdge(QUuid fromNodeId, QUuid toNodeId);
    void setEditMode(SkeletonDocumentEditMode mode);
    void setMeshLockState(bool locked);
    void setPaintMode(PaintMode mode);
    void setMousePickRadius(float radius);
    void createSinglePartFromEdges(const std::vector<QVector3D> &nodes,
        const std::vector<std::pair<size_t, size_t>> &edges);
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
    void generateMaterialPreviews();
    void materialPreviewsReady();
    void generateMotions();
    void motionsReady();
    void pickMouseTarget(const QVector3D &nearPosition, const QVector3D &farPosition);
    void paint();
    void paintReady();
    void setPartLockState(QUuid partId, bool locked);
    void setPartVisibleState(QUuid partId, bool visible);
    void setPartSubdivState(QUuid partId, bool subdived);
    void setPartDisableState(QUuid partId, bool disabled);
    void setPartXmirrorState(QUuid partId, bool mirrored);
    //void setPartZmirrorState(QUuid partId, bool mirrored);
    void setPartBase(QUuid partId, PartBase base);
    void setPartDeformThickness(QUuid partId, float thickness);
    void setPartDeformWidth(QUuid partId, float width);
    void setPartDeformUnified(QUuid partId, bool unified);
    void setPartDeformMapImageId(QUuid partId, QUuid imageId);
    void setPartDeformMapScale(QUuid partId, float scale);
    void setPartRoundState(QUuid partId, bool rounded);
    void setPartColorState(QUuid partId, bool hasColor, QColor color);
    void setPartCutRotation(QUuid partId, float cutRotation);
    void setPartCutFace(QUuid partId, CutFace cutFace);
    void setPartCutFaceLinkedId(QUuid partId, QUuid linkedId);
    void setPartMaterialId(QUuid partId, QUuid materialId);
    void setPartChamferState(QUuid partId, bool chamfered);
    void setPartTarget(QUuid partId, PartTarget target);
    void setPartColorSolubility(QUuid partId, float solubility);
    void setPartMetalness(QUuid partId, float metalness);
    void setPartRoughness(QUuid partId, float roughness);
    void setPartHollowThickness(QUuid partId, float hollowThickness);
    void setPartCountershaded(QUuid partId, bool countershaded);
    void setPartSmoothState(QUuid partId, bool smooth);
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
    void setComponentPolyCount(QUuid componentId, PolyCount count);
    void setComponentLayer(QUuid componentId, ComponentLayer layer);
    void setComponentClothStiffness(QUuid componentId, float stiffness);
    void setComponentClothIteration(QUuid componentId, size_t iteration);
    void setComponentClothForce(QUuid componentId, ClothForce force);
    void setComponentClothOffset(QUuid componentId, float offset);
    void hideOtherComponents(QUuid componentId);
    void lockOtherComponents(QUuid componentId);
    void hideAllComponents();
    void showAllComponents();
    void showOrHideAllComponents();
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
    void resetScript();
    void clearHistories();
    void silentReset();
    void silentResetScript();
    void breakEdge(QUuid edgeId);
    void reduceNode(QUuid nodeId);
    void reverseEdge(QUuid edgeId);
    void setXlockState(bool locked);
    void setYlockState(bool locked);
    void setZlockState(bool locked);
    void setRadiusLockState(bool locked);
    void enableAllPositionRelatedLocks();
    void disableAllPositionRelatedLocks();
    void toggleSmoothNormal();
    void enableWeld(bool enabled);
    void setRigType(RigType toRigType);
    void addMotion(QUuid motionId, QString name, std::map<QString, QString> parameters);
    void removeMotion(QUuid motionId);
    void setMotionParameters(QUuid motionId, std::map<QString, QString> parameters);
    void renameMotion(QUuid motionId, QString name);
    void addMaterial(QUuid materialId, QString name, std::vector<MaterialLayer>);
    void removeMaterial(QUuid materialId);
    void setMaterialLayers(QUuid materialId, std::vector<MaterialLayer> layers);
    void renameMaterial(QUuid materialId, QString name);
    void applyPreferencePartColorChange();
    void applyPreferenceFlatShadingChange();
    void applyPreferenceTextureSizeChange();
    void applyPreferenceInterpolationChange();
    void initScript(const QString &script);
    void updateScript(const QString &script);
    void runScript();
    void scriptResultReady();
    void updateVariable(const QString &name, const std::map<QString, QString> &value);
    void updateVariableValue(const QString &name, const QString &value);
    void startPaint();
    void stopPaint();
    void setMousePickMaskNodeIds(const std::set<QUuid> &nodeIds);
    void updateObject(Object *object);
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
    //void addToolToMesh(Model *mesh);
    bool updateDefaultVariables(const std::map<QString, std::map<QString, QString>> &defaultVariables);
    void checkPartGrid(QUuid partId);
private: // need initialize
    bool m_isResultMeshObsolete;
    MeshGenerator *m_meshGenerator;
    Model *m_resultMesh;
    Model *m_paintedMesh;
    //std::map<QUuid, StrokeMeshBuilder::CutFaceTransform> *m_resultMeshCutFaceTransforms;
    std::map<QUuid, std::map<QString, QVector2D>> *m_resultMeshNodesCutFaces;
    bool m_isMeshGenerationSucceed;
    int m_batchChangeRefCount;
    Object *m_currentObject;
    bool m_isTextureObsolete;
    TextureGenerator *m_textureGenerator;
    bool m_isPostProcessResultObsolete;
    MeshResultPostProcessor *m_postProcessor;
    Object *m_postProcessedObject;
    Model *m_resultTextureMesh;
    unsigned long long m_textureImageUpdateVersion;
    QUuid m_currentCanvasComponentId;
    bool m_allPositionRelatedLocksEnabled;
    bool m_smoothNormal;
    RigGenerator *m_rigGenerator;
    Model *m_resultRigWeightMesh;
    std::vector<RigBone> *m_resultRigBones;
    std::map<int, RigVertexWeights> *m_resultRigWeights;
    bool m_isRigObsolete;
    Object *m_riggedObject;
    bool m_currentRigSucceed;
    MaterialPreviewsGenerator *m_materialPreviewsGenerator;
    MotionsGenerator *m_motionsGenerator;
    quint64 m_meshGenerationId;
    quint64 m_nextMeshGenerationId;
    std::map<QString, std::map<QString, QString>> m_cachedVariables;
    std::map<QString, std::map<QString, QString>> m_mergedVariables;
    ScriptRunner *m_scriptRunner;
    bool m_isScriptResultObsolete;
    TexturePainter *m_texturePainter;
    bool m_isMouseTargetResultObsolete;
    PaintMode m_paintMode;
    float m_mousePickRadius;
    GeneratedCacheContext *m_generatedCacheContext;
    TexturePainterContext *m_texturePainterContext;
private:
    static unsigned long m_maxSnapshot;
    std::deque<HistoryItem> m_undoItems;
    std::deque<HistoryItem> m_redoItems;
    std::vector<std::pair<QtMsgType, QString>> m_resultRigMessages;
    QVector3D m_mouseRayNear;
    QVector3D m_mouseRayFar;
    QVector3D m_mouseTargetPosition;
    QString m_scriptError;
    QString m_scriptConsoleLog;
    QString m_script;
    std::set<QUuid> m_mousePickMaskNodeIds;
};

#endif
