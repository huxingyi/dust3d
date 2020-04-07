#ifndef DUST3D_POSE_PREVIEW_MANAGER_H
#define DUST3D_POSE_PREVIEW_MANAGER_H
#include <QWidget>
#include "document.h"
#include "poser.h"
#include "posemeshcreator.h"
#include "model.h"

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
    Model *takeResultPreviewMesh();
private slots:
    void poseMeshReady();
signals:
    void resultPreviewMeshChanged();
    void renderDone();
private:
    PoseMeshCreator *m_poseMeshCreator = nullptr;
    Model *m_previewMesh = nullptr;
};

#endif
