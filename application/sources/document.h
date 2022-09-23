#ifndef DUST3D_APPLICATION_DOCUMENT_H_
#define DUST3D_APPLICATION_DOCUMENT_H_

#include <QObject>
#include <vector>
#include <map>
#include <set>
#include <deque>
#include <QImage>
#include <cmath>
#include <algorithm>
#include <QPolygon>
#include <dust3d/base/uuid.h>
#include <dust3d/base/snapshot.h>
#include <dust3d/base/texture_type.h>
#include <dust3d/base/combine_mode.h>
#include "model_mesh.h"
#include "monochrome_mesh.h"
#include "theme.h"
#include "skeleton_document.h"
#include "material_layer.h"

class MaterialPreviewsGenerator;
class TextureGenerator;
class MeshGenerator;
class MeshResultPostProcessor;

class HistoryItem
{
public:
    dust3d::Snapshot snapshot;
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
    dust3d::Uuid id;
    QString name;
    bool dirty = true;
    std::vector<MaterialLayer> layers;
    void updatePreviewMesh(ModelMesh *previewMesh)
    {
        delete m_previewMesh;
        m_previewMesh = previewMesh;
    }
    ModelMesh *takePreviewMesh() const
    {
        if (nullptr == m_previewMesh)
            return nullptr;
        return new ModelMesh(*m_previewMesh);
    }
private:
    Q_DISABLE_COPY(Material);
    ModelMesh *m_previewMesh = nullptr;
};

enum class DocumentToSnapshotFor
{
    Document = 0,
    Nodes,
    Materials
};

class Document : public SkeletonDocument
{
    Q_OBJECT
signals:
    void nodeCutRotationChanged(dust3d::Uuid nodeId);
    void nodeCutFaceChanged(dust3d::Uuid nodeId);
    void partPreviewChanged(dust3d::Uuid partId);
    void resultMeshChanged();
    void resultPartPreviewsChanged();
    void turnaroundChanged();
    void editModeChanged();
    void resultTextureChanged();
    void resultColorTextureChanged();
    void postProcessedResultChanged();
    void partSubdivStateChanged(dust3d::Uuid partId);
    void partXmirrorStateChanged(dust3d::Uuid partId);
    void partBaseChanged(dust3d::Uuid partId);
    void partDeformThicknessChanged(dust3d::Uuid partId);
    void partDeformWidthChanged(dust3d::Uuid partId);
    void partDeformUnifyStateChanged(dust3d::Uuid partId);
    void partRoundStateChanged(dust3d::Uuid partId);
    void partColorStateChanged(dust3d::Uuid partId);
    void partCutRotationChanged(dust3d::Uuid partId);
    void partCutFaceChanged(dust3d::Uuid partId);
    void partMaterialIdChanged(dust3d::Uuid partId);
    void partChamferStateChanged(dust3d::Uuid partId);
    void partTargetChanged(dust3d::Uuid partId);
    void partColorSolubilityChanged(dust3d::Uuid partId);
    void partMetalnessChanged(dust3d::Uuid partId);
    void partRoughnessChanged(dust3d::Uuid partId);
    void partHollowThicknessChanged(dust3d::Uuid partId);
    void partCountershadeStateChanged(dust3d::Uuid partId);
    void partSmoothStateChanged(dust3d::Uuid partId);
    void componentCombineModeChanged(dust3d::Uuid componentId);
    void cleanup();
    void checkPart(dust3d::Uuid partId);
    void partChecked(dust3d::Uuid partId);
    void partUnchecked(dust3d::Uuid partId);
    void enableBackgroundBlur();
    void disableBackgroundBlur();
    void exportReady();
    void uncheckAll();
    void checkNode(dust3d::Uuid nodeId);
    void checkEdge(dust3d::Uuid edgeId);
    void materialAdded(dust3d::Uuid materialId);
    void materialRemoved(dust3d::Uuid materialId);
    void materialListChanged();
    void materialNameChanged(dust3d::Uuid materialId);
    void materialLayersChanged(dust3d::Uuid materialId);
    void materialPreviewChanged(dust3d::Uuid materialId);
    void meshGenerating();
    void postProcessing();
    void textureGenerating();
    void textureChanged();
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
    bool weldEnabled = true;
    float brushMetalness = ModelMesh::m_defaultMetalness;
    float brushRoughness = ModelMesh::m_defaultRoughness;
public:
    Document();
    ~Document();
    std::map<dust3d::Uuid, Material> materialMap;
    std::vector<dust3d::Uuid> materialIdList;
    
