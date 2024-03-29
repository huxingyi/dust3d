#ifndef DUST3D_APPLICATION_MONOCHROME_OPENGL_PROGRAM_H_
#define DUST3D_APPLICATION_MONOCHROME_OPENGL_PROGRAM_H_

#include <QOpenGLShader>
#include <QOpenGLShaderProgram>
#include <memory>

class MonochromeOpenGLProgram : public QOpenGLShaderProgram {
public:
    void load(bool isCoreProfile = false);
    int getUniformLocationByName(const std::string& name);
    bool isCoreProfile() const;

private:
    void addShaderFromResource(QOpenGLShader::ShaderType type, const char* resourceName);

    bool m_isLoaded = false;
    bool m_isCoreProfile = false;
    std::map<std::string, int> m_uniformLocationMap;
};

#endif
