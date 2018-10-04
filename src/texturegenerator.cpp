#include <QPainter>
#include <QGuiApplication>
#include "texturegenerator.h"
#include "theme.h"

int TextureGenerator::m_textureWidth = 512;
int TextureGenerator::m_textureHeight = 512;

TextureGenerator::TextureGenerator(const MeshResultContext &meshResultContext, QThread *thread) :
    m_resultTextureGuideImage(nullptr),
    m_resultTextureImage(nullptr),
    m_resultTextureBorderImage(nullptr),
    m_resultTextureColorImage(nullptr),
    m_thread(thread),
    m_resultMesh(nullptr)
{
    m_resultContext = new MeshResultContext();
    *m_resultContext = meshResultContext;
}

TextureGenerator::~TextureGenerator()
{
    delete m_resultContext;
    delete m_resultTextureGuideImage;
    delete m_resultTextureImage;
    delete m_resultTextureBorderImage;
    delete m_resultTextureColorImage;
    delete m_resultMesh;
}

QImage *TextureGenerator::takeResultTextureGuideImage()
{
    QImage *resultTextureGuideImage = m_resultTextureGuideImage;
    m_resultTextureGuideImage = nullptr;
    return resultTextureGuideImage;
}

QImage *TextureGenerator::takeResultTextureImage()
{
    QImage *resultTextureImage = m_resultTextureImage;
    m_resultTextureImage = nullptr;
    return resultTextureImage;
}

QImage *TextureGenerator::takeResultTextureBorderImage()
{
    QImage *resultTextureBorderImage = m_resultTextureBorderImage;
    m_resultTextureBorderImage = nullptr;
    return resultTextureBorderImage;
}

QImage *TextureGenerator::takeResultTextureColorImage()
{
    QImage *resultTextureColorImage = m_resultTextureColorImage;
    m_resultTextureColorImage = nullptr;
    return resultTextureColorImage;
}

MeshResultContext *TextureGenerator::takeResultContext()
{
    MeshResultContext *resultContext = m_resultContext;
    m_resultTextureImage = nullptr;
    return resultContext;
}

MeshLoader *TextureGenerator::takeResultMesh()
{
    MeshLoader *resultMesh = m_resultMesh;
    m_resultMesh = nullptr;
    return resultMesh;
}

void TextureGenerator::process()
{
    const std::vector<Material> &triangleMaterials = m_resultContext->triangleMaterials();
    const std::vector<ResultTriangleUv> &triangleUvs = m_resultContext->triangleUvs();
    
    m_resultTextureColorImage = new QImage(TextureGenerator::m_textureWidth, TextureGenerator::m_textureHeight, QImage::Format_ARGB32);
    m_resultTextureColorImage->fill(Qt::transparent);
    
    m_resultTextureBorderImage = new QImage(TextureGenerator::m_textureWidth, TextureGenerator::m_textureHeight, QImage::Format_ARGB32);
    m_resultTextureBorderImage->fill(Qt::transparent);
    
    QColor borderColor = Qt::darkGray;
    QPen pen(borderColor);
    
    QPainter texturePainter;
    texturePainter.begin(m_resultTextureColorImage);
    texturePainter.setRenderHint(QPainter::Antialiasing);
    texturePainter.setRenderHint(QPainter::HighQualityAntialiasing);
    
    QPainter textureBorderPainter;
    textureBorderPainter.begin(m_resultTextureBorderImage);
    textureBorderPainter.setRenderHint(QPainter::Antialiasing);
    textureBorderPainter.setRenderHint(QPainter::HighQualityAntialiasing);
 
    // round 1, paint background
    for (auto i = 0u; i < triangleUvs.size(); i++) {
        QPainterPath path;
        const ResultTriangleUv *uv = &triangleUvs[i];
        for (auto j = 0; j < 3; j++) {
            if (0 == j) {
                path.moveTo(uv->uv[j][0] * TextureGenerator::m_textureWidth, uv->uv[j][1] * TextureGenerator::m_textureHeight);
            } else {
                path.lineTo(uv->uv[j][0] * TextureGenerator::m_textureWidth, uv->uv[j][1] * TextureGenerator::m_textureHeight);
            }
        }
        QPen textureBorderPen(triangleMaterials[i].color);
        textureBorderPen.setWidth(32);
        texturePainter.setPen(textureBorderPen);
        texturePainter.setBrush(QBrush(triangleMaterials[i].color));
        texturePainter.drawPath(path);
    }
    // round 2, real paint
    texturePainter.setPen(Qt::NoPen);
    for (auto i = 0u; i < triangleUvs.size(); i++) {
        QPainterPath path;
        const ResultTriangleUv *uv = &triangleUvs[i];
        for (auto j = 0; j < 3; j++) {
            if (0 == j) {
                path.moveTo(uv->uv[j][0] * TextureGenerator::m_textureWidth, uv->uv[j][1] * TextureGenerator::m_textureHeight);
            } else {
                path.lineTo(uv->uv[j][0] * TextureGenerator::m_textureWidth, uv->uv[j][1] * TextureGenerator::m_textureHeight);
            }
        }
        texturePainter.fillPath(path, QBrush(triangleMaterials[i].color));
    }
    
    pen.setWidth(0);
    textureBorderPainter.setPen(pen);
    for (auto i = 0u; i < triangleUvs.size(); i++) {
        const ResultTriangleUv *uv = &triangleUvs[i];
        for (auto j = 0; j < 3; j++) {
            int from = j;
            int to = (j + 1) % 3;
            textureBorderPainter.drawLine(uv->uv[from][0] * TextureGenerator::m_textureWidth, uv->uv[from][1] * TextureGenerator::m_textureHeight,
                uv->uv[to][0] * TextureGenerator::m_textureWidth, uv->uv[to][1] * TextureGenerator::m_textureHeight);
        }
    }
    
    texturePainter.end();
    textureBorderPainter.end();
    
    m_resultTextureImage = new QImage(*m_resultTextureColorImage);
    
    m_resultTextureGuideImage = new QImage(*m_resultTextureImage);
    QPainter mergeTextureGuidePainter(m_resultTextureGuideImage);
    mergeTextureGuidePainter.setCompositionMode(QPainter::CompositionMode_Multiply);
    mergeTextureGuidePainter.drawImage(0, 0, *m_resultTextureBorderImage);
    mergeTextureGuidePainter.end();
    
    m_resultMesh = new MeshLoader(*m_resultContext);
    m_resultMesh->setTextureImage(new QImage(*m_resultTextureImage));
    
    this->moveToThread(QGuiApplication::instance()->thread());
    
    emit finished();
}
