#include <QSurfaceFormat>
#include <QFile>
#include <map>
#include "modelshaderprogram.h"

const QString &ModelShaderProgram::loadShaderSource(const QString &name)
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

ModelShaderProgram::ModelShaderProgram(bool usePBR)
{
    if (QSurfaceFormat::defaultFormat().profile() == QSurfaceFormat::CoreProfile) {
        this->addShaderFromSourceCode(QOpenGLShader::Vertex, loadShaderSource(":/shaders/default.core.vert"));
        this->addShaderFromSourceCode(QOpenGLShader::Fragment, loadShaderSource(":/shaders/default.core.frag"));
    } else {
        this->addShaderFromSourceCode(QOpenGLShader::Vertex, loadShaderSource(":/shaders/default.vert"));
        if (usePBR) {
            this->addShaderFromSourceCode(QOpenGLShader::Fragment, loadShaderSource(":/shaders/pbr-qt.frag"));
        } else {
            this->addShaderFromSourceCode(QOpenGLShader::Fragment, loadShaderSource(":/shaders/default.frag"));
        }
    }
    this->bindAttributeLocation("vertex", 0);
    this->bindAttributeLocation("normal", 1);
    this->bindAttributeLocation("color", 2);
    this->bindAttributeLocation("texCoord", 3);
    this->bindAttributeLocation("metalness", 4);
    this->bindAttributeLocation("roughness", 5);
    this->link();

    this->bind();
    m_projectionMatrixLoc = this->uniformLocation("projectionMatrix");
    m_modelMatrixLoc = this->uniformLocation("modelMatrix");
    m_viewMatrixLoc = this->uniformLocation("viewMatrix");
    m_lightPosLoc = this->uniformLocation("lightPos");
    m_textureIdLoc = this->uniformLocation("textureId");
    m_textureEnabledLoc = this->uniformLocation("textureEnabled");
}

int ModelShaderProgram::projectionMatrixLoc()
{
    return m_projectionMatrixLoc;
}

int ModelShaderProgram::modelMatrixLoc()
{
    return m_modelMatrixLoc;
}

int ModelShaderProgram::viewMatrixLoc()
{
    return m_viewMatrixLoc;
}

int ModelShaderProgram::lightPosLoc()
{
    return m_lightPosLoc;
}

int ModelShaderProgram::textureEnabledLoc()
{
    return m_textureEnabledLoc;
}

int ModelShaderProgram::textureIdLoc()
{
    return m_textureIdLoc;
}
