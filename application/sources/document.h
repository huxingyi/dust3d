#ifndef DUST3D_APPLICATION_DOCUMENT_H_
#define DUST3D_APPLICATION_DOCUMENT_H_

#include "debug.h"
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

class UvMapGenerator;
class MeshGenerator;

class Document : public QObject {
    Q_OBJECT
public:
    enum class EditMode {
        Add = 0,
        Select,
        Pick
    };

    enum class SnapshotFor {
        None = 0,
        Nodes = 0x00000001,
        Document = (SnapshotFor::Nodes)
    };

    class HistoryItem {
    public:
        dust3d::Snapshot snapshot;
    };

    enum class Profile {
        Unknown = 0,
        Main,
        Side
    };

    class Node {
    public:
        Node(const dust3d::Uuid& withId = dust3d::Uuid());
        void setRadius(float toRadius);
        void setCutRotation(float toRotation);
        void setCutFace(dust3d::CutFace face);
        void setCutFaceLinkedId(const dust3d::Uuid& linkedId);
        void clearCutFaceSettings();
        float getX(bool rotated = false) const;
        float getY(bool rotated = false) const;
        float getZ(bool rotated = false) const;
        void setX(float x);
        void setY(float y);
        void setZ(float z);
        void addX(float x);
        void addY(float y);
        void addZ(float z);
        dust3d::Uuid id;
        dust3d::Uuid partId;
        QString name;
        float radius = 0.0;
        float cutRotation = 0.0;
        dust3d::CutFace cutFace = dust3d::CutFace::Quad;
        dust3d::Uuid cutFaceLinkedId;
        bool hasCutFaceSettings = false;
        std::vector<dust3d::Uuid> edgeIds;

    private:
        float m_x = 0.0;
        float m_y = 0.0;
        float m_z = 0.0;
    };

    class Edge {
    public:
        Edge(const dust3d::Uuid& withId = dust3d::Uuid());
        dust3d::Uuid id;
        dust3d::Uuid partId;
        QString name;
        std::vector<dust3d::Uuid> nodeIds;
        dust3d::Uuid neighborOf(dust3d::Uuid nodeId) const;
    };

    class Part {
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
        dust3d::Uuid componentId;
        std::vector<dust3d::Uuid> nodeIds;
        bool dirty;
        float cutRotation;
        dust3d::CutFace cutFace;
        dust3d::Uuid cutFaceLinkedId;
        dust3d::PartTarget target;
        float metalness;
        float roughness;
        float hollowThickness;
        bool countershaded;
        float smoothCutoffDegrees;
        Part(const dust3d::Uuid& withId = dust3d::Uuid());
        bool hasPolyFunction() const;
        bool hasSmoothFunction() const;
        bool hasSubdivFunction() const;
        bool hasRoundEndFunction() const;
        bool hasMirrorFunction() const;
        bool hasChamferFunction() const;
        bool hasRotationFunction() const;
        bool hasHollowFunction() const;
        bool hasCutFaceFunction() const;
        bool hasLayerFunction() const;
        bool hasTargetFunction() const;
        bool hasBaseFunction() const;
        bool hasCombineModeFunction() const;
        bool hasDeformFunction() const;
        void setDeformThickness(float toThickness);
        void setDeformWidth(float toWidth);
        void setCutRotation(float toRotation);
        void setCutFace(dust3d::CutFace face);
        void setCutFaceLinkedId(const dust3d::Uuid& linkedId);
        bool deformThicknessAdjusted() const;
        bool deformWidthAdjusted() const;
        bool deformAdjusted() const;
        bool metalnessAdjusted() const;
        bool roughnessAdjusted() const;
        bool cutRotationAdjusted() const;
        bool hollowThicknessAdjusted() const;
        bool cutFaceAdjusted() const;
        bool cutAdjusted() const;
        bool isEditVisible() const;
        void copyAttributes(const Part& other);

    private:
        Q_DISABLE_COPY(Part);
    };

    class Component {
    public:
        Component();
        Component(const dust3d::Uuid& withId, const QString& linkData = QString(), const QString& linkDataType = QString());
        QString linkData() const;
        QString linkDataType() const;
        void addChild(dust3d::Uuid childId);
        void replaceChildWithOthers(const dust3d::Uuid& childId, const std::vector<dust3d::Uuid>& others);
        void removeChild(dust3d::Uuid childId);
        void replaceChild(dust3d::Uuid childId, dust3d::Uuid newId);
        void moveChildUp(dust3d::Uuid childId);
        void moveChildDown(dust3d::Uuid childId);
        void moveChildToTop(dust3d::Uuid childId);
        void moveChildToBottom(dust3d::Uuid childId);
        void updatePreviewMesh(std::unique_ptr<ModelMesh> mesh);
        ModelMesh* takePreviewMesh() const;
        dust3d::Uuid id;
        QString name;
        dust3d::Uuid linkToPartId;
        dust3d::Uuid parentId;
        QColor color = Qt::white;
        bool hasColor = false;
        dust3d::Uuid colorImageId;
        bool expanded = true;
        dust3d::CombineMode combineMode = dust3d::CombineMode::Normal;
        bool closed = false;
        bool dirty = true;
        std::vector<dust3d::Uuid> childrenIds;
        bool isPreviewMeshObsolete = false;
        std::unique_ptr<QImage> previewImage;
        bool isPreviewImageDecorationObsolete = false;
        QPixmap previewPixmap;

