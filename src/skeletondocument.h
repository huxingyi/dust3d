#ifndef DUST3D_SKELETON_DOCUMENT_H
#define DUST3D_SKELETON_DOCUMENT_H
#include <QUuid>
#include <QObject>
#include <QString>
#include <cmath>
#include <QImage>
#include <QByteArray>
#include <QDebug>
#include "bonemark.h"
#include "theme.h"
#include "model.h"
#include "cutface.h"
#include "parttarget.h"
#include "partbase.h"
#include "preferences.h"

class SkeletonNode
{
public:
    SkeletonNode(const QUuid &withId=QUuid()) :
        radius(0),
        boneMark(BoneMark::None),
        cutRotation(0.0),
        cutFace(CutFace::Quad),
        hasCutFaceSettings(false),
        m_x(0),
        m_y(0),
        m_z(0)
    {
        id = withId.isNull() ? QUuid::createUuid() : withId;
    }
    void setRadius(float toRadius)
    {
        if (toRadius < 0.005f)
            toRadius = 0.005f;
        else if (toRadius > 1)
            toRadius = 1;
        radius = toRadius;
    }
    void setCutRotation(float toRotation)
    {
        if (toRotation < -1)
            toRotation = -1;
        else if (toRotation > 1)
            toRotation = 1;
        cutRotation = toRotation;
        hasCutFaceSettings = true;
    }
    void setCutFace(CutFace face)
    {
        cutFace = face;
        cutFaceLinkedId = QUuid();
        hasCutFaceSettings = true;
    }
    void setCutFaceLinkedId(const QUuid &linkedId)
    {
        if (linkedId.isNull()) {
            clearCutFaceSettings();
            return;
        }
        cutFace = CutFace::UserDefined;
        cutFaceLinkedId = linkedId;
        hasCutFaceSettings = true;
    }
    void clearCutFaceSettings()
    {
        cutFace = CutFace::Quad;
        cutFaceLinkedId = QUuid();
        cutRotation = 0;
        hasCutFaceSettings = false;
    }
    float getX(bool rotated=false) const
    {
        if (rotated)
            return m_y;
        return m_x;
    }
    float getY(bool rotated=false) const
    {
        if (rotated)
            return m_x;
        return m_y;
    }
    float getZ(bool rotated=false) const
    {
        (void) rotated;
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
    QUuid id;
    QUuid partId;
    QString name;
    float radius;
    BoneMark boneMark;
    float cutRotation;
    CutFace cutFace;
    QUuid cutFaceLinkedId;
    bool hasCutFaceSettings;
    std::vector<QUuid> edgeIds;
private:
    float m_x;
    float m_y;
    float m_z;
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
    PartBase base;
    float deformThickness;
    float deformWidth;
    bool deformUnified;
    bool rounded;
    bool chamfered;
    QColor color;
    bool hasColor;
    QUuid componentId;
    std::vector<QUuid> nodeIds;
    bool dirty;
    float cutRotation;
    CutFace cutFace;
    QUuid cutFaceLinkedId;
    QUuid materialId;
    PartTarget target;
    float colorSolubility;
    float metalness;
    float roughness;
    float deformMapScale;
    QUuid deformMapImageId;
    float hollowThickness;
    bool countershaded;
    QUuid fillMeshLinkedId;
    bool isPreviewMeshObsolete;
    QPixmap previewPixmap;
    bool smooth;
    SkeletonPart(const QUuid &withId=QUuid()) :
        visible(true),
        locked(false),
        subdived(false),
        disabled(false),
        xMirrored(false),
        zMirrored(false),
        base(PartBase::Average),
        deformThickness(1.0),
        deformWidth(1.0),
        deformUnified(false),
        rounded(false),
        chamfered(false),
        color(Preferences::instance().partColor()),
        hasColor(false),
        dirty(true),
        cutRotation(0.0),
        cutFace(CutFace::Quad),
        target(PartTarget::Model),
        colorSolubility(0.0),
        metalness(0.0),
        roughness(1.0),
        deformMapScale(1.0),
        hollowThickness(0.0),
        countershaded(false),
        isPreviewMeshObsolete(false),
        smooth(true)
    {
        id = withId.isNull() ? QUuid::createUuid() : withId;
    }
    bool hasPolyFunction() const
    {
        return PartTarget::Model == target;
    }
    bool hasSmoothFunction() const
    {
        return PartTarget::Model == target && fillMeshLinkedId.isNull();
    }
    bool hasSubdivFunction() const
    {
        return PartTarget::Model == target && fillMeshLinkedId.isNull();
    }
    bool hasRoundEndFunction() const
    {
        return PartTarget::Model == target && fillMeshLinkedId.isNull();
    }
    bool hasMirrorFunction() const
    {
        return PartTarget::Model == target && fillMeshLinkedId.isNull();
    }
    bool hasChamferFunction() const
    {
        return PartTarget::Model == target && fillMeshLinkedId.isNull();
    }
    bool hasRotationFunction() const
    {
        return PartTarget::Model == target;
    }
    bool hasHollowFunction() const
    {
        return PartTarget::Model == target && fillMeshLinkedId.isNull();
    }
    bool hasCutFaceFunction() const
    {
        return PartTarget::Model == target && fillMeshLinkedId.isNull();
    }
    bool hasLayerFunction() const
    {
        return PartTarget::Model == target;
    }
    bool hasTargetFunction() const
    {
        return fillMeshLinkedId.isNull();
    }
    bool hasBaseFunction() const
    {
        return PartTarget::Model == target && fillMeshLinkedId.isNull();
    }
    bool hasCombineModeFunction() const
    {
        return PartTarget::Model == target;
    }
    bool hasDeformFunction() const
    {
        return PartTarget::Model == target;
    }
    bool hasDeformImageFunction() const
    {
        return hasDeformFunction() && fillMeshLinkedId.isNull();
    }
    bool hasColorFunction() const
    {
        return PartTarget::Model == target && fillMeshLinkedId.isNull();
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
    void setCutFace(CutFace face)
    {
        cutFace = face;
        cutFaceLinkedId = QUuid();
    }
    void setCutFaceLinkedId(const QUuid &linkedId)
    {
        if (linkedId.isNull()) {
            setCutFace(CutFace::Quad);
            return;
        }
        cutFace = CutFace::UserDefined;
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
    bool deformMapScaleAdjusted() const
    {
        return fabs(deformMapScale - 1.0) >= 0.01;
    }
    bool deformMapAdjusted() const
    {
        return deformMapScaleAdjusted() || !deformMapImageId.isNull();
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
        return cutFace != CutFace::Quad;
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
    void copyAttributes(const SkeletonPart &other)
    {
        visible = other.visible;
        locked = other.locked;
        subdived = other.subdived;
        disabled = other.disabled;
        xMirrored = other.xMirrored;
        zMirrored = other.zMirrored;
        base = other.base;
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
    void updatePreviewMesh(Model *previewMesh)
    {
        delete m_previewMesh;
        m_previewMesh = previewMesh;
        isPreviewMeshObsolete = true;
    }
    Model *takePreviewMesh() const
    {
        if (nullptr == m_previewMesh)
            return nullptr;
        return new Model(*m_previewMesh);
    }
private:
    Q_DISABLE_COPY(SkeletonPart);
    Model *m_previewMesh = nullptr;
};

enum class SkeletonDocumentEditMode
{
    Add = 0,
    Select,
    Paint,
    Drag,
    ZoomIn,
    ZoomOut
};

enum class SkeletonProfile
{
    Unknown = 0,
    Main,
    Side
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
    CombineMode combineMode = Preferences::instance().componentCombineMode();
    bool dirty = true;
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
private:
    std::set<QUuid> m_childrenIdSet;
};

class SkeletonDocument : public QObject
{
    Q_OBJECT
signals:
    void partAdded(QUuid partId);
    void nodeAdded(QUuid nodeId);
    void edgeAdded(QUuid edgeId);
    void partRemoved(QUuid partId);
    void partLockStateChanged(QUuid partId);
    void partVisibleStateChanged(QUuid partId);
    void partDisableStateChanged(QUuid partId);
    void componentNameChanged(QUuid componentId);
    void componentChildrenChanged(QUuid componentId);
    void componentRemoved(QUuid componentId);
    void componentAdded(QUuid componentId);
    void componentExpandStateChanged(QUuid componentId);
    void nodeRemoved(QUuid nodeId);
    void edgeRemoved(QUuid edgeId);
    void nodeRadiusChanged(QUuid nodeId);
    void nodeOriginChanged(QUuid nodeId);
    void edgeReversed(QUuid edgeId);
    void originChanged();
    void skeletonChanged();
    void optionsChanged();
public:
    SkeletonDocumentEditMode editMode = SkeletonDocumentEditMode::Select;
    bool xlocked = false;
    bool ylocked = false;
    bool zlocked = false;
    bool radiusLocked = false;
    QImage turnaround;
    QByteArray turnaroundPngByteArray;
    std::map<QUuid, SkeletonPart> partMap;
    std::map<QUuid, SkeletonNode> nodeMap;
    std::map<QUuid, SkeletonEdge> edgeMap;
    std::map<QUuid, SkeletonComponent> componentMap;
    SkeletonComponent rootComponent;

    const SkeletonNode *findNode(QUuid nodeId) const;
    const SkeletonEdge *findEdge(QUuid edgeId) const;
    const SkeletonPart *findPart(QUuid partId) const;
    const SkeletonEdge *findEdgeByNodes(QUuid firstNodeId, QUuid secondNodeId) const;
    void findAllNeighbors(QUuid nodeId, std::set<QUuid> &neighbors) const;
    bool isNodeConnectable(QUuid nodeId) const;
    const SkeletonComponent *findComponent(QUuid componentId) const;
    const SkeletonComponent *findComponentParent(QUuid componentId) const;
    QUuid findComponentParentId(QUuid componentId) const;
    void collectComponentDescendantParts(QUuid componentId, std::vector<QUuid> &partIds) const;
    void collectComponentDescendantComponents(QUuid componentId, std::vector<QUuid> &componentIds) const;
    void resetDirtyFlags();
    void markAllDirty();
    
    virtual bool undoable() const = 0;
    virtual bool redoable() const = 0;
    virtual bool hasPastableNodesInClipboard() const = 0;
    virtual bool originSettled() const = 0;
    virtual bool isNodeEditable(QUuid nodeId) const = 0;
    virtual bool isEdgeEditable(QUuid edgeId) const = 0;
    virtual bool isNodeDeactivated(QUuid nodeId) const
    {
        (void) nodeId;
        return false;
    };
    virtual bool isEdgeDeactivated(QUuid edgeId) const
    {
        (void) edgeId;
        return false;
    };
    virtual void copyNodes(std::set<QUuid> nodeIdSet) const = 0;
    
    float getOriginX(bool rotated=false) const
    {
        if (rotated)
            return m_originY;
        return m_originX;
    }
    float getOriginY(bool rotated=false) const
    {
        if (rotated)
            return m_originX;
        return m_originY;
    }
    float getOriginZ(bool rotated=false) const
    {
        (void) rotated;
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
    
public slots:
    virtual void undo() = 0;
    virtual void redo() = 0;
    virtual void paste() = 0;

    void removeNode(QUuid nodeId);
    void removeEdge(QUuid edgeId);
    void removePart(QUuid partId);
    void addNodeWithId(QUuid nodeId, float x, float y, float z, float radius, QUuid fromNodeId);
    void addNode(float x, float y, float z, float radius, QUuid fromNodeId);
    void scaleNodeByAddRadius(QUuid nodeId, float amount);
    void moveNodeBy(QUuid nodeId, float x, float y, float z);
    void setNodeOrigin(QUuid nodeId, float x, float y, float z);
    void setNodeRadius(QUuid nodeId, float radius);
    void switchNodeXZ(QUuid nodeId);
    void moveOriginBy(float x, float y, float z);
    void addEdge(QUuid fromNodeId, QUuid toNodeId);
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
    void setPartLockState(QUuid partId, bool locked);
    void setPartVisibleState(QUuid partId, bool visible);
    void setPartDisableState(QUuid partId, bool disabled);
    void enableAllPositionRelatedLocks();
    void disableAllPositionRelatedLocks();
    bool isPartReadonly(QUuid partId) const;
    void breakEdge(QUuid edgeId);
    void reduceNode(QUuid nodeId);
    void reverseEdge(QUuid edgeId);
    
private:
    float m_originX = 0;
    float m_originY = 0;
    float m_originZ = 0;
    
    QUuid m_currentCanvasComponentId;
    bool m_allPositionRelatedLocksEnabled = true;
    
    void splitPartByNode(std::vector<std::vector<QUuid>> *groups, QUuid nodeId);
    void joinNodeAndNeiborsToGroup(std::vector<QUuid> *group, QUuid nodeId, std::set<QUuid> *visitMap, QUuid noUseEdgeId=QUuid());
    void splitPartByEdge(std::vector<std::vector<QUuid>> *groups, QUuid edgeId);
    void removePartDontCareComponent(QUuid partId);
    void addPartToComponent(QUuid partId, QUuid componentId);
    bool isDescendantComponent(QUuid componentId, QUuid suspiciousId);
    void removeComponentRecursively(QUuid componentId);
    void updateLinkedPart(QUuid oldPartId, QUuid newPartId);
    QUuid createNode(QUuid nodeId, float x, float y, float z, float radius, QUuid fromNodeId);
};

#endif
