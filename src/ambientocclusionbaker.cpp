#include <QOpenGLFramebufferObjectFormat>
#include <QThread>
#include <QDebug>
#include <QtGlobal>
#include <QGuiApplication>
#define LIGHTMAPPER_IMPLEMENTATION
#include "qtlightmapper.h"
#include "ambientocclusionbaker.h"

// This file is modfied from the example code of Andreas Mantler' lightmapper
// https://github.com/ands/lightmapper/blob/master/example/example.c

AmbientOcclusionBaker::AmbientOcclusionBaker(QScreen *targetScreen) :
    QOffscreenSurface(targetScreen),
    m_context(nullptr),
    m_bakeWidth(256),
    m_bakeHeight(256),
    m_ambientOcclusionImage(nullptr),
    m_colorImage(nullptr),
    m_textureImage(nullptr),
    m_borderImage(nullptr),
    m_guideImage(nullptr),
    m_imageUpdateVersion(0),
    m_resultMesh(nullptr)
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
    m_useCore = QSurfaceFormat::defaultFormat().profile() == QSurfaceFormat::CoreProfile;
#endif
    create();
    m_context = new QOpenGLContext();
    
    QSurfaceFormat fmt = format();
    fmt.setAlphaBufferSize(8);
    fmt.setSamples(4);
    setFormat(fmt);
    
    m_context->setFormat(fmt);
    m_context->create();
}

void AmbientOcclusionBaker::setRenderThread(QThread *thread)
{
    m_context->moveToThread(thread);
}

void AmbientOcclusionBaker::setBorderImage(const QImage &borderImage)
{
    delete m_borderImage;
    m_borderImage = new QImage(borderImage);
}

void AmbientOcclusionBaker::setImageUpdateVersion(unsigned long long version)
{
    m_imageUpdateVersion = version;
}

unsigned long long AmbientOcclusionBaker::getImageUpdateVersion()
{
    return m_imageUpdateVersion;
}

void AmbientOcclusionBaker::setColorImage(const QImage &colorImage)
{
    delete m_colorImage;
    m_colorImage = new QImage(colorImage);
}

QImage *AmbientOcclusionBaker::takeAmbientOcclusionImage()
{
    QImage *resultImage = m_ambientOcclusionImage;
    m_ambientOcclusionImage = nullptr;
    return resultImage;
}

QImage *AmbientOcclusionBaker::takeTextureImage()
{
    QImage *resultImage = m_textureImage;
    m_textureImage = nullptr;
    return resultImage;
}

QImage *AmbientOcclusionBaker::takeGuideImage()
{
    QImage *resultImage = m_guideImage;
    m_guideImage = nullptr;
    return resultImage;
}

MeshLoader *AmbientOcclusionBaker::takeResultMesh()
{
    MeshLoader *resultMesh = m_resultMesh;
    m_resultMesh = nullptr;
    return resultMesh;
}

AmbientOcclusionBaker::~AmbientOcclusionBaker()
{
    delete m_context;
    m_context = nullptr;
    destroy();
    delete m_ambientOcclusionImage;
    delete m_colorImage;
    delete m_textureImage;
    delete m_borderImage;
    delete m_guideImage;
    delete m_resultMesh;
}

void AmbientOcclusionBaker::setInputMesh(const MeshResultContext &meshResultContext)
{
    m_meshResultContext = meshResultContext;
}

void AmbientOcclusionBaker::setBakeSize(int width, int height)
{
    m_bakeWidth = width;
    m_bakeHeight = height;
}

void AmbientOcclusionBaker::process()
{
    bake();
    
    if (m_colorImage) {
        m_textureImage = new QImage(*m_colorImage);
        QPainter mergeTexturePainter(m_textureImage);
        mergeTexturePainter.setCompositionMode(QPainter::CompositionMode_Multiply);
        if (m_ambientOcclusionImage)
            mergeTexturePainter.drawImage(0, 0, *m_ambientOcclusionImage);
        mergeTexturePainter.end();
    } else {
        if (m_ambientOcclusionImage)
            m_textureImage = new QImage(*m_ambientOcclusionImage);
    }
    
    if (m_textureImage) {
        m_guideImage = new QImage(*m_textureImage);
        QPainter mergeGuidePainter(m_guideImage);
        mergeGuidePainter.setCompositionMode(QPainter::CompositionMode_Multiply);
        if (m_borderImage)
            mergeGuidePainter.drawImage(0, 0, *m_borderImage);
        mergeGuidePainter.end();
        
        m_resultMesh = new MeshLoader(m_meshResultContext);
        m_resultMesh->setTextureImage(new QImage(*m_textureImage));
    }
    
    this->moveToThread(QGuiApplication::instance()->thread());
    emit finished();
}