    private:
        std::unique_ptr<ModelMesh> m_previewMesh;
        std::set<dust3d::Uuid> m_childrenIdSet;
    };

signals:
    void nodeCutRotationChanged(dust3d::Uuid nodeId);
    void nodeCutFaceChanged(dust3d::Uuid nodeId);
    void partPreviewChanged(dust3d::Uuid partId);
    void resultMeshChanged();
    void resultComponentPreviewMeshesChanged();
    void turnaroundChanged();
    void editModeChanged();
    void resultTextureChanged();
    void partSubdivStateChanged(dust3d::Uuid partId);
    void partXmirrorStateChanged(dust3d::Uuid partId);
    void partDeformThicknessChanged(dust3d::Uuid partId);
    void partDeformWidthChanged(dust3d::Uuid partId);
    void partDeformUnifyStateChanged(dust3d::Uuid partId);
    void partRoundStateChanged(dust3d::Uuid partId);
    void componentColorStateChanged(const dust3d::Uuid& componentId);
    void partCutRotationChanged(dust3d::Uuid partId);
    void partCutFaceChanged(dust3d::Uuid partId);
    void partChamferStateChanged(dust3d::Uuid partId);
    void partTargetChanged(dust3d::Uuid partId);
    void partMetalnessChanged(dust3d::Uuid partId);
    void partRoughnessChanged(dust3d::Uuid partId);
    void partHollowThicknessChanged(dust3d::Uuid partId);
    void partCountershadeStateChanged(dust3d::Uuid partId);
    void partSmoothCutoffDegreesChanged(dust3d::Uuid partId);
    void componentCombineModeChanged(dust3d::Uuid componentId);
    void cleanup();
    void clearSelections();
    void checkPart(dust3d::Uuid partId);
    void partChecked(dust3d::Uuid partId);
    void partUnchecked(dust3d::Uuid partId);
    void enableBackgroundBlur();
    void disableBackgroundBlur();
    void exportReady();
    void uncheckAll();
    void checkNode(dust3d::Uuid nodeId);
    void checkEdge(dust3d::Uuid edgeId);
    void meshGenerating();
    void textureGenerating();
    void textureChanged();
    void partAdded(dust3d::Uuid partId);
    void nodeAdded(dust3d::Uuid nodeId);
    void edgeAdded(dust3d::Uuid edgeId);
    void partRemoved(dust3d::Uuid partId);
    void partLockStateChanged(dust3d::Uuid partId);
    void partVisibleStateChanged(dust3d::Uuid partId);
    void partDisableStateChanged(dust3d::Uuid partId);
    void componentColorImageChanged(const dust3d::Uuid& componentId);
    void componentNameChanged(dust3d::Uuid componentId);
    void componentChildrenChanged(dust3d::Uuid componentId);
    void componentRemoved(dust3d::Uuid componentId);
    void componentAdded(dust3d::Uuid componentId);
    void componentExpandStateChanged(dust3d::Uuid componentId);
    void componentPreviewMeshChanged(const dust3d::Uuid& componentId);
    void componentPreviewPixmapChanged(const dust3d::Uuid& componentId);
    void componentCloseStateChanged(const dust3d::Uuid& componentId);
    void nodeRemoved(dust3d::Uuid nodeId);
    void edgeRemoved(dust3d::Uuid edgeId);
    void nodeRadiusChanged(dust3d::Uuid nodeId);
    void nodeOriginChanged(dust3d::Uuid nodeId);
    void edgeReversed(dust3d::Uuid edgeId);
    void edgeNodeChanged(const dust3d::Uuid& edgeId);
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
    EditMode editMode = EditMode::Select;
    bool xlocked = false;
    bool ylocked = false;
    bool zlocked = false;
    bool radiusLocked = false;
    QImage turnaround;
    QByteArray turnaroundPngByteArray;
    std::map<dust3d::Uuid, Part> partMap;
    std::map<dust3d::Uuid, Node> nodeMap;
    std::map<dust3d::Uuid, Edge> edgeMap;
    std::map<dust3d::Uuid, Component> componentMap;
    Component rootComponent;

public:
    Document();
    ~Document();
    bool undoable() const;
    bool redoable() const;
    bool hasPastableNodesInClipboard() const;
    bool originSettled() const;
    bool isNodeEditable(dust3d::Uuid nodeId) const;
    bool isEdgeEditable(dust3d::Uuid edgeId) const;
    void copyNodes(std::set<dust3d::Uuid> nodeIdSet) const;
    void toSnapshot(dust3d::Snapshot* snapshot, const std::set<dust3d::Uuid>& limitNodeIds = std::set<dust3d::Uuid>(),
        SnapshotFor forWhat = SnapshotFor::Document) const;
    void fromSnapshot(const dust3d::Snapshot& snapshot);
    enum class SnapshotSource {
        Unknown,
        Paste,
        Import
    };
    void addFromSnapshot(const dust3d::Snapshot& snapshot, enum SnapshotSource source = SnapshotSource::Paste);
    ModelMesh* takeResultMesh();
    quint64 resultMeshId();
    MonochromeMesh* takeWireframeMesh();
    ModelMesh* takePaintedMesh();
    bool isMeshGenerationSucceed();
    ModelMesh* takeResultTextureMesh();
    quint64 resultTextureMeshId();
    quint64 resultTextureImageUpdateVersion();
    void updateTurnaround(const QImage& image);
    void clearTurnaround();
    void updateTextureImage(QImage* image);
    void updateTextureNormalImage(QImage* image);
    void updateTextureMetalnessImage(QImage* image);
    void updateTextureRoughnessImage(QImage* image);
    void updateTextureAmbientOcclusionImage(QImage* image);
    const dust3d::Object& currentUvMappedObject() const;
    bool isExportReady() const;
    bool isMeshGenerating() const;
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
    const Node* findNode(dust3d::Uuid nodeId) const;
    const Edge* findEdge(dust3d::Uuid edgeId) const;
    const Part* findPart(dust3d::Uuid partId) const;
    const Edge* findEdgeByNodes(dust3d::Uuid firstNodeId, dust3d::Uuid secondNodeId) const;
    void findAllNeighbors(dust3d::Uuid nodeId, std::set<dust3d::Uuid>& neighbors) const;
    bool isNodeConnectable(dust3d::Uuid nodeId) const;
    const Component* findComponent(dust3d::Uuid componentId) const;
    const Component* findComponentParent(dust3d::Uuid componentId) const;
    dust3d::Uuid findComponentParentId(dust3d::Uuid componentId) const;
    void collectComponentDescendantParts(dust3d::Uuid componentId, std::vector<dust3d::Uuid>& partIds) const;
    dust3d::Uuid componentToLinkedPartId(const dust3d::Uuid& componentId);
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
    void setEditMode(EditMode mode);
    void uiReady();
    void generateMesh();
    void regenerateMesh();
    void meshReady();
    void generateTexture();
    void textureReady();
    void setPartSubdivState(dust3d::Uuid partId, bool subdived);
    void setPartXmirrorState(dust3d::Uuid partId, bool mirrored);
    void setPartDeformThickness(dust3d::Uuid partId, float thickness);
    void setPartDeformWidth(dust3d::Uuid partId, float width);
    void setPartDeformUnified(dust3d::Uuid partId, bool unified);
    void setPartRoundState(dust3d::Uuid partId, bool rounded);
    void setPartCutRotation(dust3d::Uuid partId, float cutRotation);
    void setPartCutFace(dust3d::Uuid partId, dust3d::CutFace cutFace);
    void setPartCutFaceLinkedId(dust3d::Uuid partId, dust3d::Uuid linkedId);
    void setPartChamferState(dust3d::Uuid partId, bool chamfered);
    void setPartTarget(dust3d::Uuid partId, dust3d::PartTarget target);
    void setPartMetalness(dust3d::Uuid partId, float metalness);
    void setPartRoughness(dust3d::Uuid partId, float roughness);
    void setPartHollowThickness(dust3d::Uuid partId, float hollowThickness);
    void setPartCountershaded(dust3d::Uuid partId, bool countershaded);
    void setPartSmoothCutoffDegrees(dust3d::Uuid partId, float degrees);
    void setComponentCombineMode(dust3d::Uuid componentId, dust3d::CombineMode combineMode);
    void saveSnapshot();
    void batchChangeBegin();
    void batchChangeEnd();
    void reset();
    void clearHistories();
    void silentReset();
    void toggleSmoothNormal();
    void enableWeld(bool enabled);
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
    void setComponentCloseState(const dust3d::Uuid& componentId, bool closed);
    void hideOtherComponents(dust3d::Uuid componentId);
    void lockOtherComponents(dust3d::Uuid componentId);
    void setComponentColorState(const dust3d::Uuid& componentId, bool hasColor, QColor color);
    void setComponentColorImage(const dust3d::Uuid& componentId, const dust3d::Uuid& imageId);
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
    UvMapGenerator* m_textureGenerator = nullptr;
    std::unique_ptr<dust3d::Object> m_uvMappedObject = std::make_unique<dust3d::Object>();
    ModelMesh* m_resultTextureMesh = nullptr;
    quint64 m_textureImageUpdateVersion = 0;
    bool m_smoothNormal = false;
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
