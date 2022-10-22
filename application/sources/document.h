#ifndef DUST3D_APPLICATION_DOCUMENT_H_
#define DUST3D_APPLICATION_DOCUMENT_H_

#include "debug.h"
#include "material_layer.h"
#include "model_mesh.h"
#include "monochrome_mesh.h"
#include "theme.h"
#include <QImage>
#include <QObject>
#include <QPolygon>
#include <algorithm>
#include <cmath>
#include <deque>
#include <dust3d/base/combine_mode.h>
#include <dust3d/base/cut_face.h>
#include <dust3d/base/part_target.h>
#include <dust3d/base/snapshot.h>
#include <dust3d/base/texture_type.h>
#include <dust3d/base/uuid.h>
#include <map>
#include <set>
#include <vector>

class SkeletonNode {
public:
    SkeletonNode(const dust3d::Uuid& withId = dust3d::Uuid())
        : radius(0)
        , cutRotation(0.0)
        , cutFace(dust3d::CutFace::Quad)
        , hasCutFaceSettings(false)
        , m_x(0)
        , m_y(0)
        , m_z(0)
    {
        id = withId.isNull() ? dust3d::Uuid::createUuid() : withId;
    }
    void setRadius(float toRadius);
    void setCutRotation(float toRotation)
    {
        if (toRotation < -1)
            toRotation = -1;
        else if (toRotation > 1)
            toRotation = 1;
        cutRotation = toRotation;
        hasCutFaceSettings = true;
    }
    void setCutFace(dust3d::CutFace face)
    {
        cutFace = face;
        cutFaceLinkedId = dust3d::Uuid();
        hasCutFaceSettings = true;
    }
    void setCutFaceLinkedId(const dust3d::Uuid& linkedId)
    {
        if (linkedId.isNull()) {
            clearCutFaceSettings();
            return;
        }
        cutFace = dust3d::CutFace::UserDefined;
        cutFaceLinkedId = linkedId;
        hasCutFaceSettings = true;
    }
    void clearCutFaceSettings()
    {
        cutFace = dust3d::CutFace::Quad;
        cutFaceLinkedId = dust3d::Uuid();
        cutRotation = 0;
        hasCutFaceSettings = false;
    }
    float getX(bool rotated = false) const
    {
        if (rotated)
            return m_y;
        return m_x;
    }
    float getY(bool rotated = false) const
    {
        if (rotated)
            return m_x;
        return m_y;
    }
    float getZ(bool rotated = false) const
    {
        (void)rotated;
        return m_z;
    }
    void setX(float x)
    {
        m_x = x;
    }
    void setY(float y)
    {
        m_y = y;
    }
    void setZ(float z)
    {
        m_z = z;
    }
    void addX(float x)
    {
        m_x += x;
    }
    void addY(float y)
    {
        m_y += y;
    }
    void addZ(float z)
    {
        m_z += z;
    }
    dust3d::Uuid id;
    dust3d::Uuid partId;
    QString name;
    float radius;
    float cutRotation;
    dust3d::CutFace cutFace;
    dust3d::Uuid cutFaceLinkedId;
    bool hasCutFaceSettings;
    std::vector<dust3d::Uuid> edgeIds;

private:
    float m_x;
    float m_y;
    float m_z;
};

class SkeletonEdge {
public:
    SkeletonEdge(const dust3d::Uuid& withId = dust3d::Uuid())
    {
        id = withId.isNull() ? dust3d::Uuid::createUuid() : withId;
    }
    dust3d::Uuid id;
    dust3d::Uuid partId;
    QString name;
    std::vector<dust3d::Uuid> nodeIds;
    dust3d::Uuid neighborOf(dust3d::Uuid nodeId) const
    {
        if (nodeIds.size() != 2)
            return dust3d::Uuid();
        return nodeIds[0] == nodeId ? nodeIds[1] : nodeIds[0];
    }
};

