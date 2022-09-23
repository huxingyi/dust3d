#include <QOpenGLFunctions>
#include <QFile>
#include <dust3d/base/debug.h>
#include "model_opengl_program.h"
#include "dds_file.h"

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

void ModelOpenGLProgram::addShaderFromResource(QOpenGLShader::ShaderType type, const char *resourceName)
{
    if (!addShaderFromSourceCode(type, loadShaderSource(resourceName)))
        dust3dDebug << "Failed to addShaderFromResource, resource:" << resourceName << ", " << log().toStdString();
}

bool ModelOpenGLProgram::isCoreProfile() const
{
    return m_isCoreProfile;
}

void ModelOpenGLProgram::load(bool isCoreProfile)
{
    if (m_isLoaded)
        return;

    m_isCoreProfile = isCoreProfile;
    if (m_isCoreProfile) {
        addShaderFromResource(QOpenGLShader::Vertex, ":/shaders/model_core.vert");
        addShaderFromResource(QOpenGLShader::Fragment, ":/shaders/model_core.frag");
    } else {
        addShaderFromResource(QOpenGLShader::Vertex, ":/shaders/model.vert");
        addShaderFromResource(QOpenGLShader::Fragment, ":/shaders/model.frag");
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

void ModelOpenGLProgram::bindEnvironment()
{
    if (m_isCoreProfile) {
        if (!m_environmentIrradianceMap) {
            DdsFileReader irradianceFile(":/resources/cedar_bridge_irradiance.dds");
            m_environmentIrradianceMap.reset(irradianceFile.createOpenGLTexture());
        }
        if (!m_environmentSpecularMap) {
            DdsFileReader irradianceFile(":/resources/cedar_bridge_specular.dds");
            m_environmentSpecularMap.reset(irradianceFile.createOpenGLTexture());
        }
        
        m_environmentIrradianceMap->bind(0);
        setUniformValue(getUniformLocationByName("environmentIrradianceMapId"), 0);

        m_environmentSpecularMap->bind(1);
        setUniformValue(getUniformLocationByName("environmentSpecularMapId"), 1);
    } else {
        if (!m_environmentIrradianceMaps) {
            DdsFileReader irradianceFile(":/resources/cedar_bridge_irradiance.dds");
            m_environmentIrradianceMaps = std::move(irradianceFile.createOpenGLTextures());
        }
        if (!m_environmentSpecularMaps) {
            DdsFileReader irradianceFile(":/resources/cedar_bridge_specular.dds");
            m_environmentSpecularMaps = std::move(irradianceFile.createOpenGLTextures());
        }

        size_t bindPosition = 0;

        bindPosition = 0;
        for (size_t i = 0; i < m_environmentIrradianceMaps->size(); ++i)
            m_environmentIrradianceMaps->at(i)->bind(bindPosition++);
        for (size_t i = 0; i < m_environmentSpecularMaps->size(); ++i)
            m_environmentSpecularMaps->at(i)->bind(bindPosition++);

        bindPosition = 0;
        for (size_t i = 0; i < m_environmentIrradianceMaps->size(); ++i)
            setUniformValue(getUniformLocationByName("environmentIrradianceMapId[" + std::to_string(i) + "]"), (int)(bindPosition++));
        for (size_t i = 0; i < m_environmentSpecularMaps->size(); ++i)
            setUniformValue(getUniformLocationByName("environmentSpecularMapId[" + std::to_string(i) + "]"), (int)(bindPosition++));
    }
}

int ModelOpenGLProgram::getUniformLocationByName(const std::string &name)
{
    auto findLocation = m_uniformLocationMap.find(name);
    if (findLocation != m_uniformLocationMap.end())
        return findLocation->second;
    int location = uniformLocation(name.c_str());
    m_uniformLocationMap.insert({name, location});
    return location;
}
