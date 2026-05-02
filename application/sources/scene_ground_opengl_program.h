#ifndef DUST3D_APPLICATION_SCENE_GROUND_OPENGL_PROGRAM_H_
#define DUST3D_APPLICATION_SCENE_GROUND_OPENGL_PROGRAM_H_

#include <QOpenGLShader>
#include <QOpenGLShaderProgram>
#include <map>
#include <string>

class SceneGroundOpenGLProgram : public QOpenGLShaderProgram {
public:
    void load(bool isCoreProfile = false);
    int getUniformLocationByName(const std::string& name);

private:
    void addShaderFromResource(QOpenGLShader::ShaderType type, const char* resourceName);

    bool m_isLoaded = false;
    bool m_isCoreProfile = false;
    std::map<std::string, int> m_uniformLocationMap;
};

#endif