class SkeletonPart {
public:
    dust3d::Uuid id;
    QString name;
    bool visible;
    bool locked;
    bool subdived;
    bool disabled;
    bool xMirrored;
    float deformThickness;
    float deformWidth;
    bool deformUnified;
    bool rounded;
    bool chamfered;
    QColor color;
    bool hasColor;
    dust3d::Uuid componentId;
    std::vector<dust3d::Uuid> nodeIds;
    bool dirty;
    float cutRotation;
    dust3d::CutFace cutFace;
    dust3d::Uuid cutFaceLinkedId;
    dust3d::Uuid materialId;
    dust3d::PartTarget target;
    float colorSolubility;
    float metalness;
    float roughness;
    float hollowThickness;
    bool countershaded;
    bool smooth;
    dust3d::Uuid colorImageId;
    SkeletonPart(const dust3d::Uuid& withId = dust3d::Uuid())
        : visible(true)
        , locked(false)
        , subdived(false)
        , disabled(false)
        , xMirrored(false)
        , deformThickness(1.0)
        , deformWidth(1.0)
        , deformUnified(false)
        , rounded(false)
        , chamfered(false)
        , color(Qt::white)
        , hasColor(false)
        , dirty(true)
        , cutRotation(0.0)
        , cutFace(dust3d::CutFace::Quad)
        , target(dust3d::PartTarget::Model)
        , colorSolubility(0.0)
        , metalness(0.0)
        , roughness(1.0)
        , hollowThickness(0.0)
        , countershaded(false)
        , smooth(false)
    {
        id = withId.isNull() ? dust3d::Uuid::createUuid() : withId;
    }
    bool hasPolyFunction() const
    {
        return dust3d::PartTarget::Model == target;
    }
    bool hasSmoothFunction() const
    {
        return dust3d::PartTarget::Model == target;
    }
    bool hasSubdivFunction() const
    {
        return dust3d::PartTarget::Model == target;
    }
    bool hasRoundEndFunction() const
    {
        return dust3d::PartTarget::Model == target;
    }
    bool hasMirrorFunction() const
    {
        return dust3d::PartTarget::Model == target;
    }
    bool hasChamferFunction() const
    {
        return dust3d::PartTarget::Model == target;
    }
    bool hasRotationFunction() const
    {
        return dust3d::PartTarget::Model == target;
    }
    bool hasHollowFunction() const
    {
        return dust3d::PartTarget::Model == target;
    }
    bool hasCutFaceFunction() const
    {
        return dust3d::PartTarget::Model == target;
    }
    bool hasLayerFunction() const
    {
        return dust3d::PartTarget::Model == target;
    }
    bool hasTargetFunction() const
    {
        return true;
    }
    bool hasBaseFunction() const
    {
        return dust3d::PartTarget::Model == target;
    }
    bool hasCombineModeFunction() const
    {
        return dust3d::PartTarget::Model == target;
    }
    bool hasDeformFunction() const
    {
        return dust3d::PartTarget::Model == target;
    }
    bool hasColorFunction() const
    {
        return dust3d::PartTarget::Model == target;
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
    void setCutRotation(float toRotation)
    {
        if (toRotation < -1)
            toRotation = -1;
        else if (toRotation > 1)
            toRotation = 1;
        cutRotation = toRotation;
    }
    void setCutFace(dust3d::CutFace face)
    {
        cutFace = face;
        cutFaceLinkedId = dust3d::Uuid();
    }
    void setCutFaceLinkedId(const dust3d::Uuid& linkedId)
    {
        if (linkedId.isNull()) {
            setCutFace(dust3d::CutFace::Quad);
            return;
        }
        cutFace = dust3d::CutFace::UserDefined;
        cutFaceLinkedId = linkedId;
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
        return deformThicknessAdjusted() || deformWidthAdjusted() || deformUnified;
    }
    bool colorSolubilityAdjusted() const
    {
        return fabs(colorSolubility - 0.0) >= 0.01;
    }
    bool metalnessAdjusted() const
    {
        return fabs(metalness - 0.0) >= 0.01;
    }
    bool roughnessAdjusted() const
    {
        return fabs(roughness - 1.0) >= 0.01;
    }
    bool cutRotationAdjusted() const
    {
        return fabs(cutRotation - 0.0) >= 0.01;
    }
    bool hollowThicknessAdjusted() const
    {
        return fabs(hollowThickness - 0.0) >= 0.01;
    }
    bool cutFaceAdjusted() const
    {
        return cutFace != dust3d::CutFace::Quad;
    }
    bool cutAdjusted() const
    {
        return cutRotationAdjusted() || cutFaceAdjusted() || hollowThicknessAdjusted();
    }
    bool materialAdjusted() const
    {
        return !materialId.isNull();
    }
    bool isEditVisible() const
    {
        return visible && !disabled;
    }
    void copyAttributes(const SkeletonPart& other)
    {
        visible = other.visible;
        locked = other.locked;
        subdived = other.subdived;
        disabled = other.disabled;
        xMirrored = other.xMirrored;
        deformThickness = other.deformThickness;
        deformWidth = other.deformWidth;
        rounded = other.rounded;
        chamfered = other.chamfered;
        color = other.color;
        hasColor = other.hasColor;
        cutRotation = other.cutRotation;
        cutFace = other.cutFace;
        cutFaceLinkedId = other.cutFaceLinkedId;
        componentId = other.componentId;
        dirty = other.dirty;
        materialId = other.materialId;
        target = other.target;
        colorSolubility = other.colorSolubility;
        countershaded = other.countershaded;
        metalness = other.metalness;
        roughness = other.roughness;
        deformUnified = other.deformUnified;
        smooth = other.smooth;
        hollowThickness = other.hollowThickness;
    }

private:
    Q_DISABLE_COPY(SkeletonPart);
};

enum class DocumentEditMode {
    Add = 0,
    Select,
    Paint,
    Drag,
    ZoomIn,
    ZoomOut
};

enum class SkeletonProfile {
    Unknown = 0,
    Main,
    Side
};

class SkeletonComponent {
public:
    SkeletonComponent()
    {
    }
    SkeletonComponent(const dust3d::Uuid& withId, const QString& linkData = QString(), const QString& linkDataType = QString())
    {
        id = withId.isNull() ? dust3d::Uuid::createUuid() : withId;
        if (!linkData.isEmpty()) {
            if ("partId" == linkDataType) {
                linkToPartId = dust3d::Uuid(linkData.toUtf8().constData());
            }
        }
    }
    dust3d::Uuid id;
    QString name;
    dust3d::Uuid linkToPartId;
    dust3d::Uuid parentId;
    bool expanded = true;
    dust3d::CombineMode combineMode = dust3d::CombineMode::Normal;
    bool dirty = true;
    std::vector<dust3d::Uuid> childrenIds;
    bool isPreviewMeshObsolete = false;
    std::unique_ptr<QImage> previewImage;
    bool isPreviewImageDecorationObsolete = false;
    QPixmap previewPixmap;
    QString linkData() const
    {
        return linkToPartId.isNull() ? QString() : QString(linkToPartId.toString().c_str());
    }
    QString linkDataType() const
    {
        return linkToPartId.isNull() ? QString() : QString("partId");
    }
    void addChild(dust3d::Uuid childId)
    {
        if (m_childrenIdSet.find(childId) != m_childrenIdSet.end())
            return;
        m_childrenIdSet.insert(childId);
        childrenIds.push_back(childId);
    }
    void replaceChildWithOthers(const dust3d::Uuid& childId, const std::vector<dust3d::Uuid>& others)
    {
        if (m_childrenIdSet.find(childId) == m_childrenIdSet.end())
            return;
        m_childrenIdSet.erase(childId);
        std::vector<dust3d::Uuid> candidates;
        for (const auto& it : others) {
            if (m_childrenIdSet.find(it) == m_childrenIdSet.end()) {
                m_childrenIdSet.insert(it);
                candidates.emplace_back(it);
            }
        }
        for (size_t i = 0; i < childrenIds.size(); ++i) {
            if (childId == childrenIds[i]) {
                size_t newAddSize = candidates.size() - 1;
                if (newAddSize > 0) {
                    size_t oldSize = childrenIds.size();
                    childrenIds.resize(childrenIds.size() + newAddSize);
                    for (int j = (int)oldSize - 1; j > (int)i; --j) {
                        childrenIds[j + newAddSize] = childrenIds[j];
                    }
                }
                for (size_t k = 0; k < candidates.size(); ++k)
                    childrenIds[i + k] = candidates[k];
                break;
            }
        }
    }
    void removeChild(dust3d::Uuid childId)
    {
        if (m_childrenIdSet.find(childId) == m_childrenIdSet.end())
            return;
        m_childrenIdSet.erase(childId);
        auto findResult = std::find(childrenIds.begin(), childrenIds.end(), childId);
        if (findResult != childrenIds.end())
            childrenIds.erase(findResult);
    }
    void replaceChild(dust3d::Uuid childId, dust3d::Uuid newId)
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
    void moveChildUp(dust3d::Uuid childId)
    {
        auto it = std::find(childrenIds.begin(), childrenIds.end(), childId);
        if (it == childrenIds.end()) {
            return;
        }

        auto index = std::distance(childrenIds.begin(), it);
        if (index == 0)
            return;
        std::swap(childrenIds[index - 1], childrenIds[index]);
    }
    void moveChildDown(dust3d::Uuid childId)
    {
        auto it = std::find(childrenIds.begin(), childrenIds.end(), childId);
        if (it == childrenIds.end()) {
            return;
        }

        auto index = std::distance(childrenIds.begin(), it);
        if (index == (int)childrenIds.size() - 1)
            return;
        std::swap(childrenIds[index], childrenIds[index + 1]);
    }
    void moveChildToTop(dust3d::Uuid childId)
    {
        auto it = std::find(childrenIds.begin(), childrenIds.end(), childId);
        if (it == childrenIds.end()) {
            return;
        }

        auto index = std::distance(childrenIds.begin(), it);
        if (index == 0)
            return;
        for (int i = index; i >= 1; i--)
            std::swap(childrenIds[i - 1], childrenIds[i]);
    }
    void moveChildToBottom(dust3d::Uuid childId)
    {
        auto it = std::find(childrenIds.begin(), childrenIds.end(), childId);
        if (it == childrenIds.end()) {
            return;
        }

        auto index = std::distance(childrenIds.begin(), it);
        if (index == (int)childrenIds.size() - 1)
            return;
        for (int i = index; i <= (int)childrenIds.size() - 2; i++)
            std::swap(childrenIds[i], childrenIds[i + 1]);
    }
    void updatePreviewMesh(std::unique_ptr<ModelMesh> mesh)
    {
        m_previewMesh = std::move(mesh);
        isPreviewMeshObsolete = true;
    }
    ModelMesh* takePreviewMesh() const
    {
        if (nullptr == m_previewMesh)
            return nullptr;
        return new ModelMesh(*m_previewMesh);
    }

private:
    std::unique_ptr<ModelMesh> m_previewMesh;
    std::set<dust3d::Uuid> m_childrenIdSet;
};

class MaterialPreviewsGenerator;
class TextureGenerator;
class MeshGenerator;
class MeshResultPostProcessor;

class HistoryItem {
public:
    dust3d::Snapshot snapshot;
};

class Material {
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
    void updatePreviewMesh(ModelMesh* previewMesh)
    {
        delete m_previewMesh;
        m_previewMesh = previewMesh;
    }
    ModelMesh* takePreviewMesh() const
    {
        if (nullptr == m_previewMesh)
            return nullptr;
        return new ModelMesh(*m_previewMesh);
    }

private:
    Q_DISABLE_COPY(Material);
    ModelMesh* m_previewMesh = nullptr;
};

enum class DocumentToSnapshotFor {
    Document = 0,
    Nodes,
    Materials
};

class Document : public QObject {
    Q_OBJECT
signals:
    void nodeCutRotationChanged(dust3d::Uuid nodeId);
    void nodeCutFaceChanged(dust3d::Uuid nodeId);
    void partPreviewChanged(dust3d::Uuid partId);
    void resultMeshChanged();
    void resultComponentPreviewMeshesChanged();
    void turnaroundChanged();
    void editModeChanged();
    void resultTextureChanged();
    void postProcessedResultChanged();
    void partSubdivStateChanged(dust3d::Uuid partId);
    void partXmirrorStateChanged(dust3d::Uuid partId);
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
    void partAdded(dust3d::Uuid partId);
    void nodeAdded(dust3d::Uuid nodeId);
    void edgeAdded(dust3d::Uuid edgeId);
    void partRemoved(dust3d::Uuid partId);
    void partLockStateChanged(dust3d::Uuid partId);
    void partVisibleStateChanged(dust3d::Uuid partId);
    void partDisableStateChanged(dust3d::Uuid partId);
    void partColorImageChanged(const dust3d::Uuid& partId);
    void componentNameChanged(dust3d::Uuid componentId);
    void componentChildrenChanged(dust3d::Uuid componentId);
    void componentRemoved(dust3d::Uuid componentId);
    void componentAdded(dust3d::Uuid componentId);
    void componentExpandStateChanged(dust3d::Uuid componentId);
    void componentPreviewMeshChanged(const dust3d::Uuid& componentId);
    void componentPreviewPixmapChanged(const dust3d::Uuid& componentId);
    void nodeRemoved(dust3d::Uuid nodeId);
    void edgeRemoved(dust3d::Uuid edgeId);
    void nodeRadiusChanged(dust3d::Uuid nodeId);
    void nodeOriginChanged(dust3d::Uuid nodeId);
    void edgeReversed(dust3d::Uuid edgeId);
    void originChanged();
    void skeletonChanged();
    void optionsChanged();
    void xlockStateChanged();
    void ylockStateChanged();
    void zlockStateChanged();
    void radiusLockStateChanged();

public: // need initialize
    QImage* textureImage = nullptr;
    QByteArray* textureImageByteArray = nullptr;
    QImage* textureNormalImage = nullptr;
    QByteArray* textureNormalImageByteArray = nullptr;
    QImage* textureMetalnessImage = nullptr;
    QByteArray* textureMetalnessImageByteArray = nullptr;
    QImage* textureRoughnessImage = nullptr;
    QByteArray* textureRoughnessImageByteArray = nullptr;
    QImage* textureAmbientOcclusionImage = nullptr;
    QByteArray* textureAmbientOcclusionImageByteArray = nullptr;
    bool weldEnabled = true;
    float brushMetalness = ModelMesh::m_defaultMetalness;
    float brushRoughness = ModelMesh::m_defaultRoughness;
    DocumentEditMode editMode = DocumentEditMode::Select;
    bool xlocked = false;
    bool ylocked = false;
    bool zlocked = false;
    bool radiusLocked = false;
    QImage turnaround;
    QByteArray turnaroundPngByteArray;
    std::map<dust3d::Uuid, SkeletonPart> partMap;
    std::map<dust3d::Uuid, SkeletonNode> nodeMap;
    std::map<dust3d::Uuid, SkeletonEdge> edgeMap;
    std::map<dust3d::Uuid, SkeletonComponent> componentMap;
    SkeletonComponent rootComponent;

public:
    Document();
    ~Document();
    std::map<dust3d::Uuid, Material> materialMap;
    std::vector<dust3d::Uuid> materialIdList;

