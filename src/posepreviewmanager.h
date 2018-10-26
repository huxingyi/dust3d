#ifndef DUST3D_POSE_PREVIEW_MANAGER_H
#define DUST3D_POSE_PREVIEW_MANAGER_H
#include <QWidget>
#include "document.h"
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
        const Outcome &outcome,
        const std::map<int, RiggerVertexWeights> &resultWeights);
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
