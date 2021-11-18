#ifndef DUST3D_APPLICATION_SKELETON_DOCUMENT_H_
#define DUST3D_APPLICATION_SKELETON_DOCUMENT_H_

#include <QObject>
#include <QString>
#include <cmath>
#include <QImage>
#include <QByteArray>
#include <QDebug>
#include <QVector3D>
#include <QPixmap>
#include <dust3d/base/uuid.h>
#include <dust3d/base/cut_face.h>
#include <dust3d/base/part_target.h>
#include <dust3d/base/part_base.h>
#include <dust3d/base/combine_mode.h>
#include "theme.h"
#include "model.h"
#include "debug.h"

class SkeletonNode
{
public:
    SkeletonNode(const dust3d::Uuid &withId=dust3d::Uuid()) :
        radius(0),
        cutRotation(0.0),
        cutFace(dust3d::CutFace::Quad),
        hasCutFaceSettings(false),
        m_x(0),
        m_y(0),
        m_z(0)
    {
        id = withId.isNull() ? dust3d::Uuid::createUuid() : withId;
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
    void setCutFace(dust3d::CutFace face)
    {
        cutFace = face;
        cutFaceLinkedId = dust3d::Uuid();
        hasCutFaceSettings = true;
    }
    void setCutFaceLinkedId(const dust3d::Uuid &linkedId)
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

class SkeletonEdge
{
public:
    SkeletonEdge(const dust3d::Uuid &withId=dust3d::Uuid())
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

class SkeletonPart
{
public:
    ~SkeletonPart()
    {
        delete m_previewMesh;
    }
    dust3d::Uuid id;
    QString name;
    bool visible;
    bool locked;
    bool subdived;
    bool disabled;
    bool xMirrored;
    dust3d::PartBase base;
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
    bool isPreviewMeshObsolete;
    QPixmap previewPixmap;
    bool smooth;
    SkeletonPart(const dust3d::Uuid &withId=dust3d::Uuid()) :
        visible(true),
        locked(false),
        subdived(false),
        disabled(false),
        xMirrored(false),
        base(dust3d::PartBase::Average),
        deformThickness(1.0),
        deformWidth(1.0),
        deformUnified(false),
        rounded(false),
        chamfered(false),
        color(Qt::white),
        hasColor(false),
        dirty(true),
        cutRotation(0.0),
        cutFace(dust3d::CutFace::Quad),
        target(dust3d::PartTarget::Model),
        colorSolubility(0.0),
        metalness(0.0),
        roughness(1.0),
        hollowThickness(0.0),
        countershaded(false),
        isPreviewMeshObsolete(false),
        smooth(false)
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
    void setCutFaceLinkedId(const dust3d::Uuid &linkedId)
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
    void copyAttributes(const SkeletonPart &other)
    {
        visible = other.visible;
        locked = other.locked;
        subdived = other.subdived;
        disabled = other.disabled;
        xMirrored = other.xMirrored;
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
    SkeletonComponent(const dust3d::Uuid &withId, const QString &linkData=QString(), const QString &linkDataType=QString())
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
private:
    std::set<dust3d::Uuid> m_childrenIdSet;
};

class SkeletonDocument : public QObject
{
    Q_OBJECT
signals:
    void partAdded(dust3d::Uuid partId);
    void nodeAdded(dust3d::Uuid nodeId);
    void edgeAdded(dust3d::Uuid edgeId);
    void partRemoved(dust3d::Uuid partId);
    void partLockStateChanged(dust3d::Uuid partId);
    void partVisibleStateChanged(dust3d::Uuid partId);
    void partDisableStateChanged(dust3d::Uuid partId);
    void componentNameChanged(dust3d::Uuid componentId);
    void componentChildrenChanged(dust3d::Uuid componentId);
    void componentRemoved(dust3d::Uuid componentId);
    void componentAdded(dust3d::Uuid componentId);
    void componentExpandStateChanged(dust3d::Uuid componentId);
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
public:
    SkeletonDocumentEditMode editMode = SkeletonDocumentEditMode::Select;
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

    const SkeletonNode *findNode(dust3d::Uuid nodeId) const;
    const SkeletonEdge *findEdge(dust3d::Uuid edgeId) const;
    const SkeletonPart *findPart(dust3d::Uuid partId) const;
    const SkeletonEdge *findEdgeByNodes(dust3d::Uuid firstNodeId, dust3d::Uuid secondNodeId) const;
    void findAllNeighbors(dust3d::Uuid nodeId, std::set<dust3d::Uuid> &neighbors) const;
    bool isNodeConnectable(dust3d::Uuid nodeId) const;
    const SkeletonComponent *findComponent(dust3d::Uuid componentId) const;
    const SkeletonComponent *findComponentParent(dust3d::Uuid componentId) const;
    dust3d::Uuid findComponentParentId(dust3d::Uuid componentId) const;
    void collectComponentDescendantParts(dust3d::Uuid componentId, std::vector<dust3d::Uuid> &partIds) const;
    void collectComponentDescendantComponents(dust3d::Uuid componentId, std::vector<dust3d::Uuid> &componentIds) const;
    void resetDirtyFlags();
    void markAllDirty();
    
    virtual bool undoable() const = 0;
    virtual bool redoable() const = 0;
    virtual bool hasPastableNodesInClipboard() const = 0;
    virtual bool originSettled() const = 0;
    virtual bool isNodeEditable(dust3d::Uuid nodeId) const = 0;
    virtual bool isEdgeEditable(dust3d::Uuid edgeId) const = 0;
    virtual bool isNodeDeactivated(dust3d::Uuid nodeId) const
    {
        (void) nodeId;
        return false;
    };
    virtual bool isEdgeDeactivated(dust3d::Uuid edgeId) const
    {
        (void) edgeId;
        return false;
    };
    virtual void copyNodes(std::set<dust3d::Uuid> nodeIdSet) const = 0;
    
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
    void createNewComponentAndMoveThisIn(dust3d::Uuid componentId);
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
    float m_originX = 0;
    float m_originY = 0;
    float m_originZ = 0;
    
    dust3d::Uuid m_currentCanvasComponentId;
    bool m_allPositionRelatedLocksEnabled = true;
    
    void splitPartByNode(std::vector<std::vector<dust3d::Uuid>> *groups, dust3d::Uuid nodeId);
    void joinNodeAndNeiborsToGroup(std::vector<dust3d::Uuid> *group, dust3d::Uuid nodeId, std::set<dust3d::Uuid> *visitMap, dust3d::Uuid noUseEdgeId=dust3d::Uuid());
    void splitPartByEdge(std::vector<std::vector<dust3d::Uuid>> *groups, dust3d::Uuid edgeId);
    void removePartDontCareComponent(dust3d::Uuid partId);
    void addPartToComponent(dust3d::Uuid partId, dust3d::Uuid componentId);
    bool isDescendantComponent(dust3d::Uuid componentId, dust3d::Uuid suspiciousId);
    void removeComponentRecursively(dust3d::Uuid componentId);
    void updateLinkedPart(dust3d::Uuid oldPartId, dust3d::Uuid newPartId);
    dust3d::Uuid createNode(dust3d::Uuid nodeId, float x, float y, float z, float radius, dust3d::Uuid fromNodeId);
};

#endif
