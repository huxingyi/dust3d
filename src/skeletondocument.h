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

class SkeletonNode
{
public:
    SkeletonNode(const QUuid &withId=QUuid()) :
        x(0),
        y(0),
        z(0),
        radius(0),
        boneMark(BoneMark::None)
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
    QUuid id;
    QUuid partId;
    QString name;
    float x;
    float y;
    float z;
    float radius;
    BoneMark boneMark;
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
    bool chamfered;
    QColor color;
    bool hasColor;
    QUuid componentId;
    std::vector<QUuid> nodeIds;
    bool dirty;
    float cutRotation;
    CutFace cutFace;
    QUuid materialId;
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
        chamfered(false),
        color(Theme::white),
        hasColor(false),
        dirty(true),
        cutRotation(0.0),
        cutFace(CutFace::Quad)
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
    bool cutRotationAdjusted() const
    {
        return fabs(cutRotation - 0.0) >= 0.01;
    }
    bool cutFaceAdjusted() const
    {
        return cutFace != CutFace::Quad;
    }
    bool cutAdjusted() const
    {
        return cutRotationAdjusted() || cutFaceAdjusted();
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
        deformThickness = other.deformThickness;
        deformWidth = other.deformWidth;
        rounded = other.rounded;
        chamfered = other.chamfered;
        color = other.color;
        hasColor = other.hasColor;
        cutRotation = other.cutRotation;
        cutFace = other.cutFace;
        componentId = other.componentId;
        dirty = other.dirty;
        materialId = other.materialId;
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
    float originX = 0;
    float originY = 0;
    float originZ = 0;
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
    
public slots:
    virtual void undo() = 0;
    virtual void redo() = 0;
    virtual void paste() = 0;
};

#endif