    bool undoable() const;
    bool redoable() const;
    bool hasPastableNodesInClipboard() const;
    bool originSettled() const;
    bool isNodeEditable(dust3d::Uuid nodeId) const;
    bool isEdgeEditable(dust3d::Uuid edgeId) const;
    void copyNodes(std::set<dust3d::Uuid> nodeIdSet) const;
    void toSnapshot(dust3d::Snapshot* snapshot, const std::set<dust3d::Uuid>& limitNodeIds = std::set<dust3d::Uuid>(),
        DocumentToSnapshotFor forWhat = DocumentToSnapshotFor::Document,
        const std::set<dust3d::Uuid>& limitMaterialIds = std::set<dust3d::Uuid>()) const;
    void fromSnapshot(const dust3d::Snapshot& snapshot);
    enum class SnapshotSource {
        Unknown,
        Paste,
        Import
    };
    void addFromSnapshot(const dust3d::Snapshot& snapshot, enum SnapshotSource source = SnapshotSource::Paste);
    const Material* findMaterial(dust3d::Uuid materialId) const;
    ModelMesh* takeResultMesh();
    MonochromeMesh* takeWireframeMesh();
    ModelMesh* takePaintedMesh();
    bool isMeshGenerationSucceed();
    ModelMesh* takeResultTextureMesh();
    ModelMesh* takeResultRigWeightMesh();
    void updateTurnaround(const QImage& image);
    void clearTurnaround();
    void updateTextureImage(QImage* image);
    void updateTextureNormalImage(QImage* image);
    void updateTextureMetalnessImage(QImage* image);
    void updateTextureRoughnessImage(QImage* image);
    void updateTextureAmbientOcclusionImage(QImage* image);
    bool hasPastableMaterialsInClipboard() const;
    const dust3d::Object& currentPostProcessedObject() const;
    bool isExportReady() const;
    bool isPostProcessResultObsolete() const;
    bool isMeshGenerating() const;
    bool isPostProcessing() const;
    bool isTextureGenerating() const;
    void collectCutFaceList(std::vector<QString>& cutFaces) const;
    float getOriginX(bool rotated = false) const
    {
        if (rotated)
            return m_originY;
        return m_originX;
    }
    float getOriginY(bool rotated = false) const
    {
        if (rotated)
            return m_originX;
        return m_originY;
    }
    float getOriginZ(bool rotated = false) const
    {
        (void)rotated;
        return m_originZ;
    }
    void setOriginX(float originX)
    {
        m_originX = originX;
    }
    void setOriginY(float originY)
    {
        m_originY = originY;
    }
    void setOriginZ(float originZ)
    {
        m_originZ = originZ;
    }
    void addOriginX(float originX)
    {
        m_originX += originX;
    }
    void addOriginY(float originY)
    {
        m_originY += originY;
    }
    void addOriginZ(float originZ)
    {
        m_originZ += originZ;
    }
    const SkeletonNode* findNode(dust3d::Uuid nodeId) const;
    const SkeletonEdge* findEdge(dust3d::Uuid edgeId) const;
    const SkeletonPart* findPart(dust3d::Uuid partId) const;
    const SkeletonEdge* findEdgeByNodes(dust3d::Uuid firstNodeId, dust3d::Uuid secondNodeId) const;
    void findAllNeighbors(dust3d::Uuid nodeId, std::set<dust3d::Uuid>& neighbors) const;
    bool isNodeConnectable(dust3d::Uuid nodeId) const;
    const SkeletonComponent* findComponent(dust3d::Uuid componentId) const;
    const SkeletonComponent* findComponentParent(dust3d::Uuid componentId) const;
    dust3d::Uuid findComponentParentId(dust3d::Uuid componentId) const;
    void collectComponentDescendantParts(dust3d::Uuid componentId, std::vector<dust3d::Uuid>& partIds) const;
    void collectComponentDescendantComponents(dust3d::Uuid componentId, std::vector<dust3d::Uuid>& componentIds) const;
    void setComponentPreviewMesh(const dust3d::Uuid& componentId, std::unique_ptr<ModelMesh> mesh);
    void setComponentPreviewImage(const dust3d::Uuid& componentId, std::unique_ptr<QImage> image);
    void resetDirtyFlags();
    void markAllDirty();

public slots:
    void undo();
    void redo();
    void paste();
    void setNodeCutRotation(dust3d::Uuid nodeId, float cutRotation);
    void setNodeCutFace(dust3d::Uuid nodeId, dust3d::CutFace cutFace);
    void setNodeCutFaceLinkedId(dust3d::Uuid nodeId, dust3d::Uuid linkedId);
    void clearNodeCutFaceSettings(dust3d::Uuid nodeId);
    void setEditMode(DocumentEditMode mode);
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
    void removeNode(dust3d::Uuid nodeId);
    void removeEdge(dust3d::Uuid edgeId);
    void removePart(dust3d::Uuid partId);
    void addNodeWithId(dust3d::Uuid nodeId, float x, float y, float z, float radius, dust3d::Uuid fromNodeId);
    void addNode(float x, float y, float z, float radius, dust3d::Uuid fromNodeId);
    void scaleNodeByAddRadius(dust3d::Uuid nodeId, float amount);
    void moveNodeBy(dust3d::Uuid nodeId, float x, float y, float z);
    void setNodeOrigin(dust3d::Uuid nodeId, float x, float y, float z);
    void setNodeRadius(dust3d::Uuid nodeId, float radius);
    void switchNodeXZ(dust3d::Uuid nodeId);
    void moveOriginBy(float x, float y, float z);
    void addEdge(dust3d::Uuid fromNodeId, dust3d::Uuid toNodeId);
    void moveComponentUp(dust3d::Uuid componentId);
    void moveComponentDown(dust3d::Uuid componentId);
    void moveComponentToTop(dust3d::Uuid componentId);
    void moveComponentToBottom(dust3d::Uuid componentId);
    void renameComponent(dust3d::Uuid componentId, QString name);
    void removeComponent(dust3d::Uuid componentId);
    void addComponent(dust3d::Uuid parentId);
    void moveComponent(dust3d::Uuid componentId, dust3d::Uuid toParentId);
    void setCurrentCanvasComponentId(dust3d::Uuid componentId);
    void groupComponents(const std::vector<dust3d::Uuid>& componentIds);
    void ungroupComponent(const dust3d::Uuid& componentId);
    void createNewChildComponent(dust3d::Uuid parentComponentId);
    void setComponentExpandState(dust3d::Uuid componentId, bool expanded);
    void hideOtherComponents(dust3d::Uuid componentId);
    void lockOtherComponents(dust3d::Uuid componentId);
    void hideAllComponents();
    void showAllComponents();
    void showOrHideAllComponents();
    void collapseAllComponents();
    void expandAllComponents();
    void lockAllComponents();
    void unlockAllComponents();
    void hideDescendantComponents(dust3d::Uuid componentId);
    void showDescendantComponents(dust3d::Uuid componentId);
    void lockDescendantComponents(dust3d::Uuid componentId);
    void unlockDescendantComponents(dust3d::Uuid componentId);
    void setComponentPreviewPixmap(const dust3d::Uuid& componentId, const QPixmap& pixmap);
    void setPartLockState(dust3d::Uuid partId, bool locked);
    void setPartVisibleState(dust3d::Uuid partId, bool visible);
    void setPartDisableState(dust3d::Uuid partId, bool disabled);
    void setPartColorImage(const dust3d::Uuid& partId, const dust3d::Uuid& imageId);
    void enableAllPositionRelatedLocks();
    void disableAllPositionRelatedLocks();
    bool isPartReadonly(dust3d::Uuid partId) const;
    void breakEdge(dust3d::Uuid edgeId);
    void reduceNode(dust3d::Uuid nodeId);
    void reverseEdge(dust3d::Uuid edgeId);
    void setXlockState(bool locked);
    void setYlockState(bool locked);
    void setZlockState(bool locked);
    void setRadiusLockState(bool locked);

private:
    void resolveSnapshotBoundingBox(const dust3d::Snapshot& snapshot, QRectF* mainProfile, QRectF* sideProfile);
    void settleOrigin();
    void checkExportReadyState();
    void splitPartByNode(std::vector<std::vector<dust3d::Uuid>>* groups, dust3d::Uuid nodeId);
    void joinNodeAndNeiborsToGroup(std::vector<dust3d::Uuid>* group, dust3d::Uuid nodeId, std::set<dust3d::Uuid>* visitMap, dust3d::Uuid noUseEdgeId = dust3d::Uuid());
    void splitPartByEdge(std::vector<std::vector<dust3d::Uuid>>* groups, dust3d::Uuid edgeId);
    void removePartDontCareComponent(dust3d::Uuid partId);
    void addPartToComponent(dust3d::Uuid partId, dust3d::Uuid componentId);
    bool isDescendantComponent(dust3d::Uuid componentId, dust3d::Uuid suspiciousId);
    void removeComponentRecursively(dust3d::Uuid componentId);
    void updateLinkedPart(dust3d::Uuid oldPartId, dust3d::Uuid newPartId);
    dust3d::Uuid createNode(dust3d::Uuid nodeId, float x, float y, float z, float radius, dust3d::Uuid fromNodeId);

