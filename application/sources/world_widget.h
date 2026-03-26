#ifndef DUST3D_APPLICATION_WORLD_WIDGET_H_
#define DUST3D_APPLICATION_WORLD_WIDGET_H_

#include "model_mesh.h"
#include "model_opengl_object.h"
#include "shadow_opengl_program.h"
#include "world_ground_opengl_object.h"
#include "world_ground_opengl_program.h"
#include "world_opengl_program.h"
#include <QMatrix4x4>
#include <QOpenGLWidget>
#include <QTimer>
#include <QVector3D>
#include <memory>

class WorldWidget : public QOpenGLWidget {
    Q_OBJECT
signals:
    void xRotationChanged(int angle);
    void yRotationChanged(int angle);
    void zRotationChanged(int angle);
    void eyePositionChanged(const QVector3D& eyePosition);
    void moveToPositionChanged(const QVector3D& moveToPosition);
    void renderParametersChanged();

public:
    WorldWidget(QWidget* parent = nullptr);
    ~WorldWidget();
    void updateMesh(ModelMesh* mesh);
    void toggleRotation();
    void enableMove(bool enabled);
    void enableZoom(bool enabled);
    void setMoveAndZoomByWindow(bool byWindow);
    void setMoveToPosition(const QVector3D& moveToPosition);
    void zoom(float delta);
    int xRot();
    int yRot();
    int zRot();
    const QVector3D& eyePosition();
    const QVector3D& moveToPosition();

public slots:
    void setXRotation(int angle);
    void setYRotation(int angle);
    void setZRotation(int angle);
    void setEyePosition(const QVector3D& eyePosition);
    void cleanup();
    void reRender();
    void canvasResized();

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int width, int height) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    int m_xRot = m_defaultXRotation;
    int m_yRot = m_defaultYRotation;
    int m_zRot = m_defaultZRotation;
    int m_directionOnMoveStart = 0;
    std::unique_ptr<ShadowOpenGLProgram> m_shadowOpenGLProgram;
    std::unique_ptr<WorldOpenGLProgram> m_worldOpenGLProgram;
    std::unique_ptr<WorldGroundOpenGLProgram> m_groundOpenGLProgram;
    std::unique_ptr<ModelOpenGLObject> m_modelOpenGLObject;
    std::unique_ptr<WorldGroundOpenGLObject> m_groundOpenGLObject;
    bool m_moveStarted = false;
    bool m_moveEnabled = true;
    bool m_zoomEnabled = true;
    QPoint m_lastPos;
    QMatrix4x4 m_projection;
    QMatrix4x4 m_camera;
    QMatrix4x4 m_world;
    QMatrix4x4 m_lightView;
    QMatrix4x4 m_lightProjection;
    QMatrix4x4 m_lightSpaceMatrix;
    QVector3D m_eyePosition = m_defaultEyePosition;
    static float m_minZoomRatio;
    static float m_maxZoomRatio;
    QPoint m_moveStartPos;
    QRect m_moveStartGeometry;
    int m_modelInitialHeight = 0;
    QTimer* m_rotationTimer = nullptr;
    int m_widthInPixels = 0;
    int m_heightInPixels = 0;
    QVector3D m_moveToPosition;
    bool m_moveAndZoomByWindow = true;
    GLuint m_shadowFBOId = 0;
    GLuint m_shadowDepthTexture = 0;
    static const int m_shadowMapSize = 2048;

    void initializeShadowFBO();
    void cleanupShadowFBO();
    void updateProjectionMatrix();
    void normalizeAngle(int& angle);
    void drawShadowPass();
    void drawWorldModel();
    void drawGround();

public:
    static int m_defaultXRotation;
    static int m_defaultYRotation;
    static int m_defaultZRotation;
    static QVector3D m_defaultEyePosition;
};

#endif
