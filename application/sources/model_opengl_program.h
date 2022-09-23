#ifndef DUST3D_APPLICATION_MODEL_OPENGL_PROGRAM_H_
#define DUST3D_APPLICATION_MODEL_OPENGL_PROGRAM_H_

#include <memory>
#include <QOpenGLShaderProgram>
#include <QOpenGLShader>
#include <QOpenGLTexture>

class ModelOpenGLProgram: public QOpenGLShaderProgram
{
public:
    void load(bool isCoreProfile=false);
    int getUniformLocationByName(const std::string &name);
    bool isCoreProfile() const;
    void bindEnvironment();

private:
    void addShaderFromResource(QOpenGLShader::ShaderType type, const char *resourceName);

    bool m_isLoaded = false;
    bool m_isCoreProfile = false;
    std::map<std::string, int> m_uniformLocationMap;

    std::unique_ptr<QOpenGLTexture> m_environmentIrradianceMap;
    std::unique_ptr<QOpenGLTexture> m_environmentSpecularMap;
    std::unique_ptr<std::vector<std::unique_ptr<QOpenGLTexture>>> m_environmentIrradianceMaps;
    std::unique_ptr<std::vector<std::unique_ptr<QOpenGLTexture>>> m_environmentSpecularMaps;
};

#endif
