#ifndef DUST3D_APPLICATION_SCENE_WIDGET_H_
#define DUST3D_APPLICATION_SCENE_WIDGET_H_

#include "model_mesh.h"
#include "model_opengl_object.h"
#include "monochrome_mesh.h"
#include "monochrome_opengl_object.h"
#include "monochrome_opengl_program.h"
#include "scene_ground_opengl_program.h"
#include "scene_opengl_program.h"
#include "shadow_opengl_program.h"
#include "world_ground_opengl_object.h"
#include <QImage>
#include <QMatrix4x4>
#include <QOpenGLWidget>
#include <QRectF>
#include <QTimer>
#include <QVector3D>
#include <memory>

class QOpenGLTexture;

class SceneWidget : public QOpenGLWidget {
    Q_OBJECT
public:
    struct NameCube {
        float x;
        float z;
        float width;
        float depth;
        float height;
    };

signals:
    void xRotationChanged(int angle);
    void yRotationChanged(int angle);
    void zRotationChanged(int angle);
    void eyePositionChanged(const QVector3D& eyePosition);
    void moveToPositionChanged(const QVector3D& moveToPosition);
    void renderParametersChanged();

public:
    SceneWidget(QWidget* parent = nullptr);
    ~SceneWidget();
    void updateMesh(ModelMesh* mesh);
    void updatePreviewMesh(int index, ModelMesh* mesh);
    void updateWireframeMesh(MonochromeMesh* mesh);
    void setGroundOffset(float offsetX, float offsetZ);
    void toggleWireframe();
    void setWireframeVisible(bool visible);
    bool isWireframeVisible();
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
    const std::vector<NameCube>& nameCubes() const;
    std::vector<QVector2D> dropSpawnCenters() const;
    void triggerDropSimulation();
    void stopSimulation();
    void resumeSimulation();

    struct PhysicsCube {
        enum class ShapeType {
            Cuboid,
            Tube,
            Cone
        };

        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float vx = 0.0f;
        float vy = 0.0f;
        float vz = 0.0f;
        float width = 0.0f;
        float depth = 0.0f;
        float height = 0.0f;
        float yaw = 0.0f;
        float pitch = 0.0f;
        float roll = 0.0f;
        float angularVelocityX = 0.0f;
        float angularVelocityY = 0.0f;
        float angularVelocityZ = 0.0f;
        float mass = 1.0f;
        ShapeType shapeType = ShapeType::Cuboid;
        dust3d::Color color;
        QRectF uvRect;
        bool settled = false;
    };

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
    std::unique_ptr<SceneOpenGLProgram> m_worldOpenGLProgram;
    std::unique_ptr<SceneGroundOpenGLProgram> m_groundOpenGLProgram;
    std::unique_ptr<ModelOpenGLObject> m_modelOpenGLObject;
    std::vector<std::unique_ptr<ModelOpenGLObject>> m_previewOpenGLObjects;
    std::unique_ptr<ModelOpenGLObject> m_tubeOpenGLObject;
    std::unique_ptr<WorldGroundOpenGLObject> m_groundOpenGLObject;
    std::unique_ptr<MonochromeOpenGLProgram> m_monochromeOpenGLProgram;
    std::unique_ptr<MonochromeOpenGLObject> m_wireframeOpenGLObject;
    bool m_isWireframeVisible = false;
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

    std::unique_ptr<QOpenGLTexture> m_nameAtlasTexture;
    std::unique_ptr<QImage> m_modelTextureImage;
    std::unique_ptr<QImage> m_modelNormalMapImage;
    std::unique_ptr<QImage> m_modelMetalnessRoughnessAmbientOcclusionMapImage;
    bool m_modelHasMetalnessInImage = false;
    bool m_modelHasRoughnessInImage = false;
    bool m_modelHasAmbientOcclusionInImage = false;
    std::vector<std::unique_ptr<QImage>> m_previewTextureImages;
    std::vector<std::unique_ptr<QImage>> m_previewNormalMapImages;
    std::vector<std::unique_ptr<QImage>> m_previewMetalnessRoughnessAmbientOcclusionMapImages;
    std::vector<bool> m_previewHasMetalnessInImage;
    std::vector<bool> m_previewHasRoughnessInImage;
    std::vector<bool> m_previewHasAmbientOcclusionInImage;
    std::vector<NameCube> m_nameCubes;
    std::vector<PhysicsCube> m_physicsCubes;
    QTimer* m_physicsTimer = nullptr;
    int m_activePhysicsCubeIndex = 0;
    std::vector<QRectF> m_dropSimulationUvRects;
    std::vector<float> m_dropSimulationWidthFactors;
    bool m_dropSimulationReady = false;
    bool m_dropSimulationStarted = false;

    void startDropSimulation(const std::vector<QRectF>& uvRects, const std::vector<float>& widthFactors);
    void setDropSimulationData(const std::vector<QRectF>& uvRects, const std::vector<float>& widthFactors);
    void updatePhysicsStep();

    void initializeShadowFBO();
    void cleanupShadowFBO();
    void updateProjectionMatrix();
    void normalizeAngle(int& angle);
    void drawShadowPass();
    void drawWorldModel();
    void drawGround();
    void drawWireframe();

    float m_groundOffsetX = 0.0f;
    float m_groundOffsetZ = 0.0f;

public:
    static int m_defaultXRotation;
    static int m_defaultYRotation;
    static int m_defaultZRotation;
    static QVector3D m_defaultEyePosition;
};

#endif
