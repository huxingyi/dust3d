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
#include "skeletonsnapshot.h"
#include "meshloader.h"
#include "meshgenerator.h"
#include "skeletongenerator.h"
#include "theme.h"
#include "texturegenerator.h"
#include "meshresultpostprocessor.h"
#include "ambientocclusionbaker.h"

class SkeletonNode
{
public:
    SkeletonNode(const QUuid &withId=QUuid()) :
        x(0),
        y(0),
        z(0),
        radius(0)
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
    QImage preview;
    std::vector<QUuid> nodeIds;
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
        hasColor(false)
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
    }
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

class SkeletonDocument : public QObject
{
    Q_OBJECT
signals:
    void partAdded(QUuid partId);
    void nodeAdded(QUuid nodeId);
    void edgeAdded(QUuid edgeId);
    void partRemoved(QUuid partId);
    void partListChanged();
    void nodeRemoved(QUuid nodeId);
    void edgeRemoved(QUuid edgeId);
    void nodeRadiusChanged(QUuid nodeId);
    void nodeOriginChanged(QUuid nodeId);
    void edgeChanged(QUuid edgeId);
    void partChanged(QUuid partId);
    void partPreviewChanged(QUuid partId);
    void resultMeshChanged();
    void turnaroundChanged();
    void editModeChanged();
    void skeletonChanged();
    void resultSkeletonChanged();
    void resultTextureChanged();
    void resultBakedTextureChanged();
    void postProcessedResultChanged();
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
    void cleanup();
    void originChanged();
    void xlockStateChanged();
    void ylockStateChanged();
    void zlockStateChanged();
    void checkPart(QUuid partId);
    void partChecked(QUuid partId);
    void partUnchecked(QUuid partId);
    void enableBackgroundBlur();
    void disableBackgroundBlur();
    void exportReady();
public: // need initialize
    float originX;
    float originY;
    float originZ;
    SkeletonDocumentEditMode editMode;
    bool xlocked;
    bool ylocked;
    bool zlocked;
    QImage *textureGuideImage;
    QImage *textureImage;
    QImage *textureBorderImage;
    QImage *textureAmbientOcclusionImage;
    QImage *textureColorImage;
public:
    SkeletonDocument();
    ~SkeletonDocument();
    std::map<QUuid, SkeletonPart> partMap;
    std::map<QUuid, SkeletonNode> nodeMap;
    std::map<QUuid, SkeletonEdge> edgeMap;
    std::vector<QUuid> partIds;
    QImage turnaround;
    QImage preview;
    void toSnapshot(SkeletonSnapshot *snapshot, const std::set<QUuid> &limitNodeIds=std::set<QUuid>()) const;
    void fromSnapshot(const SkeletonSnapshot &snapshot);
    void addFromSnapshot(const SkeletonSnapshot &snapshot);
    const SkeletonNode *findNode(QUuid nodeId) const;
    const SkeletonEdge *findEdge(QUuid edgeId) const;
    const SkeletonPart *findPart(QUuid partId) const;
    const SkeletonEdge *findEdgeByNodes(QUuid firstNodeId, QUuid secondNodeId) const;
    MeshLoader *takeResultMesh();
    MeshLoader *takeResultSkeletonMesh();
    MeshLoader *takeResultTextureMesh();
    void updateTurnaround(const QImage &image);
    bool hasPastableContentInClipboard() const;
    bool undoable() const;
    bool redoable() const;
    bool isNodeEditable(QUuid nodeId) const;
    bool isEdgeEditable(QUuid edgeId) const;
    bool originSettled() const;
    MeshResultContext &currentPostProcessedResultContext();
    bool isExportReady() const;
public slots:
    void removeNode(QUuid nodeId);
    void removeEdge(QUuid edgeId);
    void removePart(QUuid partId);
    void addNode(float x, float y, float z, float radius, QUuid fromNodeId);
    void scaleNodeByAddRadius(QUuid nodeId, float amount);
    void moveNodeBy(QUuid nodeId, float x, float y, float z);
    void setNodeOrigin(QUuid nodeId, float x, float y, float z);
    void setNodeRadius(QUuid nodeId, float radius);
    void moveOriginBy(float x, float y, float z);
    void addEdge(QUuid fromNodeId, QUuid toNodeId);
    void setEditMode(SkeletonDocumentEditMode mode);
    void uiReady();
    void generateMesh();
    void meshReady();
    void generateSkeleton();
    void skeletonReady();
    void generateTexture();
    void textureReady();
    void postProcess();
    void postProcessedMeshResultReady();
    void bakeAmbientOcclusionTexture();
    void ambientOcclusionTextureReady();
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
private:
    void splitPartByNode(std::vector<std::vector<QUuid>> *groups, QUuid nodeId);
    void joinNodeAndNeiborsToGroup(std::vector<QUuid> *group, QUuid nodeId, std::set<QUuid> *visitMap, QUuid noUseEdgeId=QUuid());
    void splitPartByEdge(std::vector<std::vector<QUuid>> *groups, QUuid edgeId);
    bool isPartReadonly(QUuid partId) const;
    QUuid createNode(float x, float y, float z, float radius, QUuid fromNodeId);
    void settleOrigin();
    void checkExportReadyState();
private: // need initialize
    bool m_resultMeshIsObsolete;
    MeshGenerator *m_meshGenerator;
    MeshLoader *m_resultMesh;
    int m_batchChangeRefCount;
    MeshResultContext *m_currentMeshResultContext;
    bool m_resultSkeletonIsObsolete;
    SkeletonGenerator *m_skeletonGenerator;
    MeshLoader *m_resultSkeletonMesh;
    bool m_textureIsObsolete;
    TextureGenerator *m_textureGenerator;
    bool m_postProcessResultIsObsolete;
    MeshResultPostProcessor *m_postProcessor;
    MeshResultContext *m_postProcessedResultContext;
    MeshLoader *m_resultTextureMesh;
    unsigned long long m_textureImageUpdateVersion;
    AmbientOcclusionBaker *m_ambientOcclusionBaker;
    unsigned long long m_ambientOcclusionBakedImageUpdateVersion;
private:
    static unsigned long m_maxSnapshot;
    std::deque<SkeletonHistoryItem> m_undoItems;
    std::deque<SkeletonHistoryItem> m_redoItems;
};

#endif