void AmbientOcclusionBaker::bake()
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
    if (m_meshResultContext.parts().empty())
        return;
    
    m_context->makeCurrent(this);
    
    initializeOpenGLFunctions();
    
    scene_t sceneStruct;
    memset(&sceneStruct, 0, sizeof(sceneStruct));
    scene_t *scene = &sceneStruct;
    
    scene->w = m_bakeWidth;
    scene->h = m_bakeHeight;

    std::vector<vertex_t> vertices;
    std::vector<int> indicies;
    std::vector<QVector3D> normals;
    for (const auto &part: m_meshResultContext.parts()) {
        int i = 0;
        int startIndex = vertices.size();
        for (const auto &it: part.second.vertices) {
            vertex_t vertex;
            vertex.p[0] = it.position.x();
            vertex.p[1] = it.position.y();
            vertex.p[2] = it.position.z();
            vertex.t[0] = part.second.vertexUvs[i].uv[0];
            vertex.t[1] = part.second.vertexUvs[i].uv[1];
            vertices.push_back(vertex);
            normals.push_back(QVector3D());
            i++;
        }
        for (const auto &it: part.second.triangles) {
            normals[startIndex + it.indicies[0]] += it.normal;
            normals[startIndex + it.indicies[1]] += it.normal;
            normals[startIndex + it.indicies[2]] += it.normal;
            indicies.push_back(startIndex + it.indicies[0]);
            indicies.push_back(startIndex + it.indicies[1]);
            indicies.push_back(startIndex + it.indicies[2]);
        }
    }
    for (auto &it: normals) {
        it.normalize();
    }
    
    scene->vertices = (vertex_t *)malloc(sizeof(vertex_t) * vertices.size());
    for (auto i = 0u; i < vertices.size(); i++) {
        scene->vertices[i] = vertices[i];
    }
    scene->vertexCount = vertices.size();
    
    scene->indices = (unsigned short *)malloc(sizeof(unsigned short) * indicies.size());
    for (auto i = 0u; i < indicies.size(); i++) {
        scene->indices[i] = indicies[i];
    }
    scene->indexCount = indicies.size();
    
    float *sceneNormals = (float *)malloc(sizeof(float) * 3 * normals.size());
    int sceneNormalItemIndex = 0;
    for (auto i = 0u; i < normals.size(); i++) {
        sceneNormals[sceneNormalItemIndex + 0] = normals[i].x();
        sceneNormals[sceneNormalItemIndex + 1] = normals[i].y();
        sceneNormals[sceneNormalItemIndex + 2] = normals[i].z();
        sceneNormalItemIndex += 3;
    }
    
    initScene(scene);
    
    lm_context *ctx = lmCreate(
        64,               // hemisphere resolution (power of two, max=512)
        0.001f, 100.0f,   // zNear, zFar of hemisphere cameras
        1.0f, 1.0f, 1.0f, // background color (white for ambient occlusion)
        2, 0.01f,         // lightmap interpolation threshold (small differences are interpolated rather than sampled)
                          // check debug_interpolation.tga for an overview of sampled (red) vs interpolated (green) pixels.
        5.0f);            // modifier for camera-to-surface distance for hemisphere rendering.
                          // tweak this to trade-off between interpolated normals quality and other artifacts (see declaration).
    
    int w = scene->w, h = scene->h;
    int c = 4;
    float *data = (float *)calloc(w * h * c, sizeof(float));
    lmSetTargetLightmap(ctx, data, w, h, c);
    
    lmSetGeometry(ctx, NULL,                                                                 // no transformation in this example
        LM_FLOAT, (unsigned char*)scene->vertices + offsetof(vertex_t, p), sizeof(vertex_t),
        LM_FLOAT, (unsigned char*)sceneNormals, sizeof(float) * 3, // no interpolated normals in this example
        LM_FLOAT, (unsigned char*)scene->vertices + offsetof(vertex_t, t), sizeof(vertex_t),
        scene->indexCount, LM_UNSIGNED_SHORT, scene->indices);
    
    int vp[4];
    float view[16], projection[16];
    while (lmBegin(ctx, vp, view, projection))
    {
        // render to lightmapper framebuffer
        glViewport(vp[0], vp[1], vp[2], vp[3]);
        drawScene(scene, view, projection);

        lmEnd(ctx);
    }
    //printf("\rFinished baking %d triangles.\n", scene->indexCount / 3);
    
    lmDestroy(ctx);

    // postprocess texture
    float *temp = (float *)calloc(w * h * 4, sizeof(float));
    for (int i = 0; i < 16; i++)
    {
        lmImageDilate(data, temp, w, h, 4);
        lmImageDilate(temp, data, w, h, 4);
    }
    lmImageSmooth(data, temp, w, h, 4);
    lmImageDilate(temp, data, w, h, 4);
    lmImagePower(data, w, h, 4, 1.0f / 2.2f, 0x7); // gamma correct color channels
    free(temp);
    
    {
        unsigned char *temp = (unsigned char *)LM_CALLOC(w * h * c, sizeof(unsigned char));
        lmImageFtoUB(data, temp, w, h, c, 1.0f);
        m_ambientOcclusionImage = new QImage(w, h, QImage::Format_ARGB32);
        m_ambientOcclusionImage->fill(Qt::black);
        int tempOffset = 0;
        for (auto y = 0; y < h; y++) {
            for (auto x = 0; x < w; x++) {
                uint rgb = 0;
                unsigned int r = temp[tempOffset + 0];
                unsigned int g = temp[tempOffset + 1];
                unsigned int b = temp[tempOffset + 2];
                unsigned int a = temp[tempOffset + 3];
                rgb |= b << 0;   // B
                rgb |= g << 8;   // G
                rgb |= r << 16;  // R
                rgb |= a << 24;  // A
                m_ambientOcclusionImage->setPixel(x, y, rgb);
                tempOffset += 4;
            }
        }
        LM_FREE(temp);
    }
    
    free(data);
    
    destroyScene(scene);
    
    free(sceneNormals);

    m_context->doneCurrent();
#endif
}
