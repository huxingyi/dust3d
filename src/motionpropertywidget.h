#ifndef DUST3D_MOTION_PROPERTY_WIDGET_H
#define DUST3D_MOTION_PROPERTY_WIDGET_H
#include <QMainWindow>
#include <QCloseEvent>
#include <queue>
#include "vertebratamotion.h"
#include "rigger.h"
#include "outcome.h"

class SimpleShaderWidget;
class SimpleRenderMeshGenerator;
class MotionPreviewGenerator;

class MotionPropertyWidget : public QMainWindow
{
    Q_OBJECT
public:
    struct ResultMesh
    {
        std::vector<QVector3D> vertices;
        std::vector<std::vector<size_t>> faces;
        std::vector<std::vector<QVector3D>> cornerNormals;
    };
    
    MotionPropertyWidget();
    ~MotionPropertyWidget();
signals:
    void parametersChanged();
protected:
    QSize sizeHint() const override;
public slots:
    void checkRenderQueue();
    void renderMeshReady();
    void generatePreview();
    void previewReady();
    void updateBones(const std::vector<RiggerBone> *rigBones,
        const std::map<int, RiggerVertexWeights> *rigWeights,
        const Outcome *outcome);
private:
    VertebrataMotion::Parameters m_parameters;
    SimpleShaderWidget *m_modelRenderWidget = nullptr;
    SimpleRenderMeshGenerator *m_renderMeshGenerator = nullptr;
    std::queue<ResultMesh> m_renderQueue;
    MotionPreviewGenerator *m_previewGenerator = nullptr;
    bool m_isPreviewObsolete = false;
    std::vector<ResultMesh> m_frames;
    size_t m_frameIndex = 0;
    std::vector<RiggerBone> *m_bones = nullptr;
    std::map<int, RiggerVertexWeights> *m_rigWeights = nullptr;
    Outcome *m_outcome = nullptr;
};

#endif
