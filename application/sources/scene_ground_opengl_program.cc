#include "scene_ground_opengl_program.h"
#include <QFile>
#include <QOpenGLShader>
#include <QTextStream>
#include <dust3d/base/debug.h>

static const QString& loadShaderSource(const QString& name)
{
    static std::map<QString, QString> s_shaderSources;
    auto findShader = s_shaderSources.find(name);
    if (findShader != s_shaderSources.end())
        return findShader->second;
    QFile file(name);
    (void)file.open(QFile::ReadOnly | QFile::Text);
    QTextStream stream(&file);
    auto insertResult = s_shaderSources.insert({ name, stream.readAll() });
    return insertResult.first->second;
}

void SceneGroundOpenGLProgram::addShaderFromResource(QOpenGLShader::ShaderType type, const char* resourceName)
{
    if (!addShaderFromSourceCode(type, loadShaderSource(resourceName)))
        dust3dDebug << "Failed to addShaderFromResource, resource:" << resourceName << ", " << log().toStdString();
}

void SceneGroundOpenGLProgram::load(bool isCoreProfile)
{
    if (m_isLoaded)
        return;
    m_isCoreProfile = isCoreProfile;
    if (m_isCoreProfile) {
        addShaderFromResource(QOpenGLShader::Vertex, ":/shaders/world_ground_core.vert");
        addShaderFromResource(QOpenGLShader::Fragment, ":/shaders/scene_ground_core.frag");
    } else {
        addShaderFromResource(QOpenGLShader::Vertex, ":/shaders/world_ground.vert");
        addShaderFromResource(QOpenGLShader::Fragment, ":/shaders/scene_ground.frag");
    }
    bindAttributeLocation("vertex", 0);
    bindAttributeLocation("normal", 1);
    bindAttributeLocation("color", 2);
    bindAttributeLocation("texCoord", 3);
    bindAttributeLocation("metalness", 4);
    bindAttributeLocation("roughness", 5);
    bindAttributeLocation("tangent", 6);
    bindAttributeLocation("alpha", 7);
    link();
    m_isLoaded = true;
}

int SceneGroundOpenGLProgram::getUniformLocationByName(const std::string& name)
{
    auto findLocation = m_uniformLocationMap.find(name);
    if (findLocation != m_uniformLocationMap.end())
        return findLocation->second;
    int location = uniformLocation(name.c_str());
    m_uniformLocationMap.insert({ name, location });
    return location;
}
