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
#include "preferences.h"
#include "paintmode.h"
#include "proceduralanimation.h"
#include "texturepainter.h"
#include "materiallayer.h"

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
    void nodeBoneMarkChanged(QUuid nodeId);
    void nodeCutRotationChanged(QUuid nodeId);
    void nodeCutFaceChanged(QUuid nodeId);
    void partPreviewChanged(QUuid partId);
    void resultMeshChanged();
    void resultPartPreviewsChanged();
    void paintedMeshChanged();
    void turnaroundChanged();
    void editModeChanged();
    void paintModeChanged();
    //void resultSkeletonChanged();
    void resultTextureChanged();
    void resultColorTextureChanged();
    //void resultBakedTextureChanged();
    void postProcessedResultChanged();
    void resultRigChanged();
    void rigChanged();
    void partSubdivStateChanged(QUuid partId);
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
    void componentCombineModeChanged(QUuid componentId);
    void cleanup();
    void cleanupScript();
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
    QImage *textureImage = nullptr;
    QByteArray *textureImageByteArray = nullptr;
    QImage *textureNormalImage = nullptr;
    QByteArray *textureNormalImageByteArray = nullptr;
    QImage *textureMetalnessImage = nullptr;
    QByteArray *textureMetalnessImageByteArray = nullptr;
    QImage *textureRoughnessImage = nullptr;
    QByteArray *textureRoughnessImageByteArray = nullptr;
    QImage *textureAmbientOcclusionImage = nullptr;
    QByteArray *textureAmbientOcclusionImageByteArray = nullptr;
    RigType rigType = RigType::None;
    bool weldEnabled = true;
    QColor brushColor = Qt::white;
    bool objectLocked = false;
    float brushMetalness = Model::m_defaultMetalness;
    float brushRoughness = Model::m_defaultRoughness;
public:
    Document();
    ~Document();
    std::map<QUuid, Material> materialMap;
    std::vector<QUuid> materialIdList;
    std::map<QUuid, Motion> motionMap;
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
    void clearTurnaround();
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
    void collectCutFaceList(std::vector<QString> &cutFaces) const;
public slots:
    void undo() override;
    void redo() override;
    void paste() override;
    void setNodeBoneMark(QUuid nodeId, BoneMark mark);
    void setNodeCutRotation(QUuid nodeId, float cutRotation);
    void setNodeCutFace(QUuid nodeId, CutFace cutFace);
    void setNodeCutFaceLinkedId(QUuid nodeId, QUuid linkedId);
    void clearNodeCutFaceSettings(QUuid nodeId);
    void setEditMode(SkeletonDocumentEditMode mode);
    void setMeshLockState(bool locked);
    void setPaintMode(PaintMode mode);
    void setMousePickRadius(float radius);
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
    void setPartSubdivState(QUuid partId, bool subdived);
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
    void saveSnapshot();
    void batchChangeBegin();
    void batchChangeEnd();
    void reset();
    void resetScript();
    void clearHistories();
    void silentReset();
    void silentResetScript();
    void setXlockState(bool locked);
    void setYlockState(bool locked);
    void setZlockState(bool locked);
    void setRadiusLockState(bool locked);
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
    void settleOrigin();
    void checkExportReadyState();
    void removeRigResults();
    bool updateDefaultVariables(const std::map<QString, std::map<QString, QString>> &defaultVariables);
private:
    bool m_isResultMeshObsolete = false;
    MeshGenerator *m_meshGenerator = nullptr;
    Model *m_resultMesh = nullptr;
    Model *m_paintedMesh = nullptr;
    std::map<QUuid, std::map<QString, QVector2D>> *m_resultMeshNodesCutFaces = nullptr;
    bool m_isMeshGenerationSucceed = true;
    int m_batchChangeRefCount = 0;
    Object *m_currentObject = nullptr;
    bool m_isTextureObsolete = false;
    TextureGenerator *m_textureGenerator = nullptr;
    bool m_isPostProcessResultObsolete = false;
    MeshResultPostProcessor *m_postProcessor = nullptr;
    Object *m_postProcessedObject = new Object;
    Model *m_resultTextureMesh = nullptr;
    unsigned long long m_textureImageUpdateVersion = 0;
    bool m_smoothNormal = !Preferences::instance().flatShading();
    RigGenerator *m_rigGenerator = nullptr;
    Model *m_resultRigWeightMesh = nullptr;
    std::vector<RigBone> *m_resultRigBones = nullptr;
    std::map<int, RigVertexWeights> *m_resultRigWeights = nullptr;
    bool m_isRigObsolete = false;
    Object *m_riggedObject = new Object;
    bool m_currentRigSucceed = false;
    MaterialPreviewsGenerator *m_materialPreviewsGenerator = nullptr;
    MotionsGenerator *m_motionsGenerator = nullptr;
    quint64 m_meshGenerationId = 0;
    quint64 m_nextMeshGenerationId = 0;
    std::map<QString, std::map<QString, QString>> m_cachedVariables;
    std::map<QString, std::map<QString, QString>> m_mergedVariables;
    ScriptRunner *m_scriptRunner = nullptr;
    bool m_isScriptResultObsolete = false;
    TexturePainter *m_texturePainter = nullptr;
    bool m_isMouseTargetResultObsolete = false;
    PaintMode m_paintMode = PaintMode::None;
    float m_mousePickRadius = 0.02f;
    GeneratedCacheContext *m_generatedCacheContext = nullptr;
    TexturePainterContext *m_texturePainterContext = nullptr;
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