    bool m_isResultMeshObsolete = false;
    MeshGenerator* m_meshGenerator = nullptr;
    ModelMesh* m_resultMesh = nullptr;
    std::unique_ptr<MonochromeMesh> m_wireframeMesh;
    bool m_isMeshGenerationSucceed = true;
    int m_batchChangeRefCount = 0;
    dust3d::Object* m_currentObject = nullptr;
    bool m_isTextureObsolete = false;
    TextureGenerator* m_textureGenerator = nullptr;
    bool m_isPostProcessResultObsolete = false;
    MeshResultPostProcessor* m_postProcessor = nullptr;
    dust3d::Object* m_postProcessedObject = new dust3d::Object;
    ModelMesh* m_resultTextureMesh = nullptr;
    unsigned long long m_textureImageUpdateVersion = 0;
    bool m_smoothNormal = false;
    MaterialPreviewsGenerator* m_materialPreviewsGenerator = nullptr;
    quint64 m_meshGenerationId = 0;
    quint64 m_nextMeshGenerationId = 0;
    void* m_generatedCacheContext = nullptr;
    float m_originX = 0;
    float m_originY = 0;
    float m_originZ = 0;
    dust3d::Uuid m_currentCanvasComponentId;
    bool m_allPositionRelatedLocksEnabled = true;

private:
    static unsigned long m_maxSnapshot;
    std::deque<HistoryItem> m_undoItems;
    std::deque<HistoryItem> m_redoItems;
};

#endif
