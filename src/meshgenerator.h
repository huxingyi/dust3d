#ifndef DUST3D_MESH_GENERATOR_H
#define DUST3D_MESH_GENERATOR_H
#include <QObject>
#include <set>
#include <QColor>
#include <nodemesh/combiner.h>
#include <nodemesh/positionkey.h>
#include "outcome.h"
#include "snapshot.h"
#include "combinemode.h"
#include "meshloader.h"

class GeneratedPart
{
public:
    ~GeneratedPart()
    {
        delete mesh;
    };
    nodemesh::Combiner::Mesh *mesh = nullptr;
    std::vector<QVector3D> vertices;
    std::vector<std::vector<size_t>> faces;
    std::vector<OutcomeNode> outcomeNodes;
    std::vector<std::pair<QVector3D, std::pair<QUuid, QUuid>>> outcomeNodeVertices;
    std::vector<std::vector<size_t>> previewTriangles;
    bool isSucceed = false;
};

class GeneratedComponent
{
public:
    ~GeneratedComponent()
    {
        delete mesh;
    };
    nodemesh::Combiner::Mesh *mesh = nullptr;
    std::set<std::pair<nodemesh::PositionKey, nodemesh::PositionKey>> sharedQuadEdges;
    std::set<nodemesh::PositionKey> noneSeamVertices;
    std::vector<OutcomeNode> outcomeNodes;
    std::vector<std::pair<QVector3D, std::pair<QUuid, QUuid>>> outcomeNodeVertices;
};

class GeneratedCacheContext
{
public:
    std::map<QString, GeneratedComponent> components;
    std::map<QString, GeneratedPart> parts;
    std::map<QString, QString> partMirrorIdMap;
};

class MeshGenerator : public QObject
{
    Q_OBJECT
public:
    MeshGenerator(Snapshot *snapshot);
    ~MeshGenerator();
    bool isSucceed();
    MeshLoader *takeResultMesh();
    MeshLoader *takePartPreviewMesh(const QUuid &partId);
    const std::set<QUuid> &generatedPreviewPartIds();
    Outcome *takeOutcome();
    void generate();
    void setGeneratedCacheContext(GeneratedCacheContext *cacheContext);
    void setSmoothShadingThresholdAngleDegrees(float degrees);
signals:
    void finished();
public slots:
    void process();
    
private:
    QColor m_defaultPartColor = Qt::white;
    Snapshot *m_snapshot = nullptr;
    GeneratedCacheContext *m_cacheContext = nullptr;
    std::set<QString> m_dirtyComponentIds;
    std::set<QString> m_dirtyPartIds;
    float m_mainProfileMiddleX = 0;
    float m_sideProfileMiddleX = 0;
    float m_mainProfileMiddleY = 0;
    Outcome *m_outcome = nullptr;
    std::map<QString, std::set<QString>> m_partNodeIds;
    std::map<QString, std::set<QString>> m_partEdgeIds;
    std::set<QUuid> m_generatedPreviewPartIds;
    MeshLoader *m_resultMesh = nullptr;
    std::map<QUuid, MeshLoader *> m_partPreviewMeshes;
    bool m_isSucceed = false;
    bool m_cacheEnabled = false;
    float m_smoothShadingThresholdAngleDegrees = 60;
    
    void collectParts();
    bool checkIsComponentDirty(const QString &componentIdString);
    bool checkIsPartDirty(const QString &partIdString);
    bool checkIsPartDependencyDirty(const QString &partIdString);
    void checkDirtyFlags();
    nodemesh::Combiner::Mesh *combinePartMesh(const QString &partIdString);
    nodemesh::Combiner::Mesh *combineComponentMesh(const QString &componentIdString, CombineMode *combineMode);
    void makeXmirror(const std::vector<QVector3D> &sourceVertices, const std::vector<std::vector<size_t>> &sourceFaces,
        std::vector<QVector3D> *destVertices, std::vector<std::vector<size_t>> *destFaces);
    void collectSharedQuadEdges(const std::vector<QVector3D> &vertices, const std::vector<std::vector<size_t>> &faces,
        std::set<std::pair<nodemesh::PositionKey, nodemesh::PositionKey>> *sharedQuadEdges);
    nodemesh::Combiner::Mesh *combineTwoMeshes(const nodemesh::Combiner::Mesh &first, const nodemesh::Combiner::Mesh &second,
        nodemesh::Combiner::Method method,
        bool recombine=true);
    void generateSmoothTriangleVertexNormals(const std::vector<QVector3D> &vertices, const std::vector<std::vector<size_t>> &triangles,
        const std::vector<QVector3D> &triangleNormals,
        std::vector<std::vector<QVector3D>> *triangleVertexNormals);
    const std::map<QString, QString> *findComponent(const QString &componentIdString);
    CombineMode componentCombineMode(const std::map<QString, QString> *component);
    nodemesh::Combiner::Mesh *combineComponentChildGroupMesh(const std::vector<QString> &componentIdStrings,
        GeneratedComponent &componentCache);
    nodemesh::Combiner::Mesh *combineMultipleMeshes(const std::vector<std::pair<nodemesh::Combiner::Mesh *, CombineMode>> &multipleMeshes, bool recombine=true);
    QString componentColorName(const std::map<QString, QString> *component);
    void collectUncombinedComponent(const QString &componentIdString);
};

#endif
