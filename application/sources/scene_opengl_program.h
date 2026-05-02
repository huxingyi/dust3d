#ifndef DUST3D_APPLICATION_SCENE_OPENGL_PROGRAM_H_
#define DUST3D_APPLICATION_SCENE_OPENGL_PROGRAM_H_

#include <QMutex>
#include <QOpenGLFunctions>
#include <QOpenGLShader>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <map>
#include <memory>
#include <string>

class SceneOpenGLProgram : public QOpenGLShaderProgram {
public:
    void load(bool isCoreProfile = false);
    int getUniformLocationByName(const std::string& name);
    bool isCoreProfile() const;
    void bindMaps(GLuint shadowDepthTexture);
    void releaseMaps();
    void updateTextureImage(std::unique_ptr<QImage> image);
    void updateNormalMapImage(std::unique_ptr<QImage> image);
    void updateMetalnessRoughnessAmbientOcclusionMapImage(std::unique_ptr<QImage> image,
        bool hasMetalnessMap = false,
        bool hasRoughnessMap = false,
        bool hasAmbientOcclusionMap = false);

private:
    void addShaderFromResource(QOpenGLShader::ShaderType type, const char* resourceName);
    void activeAndBindTexture(int location, QOpenGLTexture* texture);

    bool m_isLoaded = false;
    bool m_isCoreProfile = false;
    std::map<std::string, int> m_uniformLocationMap;

    QMutex m_imageMutex;
    bool m_imageIsDirty = false;
    std::unique_ptr<QImage> m_textureImage;
    std::unique_ptr<QImage> m_normalMapImage;
    std::unique_ptr<QImage> m_metalnessRoughnessAmbientOcclusionMapImage;
    bool m_hasMetalnessMap = false;
    bool m_hasRoughnessMap = false;
    bool m_hasAmbientOcclusionMap = false;
    std::unique_ptr<QOpenGLTexture> m_texture;
    std::unique_ptr<QOpenGLTexture> m_normalMap;
    std::unique_ptr<QOpenGLTexture> m_metalnessRoughnessAmbientOcclusionMap;
    bool m_metalnessMapEnabled = false;
    bool m_roughnessMapEnabled = false;
    bool m_ambientOcclusionMapEnabled = false;
};

#endif
