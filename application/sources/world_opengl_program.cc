#include "world_opengl_program.h"
#include <QFile>
#include <QMutexLocker>
#include <QOpenGLContext>
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

void WorldOpenGLProgram::addShaderFromResource(QOpenGLShader::ShaderType type, const char* resourceName)
{
    if (!addShaderFromSourceCode(type, loadShaderSource(resourceName)))
        dust3dDebug << "Failed to addShaderFromResource, resource:" << resourceName << ", " << log().toStdString();
}

bool WorldOpenGLProgram::isCoreProfile() const
{
    return m_isCoreProfile;
}

void WorldOpenGLProgram::load(bool isCoreProfile)
{
    if (m_isLoaded)
        return;
    m_isCoreProfile = isCoreProfile;
    if (m_isCoreProfile) {
        addShaderFromResource(QOpenGLShader::Vertex, ":/shaders/world_model_core.vert");
        addShaderFromResource(QOpenGLShader::Fragment, ":/shaders/world_model_core.frag");
    } else {
        addShaderFromResource(QOpenGLShader::Vertex, ":/shaders/world_model.vert");
        addShaderFromResource(QOpenGLShader::Fragment, ":/shaders/world_model.frag");
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

void WorldOpenGLProgram::activeAndBindTexture(int location, QOpenGLTexture* texture)
{
    if (0 == texture->textureId()) {
        dust3dDebug << "Expected texture with a bound id";
        return;
    }
    texture->bind(location);
}

void WorldOpenGLProgram::updateTextureImage(std::unique_ptr<QImage> image)
{
    QMutexLocker lock(&m_imageMutex);
    m_textureImage = std::move(image);
    m_imageIsDirty = true;
}

void WorldOpenGLProgram::updateNormalMapImage(std::unique_ptr<QImage> image)
{
    QMutexLocker lock(&m_imageMutex);
    m_normalMapImage = std::move(image);
    m_imageIsDirty = true;
}

void WorldOpenGLProgram::updateMetalnessRoughnessAmbientOcclusionMapImage(std::unique_ptr<QImage> image,
    bool hasMetalnessMap,
    bool hasRoughnessMap,
    bool hasAmbientOcclusionMap)
{
    QMutexLocker lock(&m_imageMutex);
    m_metalnessRoughnessAmbientOcclusionMapImage = std::move(image);
    m_hasMetalnessMap = hasMetalnessMap;
    m_hasRoughnessMap = hasRoughnessMap;
    m_hasAmbientOcclusionMap = hasAmbientOcclusionMap;
    m_imageIsDirty = true;
}

void WorldOpenGLProgram::bindMaps(GLuint shadowDepthTexture)
{
    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();

    // Texture unit 1: shadow map
    int bindLocation = 1;
    f->glActiveTexture(GL_TEXTURE0 + bindLocation);
    f->glBindTexture(GL_TEXTURE_2D, shadowDepthTexture);
    setUniformValue(getUniformLocationByName("shadowMap"), bindLocation);

    // Upload pending images if dirty
    if (m_imageIsDirty) {
        QMutexLocker lock(&m_imageMutex);
        if (m_imageIsDirty) {
            m_imageIsDirty = false;
            if (m_texture) {
                m_texture->destroy();
                m_texture.reset();
            }
            if (m_textureImage)
                m_texture = std::make_unique<QOpenGLTexture>(*m_textureImage);
            if (m_normalMap) {
                m_normalMap->destroy();
                m_normalMap.reset();
            }
            if (m_normalMapImage)
                m_normalMap = std::make_unique<QOpenGLTexture>(*m_normalMapImage);
            if (m_metalnessRoughnessAmbientOcclusionMap) {
                m_metalnessRoughnessAmbientOcclusionMap->destroy();
                m_metalnessRoughnessAmbientOcclusionMap.reset();
            }
            if (m_metalnessRoughnessAmbientOcclusionMapImage)
                m_metalnessRoughnessAmbientOcclusionMap = std::make_unique<QOpenGLTexture>(*m_metalnessRoughnessAmbientOcclusionMapImage);
            m_metalnessMapEnabled = m_hasMetalnessMap;
            m_roughnessMapEnabled = m_hasRoughnessMap;
            m_ambientOcclusionMapEnabled = m_hasAmbientOcclusionMap;
        }
    }

    // Texture unit 2: model texture
    bindLocation++;
    if (m_texture) {
        activeAndBindTexture(bindLocation, m_texture.get());
        setUniformValue(getUniformLocationByName("textureId"), bindLocation);
        setUniformValue(getUniformLocationByName("textureEnabled"), (int)1);
    } else {
        setUniformValue(getUniformLocationByName("textureId"), (int)0);
        setUniformValue(getUniformLocationByName("textureEnabled"), (int)0);
    }

    // Texture unit 3: normal map
    bindLocation++;
    if (m_normalMap) {
        activeAndBindTexture(bindLocation, m_normalMap.get());
        setUniformValue(getUniformLocationByName("normalMapId"), bindLocation);
        setUniformValue(getUniformLocationByName("normalMapEnabled"), (int)1);
    } else {
        setUniformValue(getUniformLocationByName("normalMapId"), (int)0);
        setUniformValue(getUniformLocationByName("normalMapEnabled"), (int)0);
    }

    // Texture unit 4: metalness/roughness/AO map
    bindLocation++;
    if (m_metalnessRoughnessAmbientOcclusionMap) {
        activeAndBindTexture(bindLocation, m_metalnessRoughnessAmbientOcclusionMap.get());
        setUniformValue(getUniformLocationByName("metalnessRoughnessAoMapId"), bindLocation);
        setUniformValue(getUniformLocationByName("metalnessMapEnabled"), (int)m_metalnessMapEnabled);
        setUniformValue(getUniformLocationByName("roughnessMapEnabled"), (int)m_roughnessMapEnabled);
        setUniformValue(getUniformLocationByName("aoMapEnabled"), (int)m_ambientOcclusionMapEnabled);
    } else {
        setUniformValue(getUniformLocationByName("metalnessRoughnessAoMapId"), (int)0);
        setUniformValue(getUniformLocationByName("metalnessMapEnabled"), (int)0);
        setUniformValue(getUniformLocationByName("roughnessMapEnabled"), (int)0);
        setUniformValue(getUniformLocationByName("aoMapEnabled"), (int)0);
    }
}

void WorldOpenGLProgram::releaseMaps()
{
    if (m_texture)
        m_texture->release();
    if (m_normalMap)
        m_normalMap->release();
    if (m_metalnessRoughnessAmbientOcclusionMap)
        m_metalnessRoughnessAmbientOcclusionMap->release();
}

int WorldOpenGLProgram::getUniformLocationByName(const std::string& name)
{
    auto findLocation = m_uniformLocationMap.find(name);
    if (findLocation != m_uniformLocationMap.end())
        return findLocation->second;
    int location = uniformLocation(name.c_str());
    m_uniformLocationMap.insert({ name, location });
    return location;
}
