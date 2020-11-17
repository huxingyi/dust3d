#ifndef DUST3D_MOTION_EDIT_WIDGET_H
#define DUST3D_MOTION_EDIT_WIDGET_H
#include <QMainWindow>
#include <QCloseEvent>
#include <queue>
#include <QLineEdit>
#include <QUuid>
#include "vertebratamotion.h"
#include "rigger.h"
#include "object.h"

class SimpleShaderWidget;
class MotionsGenerator;
class SimpleShaderMesh;
class QScrollArea;

class MotionEditWidget : public QMainWindow
{
    Q_OBJECT
public:
    MotionEditWidget();
    ~MotionEditWidget();
signals:
    void parametersChanged();
    void addMotion(const QUuid &motionId, const QString &name, const std::map<QString, QString> &parameters);
    void removeMotion(const QUuid &motionId);
    void setMotionParameters(const QUuid &motionId, const std::map<QString, QString> &parameters);
    void renameMotion(const QUuid &motionId, const QString &name);
public slots:
    void checkRenderQueue();
    void generatePreview();
    void previewReady();
    void updateBones(RigType rigType,
        const std::vector<RiggerBone> *rigBones,
        const std::map<int, RiggerVertexWeights> *rigWeights,
        const Object *object);
    void setEditMotionName(const QString &name);
    void setEditMotionId(const QUuid &motionId);
    void setEditMotionParameters(const std::map<QString, QString> &parameters);
    void updateTitle();
    void save();
    void clearUnsaveState();
    void updateParameters();
    void updateParametersArea();
protected:
    QSize sizeHint() const override;
    void closeEvent(QCloseEvent *event) override;
private:
    QString m_name;
    std::map<QString, QString> m_parameters;
    SimpleShaderWidget *m_modelRenderWidget = nullptr;
    std::queue<SimpleShaderMesh *> m_renderQueue;
    MotionsGenerator *m_previewGenerator = nullptr;
    bool m_isPreviewObsolete = false;
    std::vector<SimpleShaderMesh *> m_frames;
    size_t m_frameIndex = 0;
    RigType m_rigType = RigType::None;
    std::vector<RiggerBone> *m_bones = nullptr;
    std::map<int, RiggerVertexWeights> *m_rigWeights = nullptr;
    Object *m_object = nullptr;
    QLineEdit *m_nameEdit = nullptr;
    bool m_unsaved = false;
    bool m_closed = false;
    QUuid m_motionId;
    QScrollArea *m_parametersArea = nullptr;
};

#endif
