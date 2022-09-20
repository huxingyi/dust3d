#ifndef DUST3D_APPLICATION_MODEL_OPENGL_PROGRAM_H_
#define DUST3D_APPLICATION_MODEL_OPENGL_PROGRAM_H_

#include <QOpenGLShaderProgram>
#include <QOpenGLShader>

class ModelOpenGLProgram: public QOpenGLShaderProgram
{
public:
    void load(bool isCoreProfile=false);
    int getUniformLocationByName(const char *name);

private:
    void addShaderFromResource(QOpenGLShader::ShaderType type, const char *resourceName);

    bool m_isLoaded = false;
    bool m_isCoreProfile = false;
    std::map<std::string, int> m_uniformLocationMap;
};

#endif
