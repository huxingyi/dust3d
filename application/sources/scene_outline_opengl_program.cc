#include "scene_outline_opengl_program.h"
#include <QFile>
#include <QTextStream>
#include <dust3d/base/debug.h>

static const QString& loadOutlineShaderSource(const QString& name)
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

void SceneOutlineOpenGLProgram::addShaderFromResource(QOpenGLShader::ShaderType type, const char* resourceName)
{
    if (!addShaderFromSourceCode(type, loadOutlineShaderSource(resourceName)))
        dust3dDebug << "Failed to addShaderFromResource, resource:" << resourceName << ", " << log().toStdString();
}

void SceneOutlineOpenGLProgram::load(bool isCoreProfile)
{
    if (m_isLoaded)
        return;
    m_isCoreProfile = isCoreProfile;
    if (m_isCoreProfile) {
        addShaderFromResource(QOpenGLShader::Vertex, ":/shaders/scene_outline_core.vert");
        addShaderFromResource(QOpenGLShader::Fragment, ":/shaders/scene_outline_core.frag");
    } else {
        addShaderFromResource(QOpenGLShader::Vertex, ":/shaders/scene_outline.vert");
        addShaderFromResource(QOpenGLShader::Fragment, ":/shaders/scene_outline.frag");
    }
    bindAttributeLocation("vertex", 0);
    bindAttributeLocation("normal", 1);
    link();
    m_isLoaded = true;
}

int SceneOutlineOpenGLProgram::getUniformLocationByName(const std::string& name)
{
    auto findLocation = m_uniformLocationMap.find(name);
    if (findLocation != m_uniformLocationMap.end())
        return findLocation->second;
    int location = uniformLocation(name.c_str());
    m_uniformLocationMap.insert({ name, location });
    return location;
}