    bool undoable() const override;
    bool redoable() const override;
    bool hasPastableNodesInClipboard() const override;
    bool originSettled() const override;
    bool isNodeEditable(dust3d::Uuid nodeId) const override;
    bool isEdgeEditable(dust3d::Uuid edgeId) const override;
    void copyNodes(std::set<dust3d::Uuid> nodeIdSet) const override;
    void toSnapshot(dust3d::Snapshot *snapshot, const std::set<dust3d::Uuid> &limitNodeIds=std::set<dust3d::Uuid>(),
        DocumentToSnapshotFor forWhat=DocumentToSnapshotFor::Document,
        const std::set<dust3d::Uuid> &limitMaterialIds=std::set<dust3d::Uuid>()) const;
    void fromSnapshot(const dust3d::Snapshot &snapshot);
    enum class SnapshotSource
    {
        Unknown,
        Paste,
        Import
    };
    void addFromSnapshot(const dust3d::Snapshot &snapshot, enum SnapshotSource source=SnapshotSource::Paste);
    const Material *findMaterial(dust3d::Uuid materialId) const;
    ModelMesh *takeResultMesh();
    MonochromeMesh *takeWireframeMesh();
    ModelMesh *takePaintedMesh();
    bool isMeshGenerationSucceed();
    ModelMesh *takeResultTextureMesh();
    ModelMesh *takeResultRigWeightMesh();
    void updateTurnaround(const QImage &image);
    void clearTurnaround();
    void updateTextureImage(QImage *image);
    void updateTextureNormalImage(QImage *image);
    void updateTextureMetalnessImage(QImage *image);
    void updateTextureRoughnessImage(QImage *image);
    void updateTextureAmbientOcclusionImage(QImage *image);
    bool hasPastableMaterialsInClipboard() const;
    const dust3d::Object &currentPostProcessedObject() const;
    bool isExportReady() const;
    bool isPostProcessResultObsolete() const;
    bool isMeshGenerating() const;
    bool isPostProcessing() const;
    bool isTextureGenerating() const;
    void collectCutFaceList(std::vector<QString> &cutFaces) const;
public slots:
    void undo() override;
    void redo() override;
    void paste() override;
    void setNodeCutRotation(dust3d::Uuid nodeId, float cutRotation);
    void setNodeCutFace(dust3d::Uuid nodeId, dust3d::CutFace cutFace);
    void setNodeCutFaceLinkedId(dust3d::Uuid nodeId, dust3d::Uuid linkedId);
    void clearNodeCutFaceSettings(dust3d::Uuid nodeId);
    void setEditMode(SkeletonDocumentEditMode mode);
    void uiReady();
    void generateMesh();
    void regenerateMesh();
    void meshReady();
    void generateTexture();
    void textureReady();
    void postProcess();
    void postProcessedMeshResultReady();
    void generateMaterialPreviews();
    void materialPreviewsReady();
    void setPartSubdivState(dust3d::Uuid partId, bool subdived);
    void setPartXmirrorState(dust3d::Uuid partId, bool mirrored);
    void setPartBase(dust3d::Uuid partId, dust3d::PartBase base);
    void setPartDeformThickness(dust3d::Uuid partId, float thickness);
    void setPartDeformWidth(dust3d::Uuid partId, float width);
    void setPartDeformUnified(dust3d::Uuid partId, bool unified);
    void setPartRoundState(dust3d::Uuid partId, bool rounded);
    void setPartColorState(dust3d::Uuid partId, bool hasColor, QColor color);
    void setPartCutRotation(dust3d::Uuid partId, float cutRotation);
    void setPartCutFace(dust3d::Uuid partId, dust3d::CutFace cutFace);
    void setPartCutFaceLinkedId(dust3d::Uuid partId, dust3d::Uuid linkedId);
    void setPartMaterialId(dust3d::Uuid partId, dust3d::Uuid materialId);
    void setPartChamferState(dust3d::Uuid partId, bool chamfered);
    void setPartTarget(dust3d::Uuid partId, dust3d::PartTarget target);
    void setPartColorSolubility(dust3d::Uuid partId, float solubility);
    void setPartMetalness(dust3d::Uuid partId, float metalness);
    void setPartRoughness(dust3d::Uuid partId, float roughness);
    void setPartHollowThickness(dust3d::Uuid partId, float hollowThickness);
    void setPartCountershaded(dust3d::Uuid partId, bool countershaded);
    void setPartSmoothState(dust3d::Uuid partId, bool smooth);
    void setComponentCombineMode(dust3d::Uuid componentId, dust3d::CombineMode combineMode);
    void saveSnapshot();
    void batchChangeBegin();
    void batchChangeEnd();
    void reset();
    void clearHistories();
    void silentReset();
    void toggleSmoothNormal();
    void enableWeld(bool enabled);
    void addMaterial(dust3d::Uuid materialId, QString name, std::vector<MaterialLayer>);
    void removeMaterial(dust3d::Uuid materialId);
    void setMaterialLayers(dust3d::Uuid materialId, std::vector<MaterialLayer> layers);
    void renameMaterial(dust3d::Uuid materialId, QString name);
private:
    void resolveSnapshotBoundingBox(const dust3d::Snapshot &snapshot, QRectF *mainProfile, QRectF *sideProfile);
    void settleOrigin();
    void checkExportReadyState();

    bool m_isResultMeshObsolete = false;
    MeshGenerator *m_meshGenerator = nullptr;
    ModelMesh *m_resultMesh = nullptr;
    std::unique_ptr<MonochromeMesh> m_wireframeMesh;
    bool m_isMeshGenerationSucceed = true;
    int m_batchChangeRefCount = 0;
    dust3d::Object *m_currentObject = nullptr;
    bool m_isTextureObsolete = false;
    TextureGenerator *m_textureGenerator = nullptr;
    bool m_isPostProcessResultObsolete = false;
    MeshResultPostProcessor *m_postProcessor = nullptr;
    dust3d::Object *m_postProcessedObject = new dust3d::Object;
    ModelMesh *m_resultTextureMesh = nullptr;
    unsigned long long m_textureImageUpdateVersion = 0;
    bool m_smoothNormal = false;
    MaterialPreviewsGenerator *m_materialPreviewsGenerator = nullptr;
    quint64 m_meshGenerationId = 0;
    quint64 m_nextMeshGenerationId = 0;
    void *m_generatedCacheContext = nullptr;
private:
    static unsigned long m_maxSnapshot;
    std::deque<HistoryItem> m_undoItems;
    std::deque<HistoryItem> m_redoItems;
};

#endif
