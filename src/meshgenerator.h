#ifndef MESH_GENERATOR_H
#define MESH_GENERATOR_H
#include <QObject>
#include <QString>
#include <QImage>
#include <map>
#include <set>
#include <QThread>
#include "skeletonsnapshot.h"
#include "mesh.h"
#include "modelofflinerender.h"

class MeshGenerator : public QObject
{
    Q_OBJECT
public:
    MeshGenerator(SkeletonSnapshot *snapshot, QThread *thread);
    ~MeshGenerator();
    void addPreviewRequirement();
    void addPartPreviewRequirement(const QString &partId);
    Mesh *takeResultMesh();
    QImage *takePreview();
    QImage *takePartPreview(const QString &partId);
signals:
    void finished();
public slots:
    void process();
private:
    SkeletonSnapshot *m_snapshot;
    Mesh *m_mesh;
    QImage *m_preview;
    std::map<QString, QImage *> m_partPreviewMap;
    bool m_requirePreview;
    std::set<QString> m_requirePartPreviewMap;
    ModelOfflineRender *m_previewRender;
    std::map<QString, ModelOfflineRender *> m_partPreviewRenderMap;
    QThread *m_thread;
private:
    void resolveBoundingBox(QRectF *mainProfile, QRectF *sideProfile, const QString &partId=QString());
    static bool enableDebug;
};

#endif
