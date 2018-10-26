#ifndef DUST3D_MESH_GENERATOR_H
#define DUST3D_MESH_GENERATOR_H
#include <QObject>
#include <QString>
#include <QImage>
#include <map>
#include <set>
#include <QThread>
#include <QOpenGLWidget>
#include "snapshot.h"
#include "meshloader.h"
#include "outcome.h"
#include "positionmap.h"

class GeneratedCacheContext
{
public:
    ~GeneratedCacheContext();
    std::map<QString, std::vector<std::pair<QVector3D, std::pair<QUuid, QUuid>>>> partBmeshVertices;
    std::map<QString, std::vector<OutcomeNode>> partBmeshNodes;
    std::map<QString, std::vector<std::tuple<PositionMapKey, PositionMapKey, PositionMapKey, PositionMapKey>>> partBmeshQuads;
    std::map<QString, void *> componentCombinableMeshs;
    std::map<QString, std::vector<QVector3D>> componentPositions;
    std::map<QString, PositionMap<std::pair<QVector3D, std::pair<QUuid, QUuid>>>> componentVerticesSources;
    std::map<QString, QString> partMirrorIdMap;
    void updateComponentCombinableMesh(QString componentId, void *mesh);
};

class MeshGenerator : public QObject
{
    Q_OBJECT
public:
    MeshGenerator(Snapshot *snapshot);
    ~MeshGenerator();
    void setSharedContextWidget(QOpenGLWidget *widget);
    void addPartPreviewRequirement(const QUuid &partId);
    void setGeneratedCacheContext(GeneratedCacheContext *cacheContext);
    void setSmoothNormal(bool smoothNormal);
    void setWeldEnabled(bool weldEnabled);
    MeshLoader *takeResultMesh();
    MeshLoader *takePartPreviewMesh(const QUuid &partId);
    const std::set<QUuid> &requirePreviewPartIds();
    const std::set<QUuid> &generatedPreviewPartIds();
    Outcome *takeOutcome();
    void generate();
signals:
    void finished();
public slots:
    void process();
private:
    Snapshot *m_snapshot;
    MeshLoader *m_mesh;
    std::map<QUuid, MeshLoader *> m_partPreviewMeshMap;
    std::set<QUuid> m_requirePreviewPartIds;
    std::set<QUuid> m_generatedPreviewPartIds;
    QThread *m_thread;
    Outcome *m_outcome;
    QOpenGLWidget *m_sharedContextWidget;
    void *m_meshliteContext;
    GeneratedCacheContext *m_cacheContext;
    bool m_smoothNormal;
    bool m_weldEnabled;
    float m_mainProfileMiddleX;
    float m_sideProfileMiddleX;
    float m_mainProfileMiddleY;
    std::map<QString, std::set<QString>> m_partNodeIds;
    std::map<QString, std::set<QString>> m_partEdgeIds;
    std::set<QString> m_dirtyComponentIds;
    std::set<QString> m_dirtyPartIds;
private:
    static bool m_enableDebug;
    static PositionMap<int> *m_forMakePositionKey;
private:
    void loadVertexSources(void *meshliteContext, int meshId, QUuid partId, const std::map<int, QUuid> &bmeshToNodeIdMap, std::vector<std::pair<QVector3D, std::pair<QUuid, QUuid>>> &bmeshVertices,
        std::vector<std::tuple<PositionMapKey, PositionMapKey, PositionMapKey, PositionMapKey>> &bmeshQuads);
    void loadGeneratedPositionsToOutcome(void *meshliteContext, int triangulatedMeshId);
    void collectParts();
    void checkDirtyFlags();
    bool checkIsComponentDirty(QString componentId);
    bool checkIsPartDirty(QString partId);
    void *combineComponentMesh(QString componentId, bool *inverse);
    void *combinePartMesh(QString partId);
};

#endif
