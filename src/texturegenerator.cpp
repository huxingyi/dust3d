#include <QPainter>
#include <QGuiApplication>
#include "texturegenerator.h"
#include "theme.h"

int TextureGenerator::m_textureWidth = 256;
int TextureGenerator::m_textureHeight = 256;

TextureGenerator::TextureGenerator(const MeshResultContext &meshResultContext) :
    m_resultTextureGuideImage(nullptr),
    m_resultTextureImage(nullptr),
    m_resultTextureBorderImage(nullptr)
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

MeshResultContext *TextureGenerator::takeResultContext()
{
    MeshResultContext *resultContext = m_resultContext;
    m_resultTextureImage = nullptr;
    return resultContext;
}

void TextureGenerator::process()
{
    const std::vector<QColor> &triangleColors = m_resultContext->triangleColors();
    const std::vector<ResultTriangleUv> &triangleUvs = m_resultContext->triangleUvs();
    
    m_resultTextureGuideImage = new QImage(TextureGenerator::m_textureWidth, TextureGenerator::m_textureHeight, QImage::Format_ARGB32);
    m_resultTextureGuideImage->fill(Theme::black);

    m_resultTextureImage = new QImage(TextureGenerator::m_textureWidth, TextureGenerator::m_textureHeight, QImage::Format_ARGB32);
    m_resultTextureImage->fill(Qt::transparent);
    
    m_resultTextureBorderImage = new QImage(TextureGenerator::m_textureWidth, TextureGenerator::m_textureHeight, QImage::Format_ARGB32);
    m_resultTextureBorderImage->fill(Qt::transparent);
    
    QPainter guidePainter;
    guidePainter.begin(m_resultTextureGuideImage);
    guidePainter.setRenderHint(QPainter::Antialiasing);
    guidePainter.setRenderHint(QPainter::HighQualityAntialiasing);
    QColor borderColor = Qt::darkGray;
    QPen pen(borderColor);
    
    QPainter texturePainter;
    texturePainter.begin(m_resultTextureImage);
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
        QPen textureBorderPen(triangleColors[i]);
        textureBorderPen.setWidth(10);
        texturePainter.setPen(textureBorderPen);
        texturePainter.setBrush(QBrush(triangleColors[i]));
        texturePainter.drawPath(path);
    }
    // round 2, real paint
    texturePainter.setPen(Qt::NoPen);
    guidePainter.setPen(Qt::NoPen);
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
        guidePainter.fillPath(path, QBrush(triangleColors[i]));
        texturePainter.fillPath(path, QBrush(triangleColors[i]));
    }
    
    pen.setWidth(0);
    guidePainter.setPen(pen);
    textureBorderPainter.setPen(pen);
    for (auto i = 0u; i < triangleUvs.size(); i++) {
        const ResultTriangleUv *uv = &triangleUvs[i];
        for (auto j = 0; j < 3; j++) {
            int from = j;
            int to = (j + 1) % 3;
            guidePainter.drawLine(uv->uv[from][0] * TextureGenerator::m_textureWidth, uv->uv[from][1] * TextureGenerator::m_textureHeight,
                uv->uv[to][0] * TextureGenerator::m_textureWidth, uv->uv[to][1] * TextureGenerator::m_textureHeight);
            textureBorderPainter.drawLine(uv->uv[from][0] * TextureGenerator::m_textureWidth, uv->uv[from][1] * TextureGenerator::m_textureHeight,
                uv->uv[to][0] * TextureGenerator::m_textureWidth, uv->uv[to][1] * TextureGenerator::m_textureHeight);
        }
    }
    
    texturePainter.end();
    guidePainter.end();
    textureBorderPainter.end();
    
    this->moveToThread(QGuiApplication::instance()->thread());
    
    emit finished();
}
