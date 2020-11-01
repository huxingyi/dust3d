#ifndef DUST3D_MOTION_PROPERTY_WIDGET_H
#define DUST3D_MOTION_PROPERTY_WIDGET_H
#include <QMainWindow>
#include <QCloseEvent>
#include <queue>
#include "vertebratamotion.h"
#include "rigger.h"
#include "outcome.h"

class SimpleShaderWidget;
class MotionsGenerator;
class SimpleShaderMesh;

class MotionPropertyWidget : public QMainWindow
{
    Q_OBJECT
public:
    MotionPropertyWidget();
    ~MotionPropertyWidget();
signals:
    void parametersChanged();
protected:
    QSize sizeHint() const override;
public slots:
    void checkRenderQueue();
    void generatePreview();
    void previewReady();
    void updateBones(RigType rigType,
        const std::vector<RiggerBone> *rigBones,
        const std::map<int, RiggerVertexWeights> *rigWeights,
        const Outcome *outcome);
private:
    VertebrataMotion::Parameters m_parameters;
    SimpleShaderWidget *m_modelRenderWidget = nullptr;
    std::queue<SimpleShaderMesh *> m_renderQueue;
    MotionsGenerator *m_previewGenerator = nullptr;
    bool m_isPreviewObsolete = false;
    std::vector<SimpleShaderMesh *> m_frames;
    size_t m_frameIndex = 0;
    RigType m_rigType = RigType::None;
    std::vector<RiggerBone> *m_bones = nullptr;
    std::map<int, RiggerVertexWeights> *m_rigWeights = nullptr;
    Outcome *m_outcome = nullptr;
};

#endif
