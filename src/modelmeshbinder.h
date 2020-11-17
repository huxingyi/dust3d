#ifndef DUST3D_MODEL_MESH_BINDER_H
#define DUST3D_MODEL_MESH_BINDER_H
#include <QOpenGLVertexArrayObject>
#include <QMutex>
#include <QOpenGLBuffer>
#include <QString>
#include <QOpenGLTexture>
#include "model.h"
#include "modelshaderprogram.h"

class ModelMeshBinder
{
public:
    ModelMeshBinder(bool toolEnabled=false);
    ~ModelMeshBinder();
    Model *fetchCurrentMesh();
    void updateMesh(Model *mesh);
    void updateColorTexture(QImage *colorTextureImage);
    void initialize();
    void paint(ModelShaderProgram *program);
    void cleanup();
    void showWireframe();
    void hideWireframe();
    bool isWireframeVisible();
    void enableCheckUv();
    void disableCheckUv();
    void enableEnvironmentLight();
    bool isEnvironmentLightEnabled();
    bool isCheckUvEnabled();
    void reloadMesh();
    void fetchCurrentToonNormalAndDepthMaps(QImage *normalMap, QImage *depthMap);
    void updateToonNormalAndDepthMaps(QImage *normalMap, QImage *depthMap);
private:
    Model *m_mesh = nullptr;
    Model *m_newMesh = nullptr;
    int m_renderTriangleVertexCount = 0;
    int m_renderEdgeVertexCount = 0;
    int m_renderToolVertexCount = 0;
    bool m_newMeshComing = false;
    bool m_showWireframe = false;
    bool m_hasTexture = false;
    QOpenGLTexture *m_texture = nullptr;
    bool m_hasNormalMap = false;
    QOpenGLTexture *m_normalMap = nullptr;
    bool m_hasMetalnessMap = false;
    bool m_hasRoughnessMap = false;
    bool m_hasAmbientOcclusionMap = false;
    QOpenGLTexture *m_metalnessRoughnessAmbientOcclusionMap = nullptr;
    bool m_toolEnabled = false;
    bool m_checkUvEnabled = false;
    bool m_environmentLightEnabled = false;
    QOpenGLTexture *m_environmentIrradianceMap = nullptr;
    QOpenGLTexture *m_environmentSpecularMap = nullptr;
    QOpenGLTexture *m_toonNormalMap = nullptr;
    QOpenGLTexture *m_toonDepthMap = nullptr;
    QImage *m_newToonNormalMap = nullptr;
    QImage *m_newToonDepthMap = nullptr;
    QImage *m_currentToonNormalMap = nullptr;
    QImage *m_currentToonDepthMap = nullptr;
    QImage *m_colorTextureImage = nullptr;
    bool m_newToonMapsComing = false;
private:
    QOpenGLVertexArrayObject m_vaoTriangle;
    QOpenGLBuffer m_vboTriangle;
    QOpenGLVertexArrayObject m_vaoEdge;
    QOpenGLBuffer m_vboEdge;
    QOpenGLVertexArrayObject m_vaoTool;
    QOpenGLBuffer m_vboTool;
    QMutex m_meshMutex;
    QMutex m_newMeshMutex;
    QMutex m_toonNormalAndDepthMapMutex;
    QMutex m_colorTextureMutex;
};

#endif
