// This file follow the Stackoverflow content license: CC BY-SA 4.0,
// since it's based on Prashanth N Udupa's work: https://stackoverflow.com/questions/35134270/how-to-use-qopenglframebufferobject-for-shadow-mapping
#ifndef DUST3D_SIMPLE_SHADER_MESH_BINDER_H
#define DUST3D_SIMPLE_SHADER_MESH_BINDER_H
#include <QMatrix4x4>
#include <QVector3D>
#include <QOpenGLShaderProgram>
#include <QOpenGLFunctions>
#include <QOpenGLBuffer>
#include <QMutex>
#include "simpleshadermesh.h"

class SimpleShaderMeshBinder : public QOpenGLFunctions
{
public:
    ~SimpleShaderMeshBinder()
    {
        delete m_shadowProgram;
        delete m_sceneProgram;
        delete m_vertexBuffer;
        delete m_indexBuffer;
        delete m_mesh;
        delete m_newMesh;
    }
    
    void updateMesh(SimpleShaderMesh *mesh);
    
    void setShadowMapTextureId(const uint &val) 
    {
        m_shadowMapTextureId = val;
    }
    
    void setSceneMatrix(const QMatrix4x4 &matrix) 
    {
        m_sceneMatrix = matrix;
    }
    
    void renderShadow(const QMatrix4x4 &projectionMatrix, const QMatrix4x4 &viewMatrix);
    void renderScene(const QVector3D &eyePosition, const QVector3D &lightDirection,
                const QMatrix4x4 &projectionMatrix, const QMatrix4x4 &viewMatrix,
                const QMatrix4x4 &lightViewMatrix);
                
private:
    QMatrix4x4 m_sceneMatrix;
    QMatrix4x4 m_matrix;
    QOpenGLShaderProgram *m_shadowProgram = nullptr;
    QOpenGLShaderProgram *m_sceneProgram = nullptr;
    bool m_openglFunctionsInitialized = false;
    QOpenGLBuffer *m_vertexBuffer = nullptr;
    QOpenGLBuffer *m_indexBuffer = nullptr;
    uint m_shadowMapTextureId = 0;
    int m_vertexCount = 0;
    int m_normalOffset = 0;
    SimpleShaderMesh *m_mesh = nullptr;
    bool m_newMeshComing = false;
    SimpleShaderMesh *m_newMesh = nullptr;
    QMutex m_newMeshMutex;
    
    void checkNewMesh();
};

#endif
