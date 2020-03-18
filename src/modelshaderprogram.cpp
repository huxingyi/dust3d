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

bool ModelShaderProgram::isCoreProfile()
{
    return m_isCoreProfile;
}

ModelShaderProgram::ModelShaderProgram()
{
    if (QSurfaceFormat::defaultFormat().profile() == QSurfaceFormat::CoreProfile) {
        this->addShaderFromSourceCode(QOpenGLShader::Vertex, loadShaderSource(":/shaders/default.core.vert"));
        this->addShaderFromSourceCode(QOpenGLShader::Fragment, loadShaderSource(":/shaders/default.core.frag"));
        m_isCoreProfile = true;
    } else {
        this->addShaderFromSourceCode(QOpenGLShader::Vertex, loadShaderSource(":/shaders/default.vert"));
        this->addShaderFromSourceCode(QOpenGLShader::Fragment, loadShaderSource(":/shaders/default.frag"));
    }
    this->bindAttributeLocation("vertex", 0);
    this->bindAttributeLocation("normal", 1);
    this->bindAttributeLocation("color", 2);
    this->bindAttributeLocation("texCoord", 3);
    this->bindAttributeLocation("metalness", 4);
    this->bindAttributeLocation("roughness", 5);
    this->bindAttributeLocation("tangent", 6);
    this->bindAttributeLocation("alpha", 7);
    this->link();

    this->bind();
    m_projectionMatrixLoc = this->uniformLocation("projectionMatrix");
    m_modelMatrixLoc = this->uniformLocation("modelMatrix");
    m_normalMatrixLoc = this->uniformLocation("normalMatrix");
    m_viewMatrixLoc = this->uniformLocation("viewMatrix");
    m_lightPosLoc = this->uniformLocation("lightPos");
    m_textureIdLoc = this->uniformLocation("textureId");
    m_textureEnabledLoc = this->uniformLocation("textureEnabled");
    m_normalMapIdLoc = this->uniformLocation("normalMapId");
    m_normalMapEnabledLoc = this->uniformLocation("normalMapEnabled");
    m_metalnessMapEnabledLoc = this->uniformLocation("metalnessMapEnabled");
    m_roughnessMapEnabledLoc = this->uniformLocation("roughnessMapEnabled");
    m_ambientOcclusionMapEnabledLoc = this->uniformLocation("ambientOcclusionMapEnabled");
    m_metalnessRoughnessAmbientOcclusionMapIdLoc = this->uniformLocation("metalnessRoughnessAmbientOcclusionMapId");
    m_mousePickEnabledLoc = this->uniformLocation("mousePickEnabled");
    m_mousePickTargetPositionLoc = this->uniformLocation("mousePickTargetPosition");
    m_mousePickRadiusLoc = this->uniformLocation("mousePickRadius");
    if (m_isCoreProfile) {
        m_environmentIrradianceMapIdLoc = this->uniformLocation("environmentIrradianceMapId");
        m_environmentIrradianceMapEnabledLoc = this->uniformLocation("environmentIrradianceMapEnabled");
        m_environmentSpecularMapIdLoc = this->uniformLocation("environmentSpecularMapId");
        m_environmentSpecularMapEnabledLoc = this->uniformLocation("environmentSpecularMapEnabled");
    }
}

int ModelShaderProgram::projectionMatrixLoc()
{
    return m_projectionMatrixLoc;
}

int ModelShaderProgram::modelMatrixLoc()
{
    return m_modelMatrixLoc;
}

int ModelShaderProgram::normalMatrixLoc()
{
    return m_normalMatrixLoc;
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

int ModelShaderProgram::normalMapEnabledLoc()
{
    return m_normalMapEnabledLoc;
}

int ModelShaderProgram::normalMapIdLoc()
{
    return m_normalMapIdLoc;
}

int ModelShaderProgram::metalnessMapEnabledLoc()
{
    return m_metalnessMapEnabledLoc;
}

int ModelShaderProgram::roughnessMapEnabledLoc()
{
    return m_roughnessMapEnabledLoc;
}

int ModelShaderProgram::ambientOcclusionMapEnabledLoc()
{
    return m_ambientOcclusionMapEnabledLoc;
}

int ModelShaderProgram::metalnessRoughnessAmbientOcclusionMapIdLoc()
{
    return m_metalnessRoughnessAmbientOcclusionMapIdLoc;
}

int ModelShaderProgram::mousePickEnabledLoc()
{
    return m_mousePickEnabledLoc;
}

int ModelShaderProgram::mousePickTargetPositionLoc()
{
    return m_mousePickTargetPositionLoc;
}

int ModelShaderProgram::mousePickRadiusLoc()
{
    return m_mousePickRadiusLoc;
}

int ModelShaderProgram::environmentIrradianceMapIdLoc()
{
    return m_environmentIrradianceMapIdLoc;
}

int ModelShaderProgram::environmentIrradianceMapEnabledLoc()
{
    return m_environmentIrradianceMapEnabledLoc;
}

int ModelShaderProgram::environmentSpecularMapIdLoc()
{
    return m_environmentSpecularMapIdLoc;
}

int ModelShaderProgram::environmentSpecularMapEnabledLoc()
{
    return m_environmentSpecularMapEnabledLoc;
}

