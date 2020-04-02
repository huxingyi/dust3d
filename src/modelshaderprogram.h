#ifndef DUST3D_MODEL_SHADER_PROGRAM_H
#define DUST3D_MODEL_SHADER_PROGRAM_H
#include <QOpenGLShaderProgram>
#include <QString>

class ModelShaderProgram : public QOpenGLShaderProgram
{
public:
    ModelShaderProgram(bool isCoreProfile);
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
    int mousePickEnabledLoc();
    int mousePickTargetPositionLoc();
    int mousePickRadiusLoc();
    int environmentIrradianceMapIdLoc();
    int environmentIrradianceMapEnabledLoc();
    int environmentSpecularMapIdLoc();
    int environmentSpecularMapEnabledLoc();
    int toonShadingEnabledLoc();
    int renderPurposeLoc();
    int toonEdgeEnabledLoc();
    int screenWidthLoc();
    int screenHeightLoc();
    int toonNormalMapIdLoc();
    int toonDepthMapIdLoc();
    bool isCoreProfile();
    static const QString &loadShaderSource(const QString &name);
private:
    bool m_isCoreProfile = false;
    int m_projectionMatrixLoc = 0;
    int m_modelMatrixLoc = 0;
    int m_normalMatrixLoc = 0;
    int m_viewMatrixLoc = 0;
    int m_lightPosLoc = 0;
    int m_textureIdLoc = 0;
    int m_textureEnabledLoc = 0;
    int m_normalMapEnabledLoc = 0;
    int m_normalMapIdLoc = 0;
    int m_metalnessMapEnabledLoc = 0;
    int m_roughnessMapEnabledLoc = 0;
    int m_ambientOcclusionMapEnabledLoc = 0;
    int m_metalnessRoughnessAmbientOcclusionMapIdLoc = 0;
    int m_mousePickEnabledLoc = 0;
    int m_mousePickTargetPositionLoc = 0;
    int m_mousePickRadiusLoc = 0;
    int m_environmentIrradianceMapIdLoc = 0;
    int m_environmentIrradianceMapEnabledLoc = 0;
    int m_environmentSpecularMapIdLoc = 0;
    int m_environmentSpecularMapEnabledLoc = 0;
    int m_toonShadingEnabledLoc = 0;
    int m_renderPurposeLoc = 0;
    int m_toonEdgeEnabledLoc = 0;
    int m_screenWidthLoc = 0;
    int m_screenHeightLoc = 0;
    int m_toonNormalMapIdLoc = 0;
    int m_toonDepthMapIdLoc = 0;
};

#endif
