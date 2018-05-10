#include <QOpenGLFramebufferObjectFormat>
#include <QThread>
#include <QDebug>
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
    m_ambientOcclusionImage(nullptr)
{
    m_useCore = QSurfaceFormat::defaultFormat().profile() == QSurfaceFormat::CoreProfile;
    
    create();
    m_context = new QOpenGLContext();
    
    QSurfaceFormat fmt = format();
    fmt.setAlphaBufferSize(8);
    fmt.setSamples(4);
    setFormat(fmt);
    
    m_context->setFormat(fmt);
    m_context->create();
}

QImage *AmbientOcclusionBaker::takeAmbientOcclusionImage()
{
    QImage *resultImage = m_ambientOcclusionImage;
    m_ambientOcclusionImage = nullptr;
    return resultImage;
}

AmbientOcclusionBaker::~AmbientOcclusionBaker()
{
    delete m_context;
    m_context = nullptr;
    destroy();
    delete m_ambientOcclusionImage;
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

void AmbientOcclusionBaker::setRenderThread(QThread *thread)
{
    m_context->moveToThread(thread);
}

void AmbientOcclusionBaker::bake()
{
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
    
    // save result to a file
    //if (lmImageSaveTGAf("result.tga", data, w, h, 4, 1.0f))
    //    printf("Saved result.tga\n");

    // upload result
    //glBindTexture(GL_TEXTURE_2D, scene->lightmap);
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_FLOAT, data);
    free(data);
    
    destroyScene(scene);
    
    free(sceneNormals);

    m_context->doneCurrent();
}
