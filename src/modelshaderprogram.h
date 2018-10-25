#ifndef DUST3D_MODEL_SHADER_PROGRAM_H
#define DUST3D_MODEL_SHADER_PROGRAM_H
#include <QOpenGLShaderProgram>
#include <QString>

class ModelShaderProgram : public QOpenGLShaderProgram
{
public:
    ModelShaderProgram(bool usePBR=true);
    int projectionMatrixLoc();
    int modelMatrixLoc();
    int normalMatrixLoc();
    int viewMatrixLoc();
    int lightPosLoc();
    int textureIdLoc();
    int textureEnabledLoc();
    int normalMapEnabledLoc();
    int normalMapIdLoc();
    int metalnessMapEnabledLoc();
    int roughnessMapEnabledLoc();
    int ambientOcclusionMapEnabledLoc();
    int metalnessRoughnessAmbientOcclusionMapIdLoc();
    static const QString &loadShaderSource(const QString &name);
private:
    int m_projectionMatrixLoc;
    int m_modelMatrixLoc;
    int m_normalMatrixLoc;
    int m_viewMatrixLoc;
    int m_lightPosLoc;
    int m_textureIdLoc;
    int m_textureEnabledLoc;
    int m_normalMapEnabledLoc;
    int m_normalMapIdLoc;
    int m_metalnessMapEnabledLoc;
    int m_roughnessMapEnabledLoc;
    int m_ambientOcclusionMapEnabledLoc;
    int m_metalnessRoughnessAmbientOcclusionMapIdLoc;
};

#endif
