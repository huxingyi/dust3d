#ifndef POSE_PREVIEW_MANAGER_H
#define POSE_PREVIEW_MANAGER_H
#include <QWidget>
#include "skeletondocument.h"
#include "poser.h"
#include "posemeshcreator.h"
#include "meshloader.h"

class PosePreviewManager : public QObject
{
    Q_OBJECT
public:
    PosePreviewManager();
    ~PosePreviewManager();
    bool isRendering();
    bool postUpdate(const Poser &poser,
        const MeshResultContext &meshResultContext,
        const std::map<int, AutoRiggerVertexWeights> &resultWeights);
    MeshLoader *takeResultPreviewMesh();
private slots:
    void poseMeshReady();
signals:
    void resultPreviewMeshChanged();
    void renderDone();
private:
    PoseMeshCreator *m_poseMeshCreator = nullptr;
    MeshLoader *m_previewMesh = nullptr;
};

#endif
