#ifndef DUST3D_SKELETON_DOCUMENT_H
#define DUST3D_SKELETON_DOCUMENT_H
#include <QUuid>
#include <QObject>
#include <QString>
#include <cmath>
#include <QImage>
#include <QByteArray>
#include "bonemark.h"
#include "theme.h"
#include "meshloader.h"
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
        if (toRadius < 0.005)
            toRadius = 0.005;
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
    float deformMapScale;
    QUuid deformMapImageId;
    float hollowThickness;
    bool countershaded;
    bool gridded;
    SkeletonPart(const QUuid &withId=QUuid()) :
        visible(true),
        locked(false),
        subdived(false),
        disabled(false),
        xMirrored(false),
        zMirrored(false),
        base(PartBase::XYZ),
        deformThickness(1.0),
        deformWidth(1.0),
        rounded(false),
        chamfered(false),
        color(Preferences::instance().partColor()),
        hasColor(false),
        dirty(true),
        cutRotation(0.0),
        cutFace(CutFace::Quad),
        target(PartTarget::Model),
        colorSolubility(0.0),
        deformMapScale(1.0),
        hollowThickness(0.0),
        countershaded(false),
        gridded(false)
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
        return deformThicknessAdjusted() || deformWidthAdjusted();
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

class SkeletonDocument : public QObject
{
    Q_OBJECT
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

    const SkeletonNode *findNode(QUuid nodeId) const;
    const SkeletonEdge *findEdge(QUuid edgeId) const;
    const SkeletonPart *findPart(QUuid partId) const;
    const SkeletonEdge *findEdgeByNodes(QUuid firstNodeId, QUuid secondNodeId) const;
    void findAllNeighbors(QUuid nodeId, std::set<QUuid> &neighbors) const;
    bool isNodeConnectable(QUuid nodeId) const;
    
    virtual bool undoable() const = 0;
    virtual bool redoable() const = 0;
    virtual bool hasPastableNodesInClipboard() const = 0;
    virtual bool originSettled() const = 0;
    virtual bool isNodeEditable(QUuid nodeId) const = 0;
    virtual bool isEdgeEditable(QUuid edgeId) const = 0;
    virtual bool isNodeDeactivated(QUuid nodeId) const
    {
        return false;
    };
    virtual bool isEdgeDeactivated(QUuid edgeId) const
    {
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
    
private:
    float m_originX = 0;
    float m_originY = 0;
    float m_originZ = 0;
};

#endif
