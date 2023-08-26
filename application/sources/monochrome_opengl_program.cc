#include "monochrome_opengl_program.h"
#include <QFile>
#include <QOpenGLFunctions>
#include <dust3d/base/debug.h>

static const QString& loadShaderSource(const QString& name)
{
    static std::map<QString, QString> s_shaderSources;
    auto findShader = s_shaderSources.find(name);
    if (findShader != s_shaderSources.end()) {
        return findShader->second;
    }
    QFile file(name);
    file.open(QFile::ReadOnly | QFile::Text);
    QTextStream stream(&file);
    auto insertResult = s_shaderSources.insert({ name, stream.readAll() });
    return insertResult.first->second;
}

void MonochromeOpenGLProgram::addShaderFromResource(QOpenGLShader::ShaderType type, const char* resourceName)
{
    if (!addShaderFromSourceCode(type, loadShaderSource(resourceName)))
        dust3dDebug << "Failed to addShaderFromResource, resource:" << resourceName << ", " << log().toStdString();
}

bool MonochromeOpenGLProgram::isCoreProfile() const
{
    return m_isCoreProfile;
}

void MonochromeOpenGLProgram::load(bool isCoreProfile)
{
    if (m_isLoaded)
        return;

    m_isCoreProfile = isCoreProfile;
    if (m_isCoreProfile) {
        addShaderFromResource(QOpenGLShader::Vertex, ":/shaders/monochrome_core.vert");
        addShaderFromResource(QOpenGLShader::Fragment, ":/shaders/monochrome_core.frag");
    } else {
#if defined(Q_OS_WASM)
        addShaderFromResource(QOpenGLShader::Vertex, ":/shaders/monochrome_wasm.vert");
        addShaderFromResource(QOpenGLShader::Fragment, ":/shaders/monochrome_wasm.frag");
#else
        addShaderFromResource(QOpenGLShader::Vertex, ":/shaders/monochrome.vert");
        addShaderFromResource(QOpenGLShader::Fragment, ":/shaders/monochrome.frag");
#endif
    }
    bindAttributeLocation("vertex", 0);
    bindAttributeLocation("color", 1);
    bindAttributeLocation("alpha", 2);
    link();
    bind();
    m_isLoaded = true;
}

int MonochromeOpenGLProgram::getUniformLocationByName(const std::string& name)
{
    auto findLocation = m_uniformLocationMap.find(name);
    if (findLocation != m_uniformLocationMap.end())
        return findLocation->second;
    int location = uniformLocation(name.c_str());
    m_uniformLocationMap.insert({ name, location });
    return location;
}
