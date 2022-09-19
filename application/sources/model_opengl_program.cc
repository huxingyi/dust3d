#include <QOpenGLFunctions>
#include <QFile>
#include "model_opengl_program.h"

static const QString &loadShaderSource(const QString &name)
{
    static std::map<QString, QString> s_shaderSources;
    auto findShader = s_shaderSources.find(name);
    if (findShader != s_shaderSources.end()) {
        return findShader->second;
    }
    QFile file(name);
    file.open(QFile::ReadOnly | QFile::Text);
    QTextStream stream(&file);
    auto insertResult = s_shaderSources.insert({name, stream.readAll()});
    return insertResult.first->second;
}

void ModelOpenGLProgram::load(bool isCoreProfile)
{
    if (m_isLoaded)
        return;

    m_isCoreProfile = isCoreProfile;
    if (m_isCoreProfile) {
        addShaderFromSourceCode(QOpenGLShader::Vertex, loadShaderSource(":/shaders/model_core.vert"));
        addShaderFromSourceCode(QOpenGLShader::Fragment, loadShaderSource(":/shaders/model_core.frag"));
    } else {
        addShaderFromSourceCode(QOpenGLShader::Vertex, loadShaderSource(":/shaders/model.vert"));
        addShaderFromSourceCode(QOpenGLShader::Fragment, loadShaderSource(":/shaders/model.frag"));
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
    bind();
        
    m_isLoaded = true;
}
