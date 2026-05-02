#include "scene_widget.h"
#include "scene_ground_opengl_program.h"
#include "scene_opengl_program.h"
#include "theme.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QMouseEvent>
#include <QOpenGLContext>
#include <QOpenGLExtraFunctions>
#include <QOpenGLFunctions>
#include <QOpenGLTexture>
#include <QPainter>
#include <QSurfaceFormat>
#include <QTextStream>
#include <QVector2D>
#include <QVector4D>
#include <algorithm>
#include <array>
#include <cmath>
#include <random>
#include <unordered_map>

float SceneWidget::m_minZoomRatio = 5.0;
float SceneWidget::m_maxZoomRatio = 80.0;

int SceneWidget::m_defaultXRotation = 30 * 16;
int SceneWidget::m_defaultYRotation = -45 * 16;
int SceneWidget::m_defaultZRotation = 0;
QVector3D SceneWidget::m_defaultEyePosition = QVector3D(0.0, 1.5, -12.5);

SceneWidget::SceneWidget(QWidget* parent)
    : QOpenGLWidget(parent)
{
    QSurfaceFormat fmt = format();
    fmt.setSamples(4);
    setFormat(fmt);

    setContextMenuPolicy(Qt::CustomContextMenu);

    m_widthInPixels = width() * window()->devicePixelRatio();
    m_heightInPixels = height() * window()->devicePixelRatio();

    zoom(200);
}

SceneWidget::~SceneWidget()
{
    cleanup();
}

namespace {

std::vector<QString> loadNamesFromRepository()
{
    auto stripAngleBrackets = [](QString text) {
        for (;;) {
            int start = text.indexOf('<');
            if (start < 0)
                break;
            int end = text.indexOf('>', start + 1);
            if (end < 0)
                break;
            text.remove(start, end - start + 1);
        }
        return text.trimmed();
    };

    std::vector<QString> names;
    const QStringList files = { ":/AUTHORS", ":/CONTRIBUTORS", ":/SUPPORTERS" };
    for (const QString& filePath : files) {
        QFile file(filePath);
        if (!file.open(QFile::ReadOnly | QFile::Text))
            continue;
        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = stripAngleBrackets(in.readLine()).trimmed();
            if (line.isEmpty() || line.startsWith('#'))
                continue;
            names.push_back(line);
        }
    }
    return names;
}

QImage buildNameAtlasImage(const std::vector<QString>& names, std::vector<QRectF>& uvRects, std::vector<float>& widthFactors)
{
    uvRects.clear();
    widthFactors.clear();
    if (names.empty())
        return QImage();

    QFont font("Arial", 20, QFont::Bold);
    QFontMetrics fm(font);
    int maxWidth = 0;
    for (const QString& name : names)
        maxWidth = qMax(maxWidth, fm.horizontalAdvance(name));
    int textHeight = fm.height();
    int cellWidth = qMax(220, maxWidth + 40);
    int cellHeight = textHeight + 24;
    int columns = qMin(8, qMax(1, 4096 / cellWidth));
    columns = qMin(columns, (int)names.size());
    int rows = (names.size() + columns - 1) / columns;
    int width = columns * cellWidth;
    int height = rows * cellHeight;

    QImage image(width, height, QImage::Format_RGBA8888);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);
    painter.setFont(font);

    int nameCount = (int)names.size();
    for (int index = 0; index < nameCount; ++index) {
        int row = index / columns;
        int col = index % columns;
        QRect cellRect(col * cellWidth, row * cellHeight, cellWidth, cellHeight);
        QRect textRect = cellRect.adjusted(12, 8, -12, -8);
        painter.setPen(QColor(255, 255, 255, 100));
        painter.drawText(textRect.translated(1, 1), Qt::AlignLeft | Qt::AlignVCenter, names[index]);
        painter.setPen(QColor(24, 24, 24));
        painter.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, names[index]);
        float u = float(cellRect.x()) / float(width);
        float v = float(cellRect.y()) / float(height);
        float rectWidth = float(cellRect.width()) / float(width);
        float rectHeight = float(cellRect.height()) / float(height);
        uvRects.emplace_back(u, 1.0f - v - rectHeight, rectWidth, rectHeight);
        float labelWidth = float(fm.horizontalAdvance(names[index]) + 24);
        widthFactors.emplace_back(labelWidth / float(cellWidth));
    }
    painter.end();
    return image;
}

std::unique_ptr<ModelMesh> buildCubeScatterMesh(const std::vector<QRectF>& uvRects, const std::vector<float>& widthFactors, std::vector<SceneWidget::NameCube>* cubesOut = nullptr)
{
    struct CubeScatter {
        float x;
        float z;
        float width;
        float depth;
        float height;
        float yaw;
        dust3d::Color color;
        QRectF uvRect;
    };

    std::vector<CubeScatter> cubes;
    if (uvRects.empty()) {
        const std::array<dust3d::Color, 5> defaultColors = {
            dust3d::Color(0.949f, 0.416f, 0.302f),
            dust3d::Color(0.952f, 0.698f, 0.416f),
            dust3d::Color(0.467f, 0.780f, 0.941f),
            dust3d::Color(0.498f, 0.443f, 0.961f),
            dust3d::Color(0.345f, 0.831f, 0.647f)
        };
        cubes = std::vector<CubeScatter> {
            { -1.2f, -0.8f, 0.70f, 0.28f, 0.24f, 0.0f, defaultColors[0], QRectF(0.0f, 0.0f, 1.0f, 1.0f) },
            { 1.0f, -1.3f, 0.78f, 0.32f, 0.26f, 0.3f, defaultColors[1], QRectF(0.0f, 0.0f, 1.0f, 1.0f) },
            { 0.6f, 0.8f, 0.64f, 0.24f, 0.22f, 1.1f, defaultColors[2], QRectF(0.0f, 0.0f, 1.0f, 1.0f) },
            { -0.8f, 1.1f, 0.68f, 0.28f, 0.23f, 4.2f, defaultColors[0], QRectF(0.0f, 0.0f, 1.0f, 1.0f) },
            { 1.5f, 0.2f, 0.55f, 0.20f, 0.18f, 2.5f, defaultColors[1], QRectF(0.0f, 0.0f, 1.0f, 1.0f) }
        };
    } else {
        const int count = (int)uvRects.size();
        std::mt19937 rng(4127);
        std::vector<QRectF> shuffledUvRects = uvRects;
        std::shuffle(shuffledUvRects.begin(), shuffledUvRects.end(), rng);
        std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * 3.14159265f);
        std::uniform_real_distribution<float> radiusDist(0.0f, 1.0f);
        std::uniform_real_distribution<float> jitterDist(-0.08f, 0.08f);
        std::uniform_real_distribution<float> sizeDist(0.28f, 0.48f);
        std::uniform_real_distribution<float> uniformDist(0.0f, 1.0f);
        std::uniform_int_distribution<int> clusterCountDist(2, 5);
        std::uniform_int_distribution<int> colorIndexDist(0, 5);
        const std::array<dust3d::Color, 6> colorOptions = {
            dust3d::Color(0.949f, 0.416f, 0.302f),
            dust3d::Color(0.952f, 0.698f, 0.416f),
            dust3d::Color(0.467f, 0.780f, 0.941f),
            dust3d::Color(0.498f, 0.443f, 0.961f),
            dust3d::Color(0.345f, 0.831f, 0.647f),
            dust3d::Color(1.000f, 0.620f, 0.898f)
        };

        const float scatterRadius = 1.2f + std::sqrt((float)count) * 0.18f;
        const float noiseScale = 1.4f;
        const float clusterStdDev = 0.20f;
        const float gridCellSize = 0.75f + std::min(0.32f, std::max(0.0f, (float)count - 10.0f) * 0.010f);
        const auto makeCellKey = [](int cx, int cz) {
            return (uint64_t(uint32_t(cx)) << 32) | uint32_t(cz);
        };
        const auto cellIndex = [&](float value) {
            return int(std::floor(value / gridCellSize));
        };
        const auto getNoise = [&](float x, float z) {
            float value = std::sin(x * noiseScale) * std::cos(z * noiseScale)
                + std::sin(x * noiseScale * 0.5f);
            return (value + 1.5f) / 3.0f;
        };
        const auto randomGaussian = [&](float mean, float stdDev) {
            std::normal_distribution<float> normalDist(mean, stdDev);
            return normalDist(rng);
        };

        std::unordered_map<uint64_t, std::vector<int>> grid;
        grid.reserve(count * 2);
        cubes.reserve(count);

        auto placementRadius = [&](float testWidth, float testDepth) {
            return std::max(testWidth, testDepth) * 0.5f;
        };

        auto canPlaceAtWithBuffer = [&](float x, float z, float width, float depth, int& cellX, int& cellZ, float buffer) {
            cellX = cellIndex(x);
            cellZ = cellIndex(z);
            float myRadius = placementRadius(width, depth);
            for (int dzOffset = -1; dzOffset <= 1; ++dzOffset) {
                for (int dxOffset = -1; dxOffset <= 1; ++dxOffset) {
                    auto it = grid.find(makeCellKey(cellX + dxOffset, cellZ + dzOffset));
                    if (it == grid.end())
                        continue;
                    for (int otherIndex : it->second) {
                        const auto& other = cubes[otherIndex];
                        float otherRadius = placementRadius(other.width, other.depth);
                        float minDistance = myRadius + otherRadius + buffer;
                        float dx = x - other.x;
                        float dz = z - other.z;
                        if (dx * dx + dz * dz < minDistance * minDistance)
                            return false;
                    }
                }
            }
            return true;
        };

        auto canPlaceAt = [&](float x, float z, float width, float depth, int& cellX, int& cellZ) {
            return canPlaceAtWithBuffer(x, z, width, depth, cellX, cellZ, 0.16f);
        };

        int used = 0;
        int clusterCount = std::clamp((count + 4) / 5, 2, 5);
        std::vector<std::pair<float, float>> clusterCenters;
        clusterCenters.reserve(clusterCount);
        const float clusterBaseRadius = scatterRadius * 0.50f;
        const float clusterRadiusStep = scatterRadius * 0.28f;
        for (int i = 0; i < clusterCount; ++i) {
            float angle = 2.0f * float(M_PI) * float(i) / float(clusterCount);
            float radius = clusterBaseRadius + clusterRadiusStep * i;
            float cx = std::cos(angle) * radius;
            float cz = std::sin(angle) * radius;
            clusterCenters.emplace_back(cx, cz);
        }

        int attempts = 0;
        for (int clusterIndex = 0; clusterIndex < clusterCount && used < count; ++clusterIndex) {
            int clusterSize = count / clusterCount + (clusterIndex < count % clusterCount ? 1 : 0);
            float centerX = clusterCenters[clusterIndex].first;
            float centerZ = clusterCenters[clusterIndex].second;
            for (int child = 0; child < clusterSize && used < count; ++child) {
                float size = sizeDist(rng);
                float widthFactor = 1.0f;
                if (used < (int)widthFactors.size())
                    widthFactor = widthFactors[used];
                widthFactor = std::clamp(widthFactor, 0.40f, 1.0f);
                float width = size * 2.3f * widthFactor;
                float depth = size * 0.75f;
                float height = size * 1.05f;
                float x = centerX + randomGaussian(0.0f, clusterStdDev * 0.30f);
                float z = centerZ + randomGaussian(0.0f, clusterStdDev * 0.30f);
                int cellX = 0;
                int cellZ = 0;

                if (!canPlaceAtWithBuffer(x, z, width, depth, cellX, cellZ, 0.04f)) {
                    int attempt = 0;
                    for (; attempt < 25; ++attempt) {
                        x = centerX + randomGaussian(0.0f, clusterStdDev * 0.40f);
                        z = centerZ + randomGaussian(0.0f, clusterStdDev * 0.40f);
                        if (canPlaceAtWithBuffer(x, z, width, depth, cellX, cellZ, 0.04f))
                            break;
                    }
                    if (attempt >= 25)
                        continue;
                }

                float yaw = angleDist(rng);
                dust3d::Color castColor = colorOptions[colorIndexDist(rng)];
                int currentIndex = (int)cubes.size();
                cubes.push_back({ x, z, width, depth, height, yaw, castColor, shuffledUvRects[used] });
                grid[makeCellKey(cellX, cellZ)].push_back(currentIndex);
                used++;
            }
        }

        while (used < count && attempts < count * 15) {
            attempts++;
            float x = (uniformDist(rng) * 2.0f - 1.0f) * scatterRadius;
            float z = (uniformDist(rng) * 2.0f - 1.0f) * scatterRadius;
            float density = getNoise(x, z);
            density = std::pow(density, 3.0f);
            if (uniformDist(rng) >= density)
                continue;

            float size = sizeDist(rng);
            float widthFactor = 1.0f;
            if (used < (int)widthFactors.size())
                widthFactor = widthFactors[used];
            widthFactor = std::clamp(widthFactor, 0.40f, 1.0f);
            float width = size * 2.3f * widthFactor;
            float depth = size * 0.75f;
            float height = size * 1.05f;
            int cellX = 0;
            int cellZ = 0;

            if (!canPlaceAt(x, z, width, depth, cellX, cellZ))
                continue;

            float yaw = angleDist(rng);
            dust3d::Color castColor = colorOptions[colorIndexDist(rng)];
            int currentIndex = (int)cubes.size();
            cubes.push_back({ x, z, width, depth, height, yaw, castColor, shuffledUvRects[used] });
            grid[makeCellKey(cellX, cellZ)].push_back(currentIndex);
            used++;
        }

        for (int index = used; index < count; ++index) {
            float size = sizeDist(rng);
            float widthFactor = 1.0f;
            if (index < (int)widthFactors.size())
                widthFactor = widthFactors[index];
            widthFactor = std::clamp(widthFactor, 0.40f, 1.0f);
            float width = size * 2.3f * widthFactor;
            float depth = size * 0.75f;
            float height = size * 1.05f;
            float x = 0.0f;
            float z = 0.0f;
            bool placed = false;
            int cellX = 0;
            int cellZ = 0;
            for (int attempt = 0; attempt < 50; ++attempt) {
                float angle = angleDist(rng);
                float radius = std::sqrt(radiusDist(rng)) * scatterRadius + attempt * 0.015f;
                x = std::cos(angle) * radius + jitterDist(rng);
                z = std::sin(angle) * radius + jitterDist(rng);
                if (canPlaceAt(x, z, width, depth, cellX, cellZ)) {
                    placed = true;
                    break;
                }
            }
            if (!placed) {
                float angle = angleDist(rng);
                float radius = 0.75f + index * 0.06f;
                x = std::cos(angle) * radius;
                z = std::sin(angle) * radius;
                cellX = cellIndex(x);
                cellZ = cellIndex(z);
            }
            float yaw = angleDist(rng);
            dust3d::Color castColor = colorOptions[colorIndexDist(rng)];
            int currentIndex = (int)cubes.size();
            cubes.push_back({ x, z, width, depth, height, yaw, castColor, shuffledUvRects[index] });
            grid[makeCellKey(cellX, cellZ)].push_back(currentIndex);
        }
    }
    if (cubesOut) {
        cubesOut->clear();
        cubesOut->reserve(cubes.size());
        for (const auto& cube : cubes) {
            cubesOut->push_back({ cube.x, cube.z, cube.width, cube.depth, cube.height });
        }
    }

    std::vector<ModelOpenGLVertex> triangleVertices;
    triangleVertices.reserve(4096);

    const float metalness = 0.1f;
    const float roughness = 0.8f;

    auto appendTriangle = [&](const dust3d::Vector3& a,
                              const dust3d::Vector3& b,
                              const dust3d::Vector3& c,
                              const std::array<QVector2D, 3>& uvTri,
                              const dust3d::Color& faceColor,
                              const dust3d::Vector3& interiorPoint) {
        auto normal = dust3d::Vector3::normal(a, b, c);
        dust3d::Vector3 centroid((a.x() + b.x() + c.x()) / 3.0,
            (a.y() + b.y() + c.y()) / 3.0,
            (a.z() + b.z() + c.z()) / 3.0);
        if (dust3d::Vector3::dotProduct(normal, centroid - interiorPoint) < 0.0) {
            normal = dust3d::Vector3::normal(a, c, b);
            std::array<QVector2D, 3> flippedUvs = { uvTri[0], uvTri[2], uvTri[1] };
            for (int vertexIndex = 0; vertexIndex < 3; ++vertexIndex) {
                const auto& src = (vertexIndex == 0 ? a : vertexIndex == 1 ? c
                                                                           : b);
                const auto& texCoord = flippedUvs[vertexIndex];
                ModelOpenGLVertex dest;
                dest.posX = static_cast<GLfloat>(src.x());
                dest.posY = static_cast<GLfloat>(src.y());
                dest.posZ = static_cast<GLfloat>(src.z());
                dest.normX = static_cast<GLfloat>(normal.x());
                dest.normY = static_cast<GLfloat>(normal.y());
                dest.normZ = static_cast<GLfloat>(normal.z());
                dest.colorR = static_cast<GLfloat>(faceColor.r());
                dest.colorG = static_cast<GLfloat>(faceColor.g());
                dest.colorB = static_cast<GLfloat>(faceColor.b());
                dest.texU = static_cast<GLfloat>(texCoord.x());
                dest.texV = static_cast<GLfloat>(texCoord.y());
                dest.metalness = metalness;
                dest.roughness = roughness;
                dest.tangentX = 0.0f;
                dest.tangentY = 0.0f;
                dest.tangentZ = 0.0f;
                dest.alpha = 1.0f;
                triangleVertices.push_back(dest);
            }
            return;
        }
        for (int vertexIndex = 0; vertexIndex < 3; ++vertexIndex) {
            const auto& src = (vertexIndex == 0 ? a : vertexIndex == 1 ? b
                                                                       : c);
            const auto& texCoord = uvTri[vertexIndex];
            ModelOpenGLVertex dest;
            dest.posX = static_cast<GLfloat>(src.x());
            dest.posY = static_cast<GLfloat>(src.y());
            dest.posZ = static_cast<GLfloat>(src.z());
            dest.normX = static_cast<GLfloat>(normal.x());
            dest.normY = static_cast<GLfloat>(normal.y());
            dest.normZ = static_cast<GLfloat>(normal.z());
            dest.colorR = static_cast<GLfloat>(faceColor.r());
            dest.colorG = static_cast<GLfloat>(faceColor.g());
            dest.colorB = static_cast<GLfloat>(faceColor.b());
            dest.texU = static_cast<GLfloat>(texCoord.x());
            dest.texV = static_cast<GLfloat>(texCoord.y());
            dest.metalness = metalness;
            dest.roughness = roughness;
            dest.tangentX = 0.0f;
            dest.tangentY = 0.0f;
            dest.tangentZ = 0.0f;
            dest.alpha = 1.0f;
            triangleVertices.push_back(dest);
        }
    };

    auto appendFace = [&](const std::array<dust3d::Vector3, 4>& vertices,
                          const QRectF& uvRect,
                          const dust3d::Color& faceColor,
                          const dust3d::Vector3& interiorPoint) {
        const std::array<std::array<dust3d::Vector3, 3>, 2> faces = { { std::array<dust3d::Vector3, 3> { vertices[0], vertices[1], vertices[2] },
            std::array<dust3d::Vector3, 3> { vertices[0], vertices[2], vertices[3] } } };
        const std::array<QVector2D, 4> uvCorners = { { QVector2D(uvRect.left(), uvRect.bottom()),
            QVector2D(uvRect.right(), uvRect.bottom()),
            QVector2D(uvRect.right(), uvRect.top()),
            QVector2D(uvRect.left(), uvRect.top()) } };
        const std::array<std::array<QVector2D, 3>, 2> faceUvs = { { std::array<QVector2D, 3> { uvCorners[0], uvCorners[1], uvCorners[2] },
            std::array<QVector2D, 3> { uvCorners[0], uvCorners[2], uvCorners[3] } } };
        for (int faceIndex = 0; faceIndex < 2; ++faceIndex)
            appendTriangle(faces[faceIndex][0], faces[faceIndex][1], faces[faceIndex][2], faceUvs[faceIndex], faceColor, interiorPoint);
    };

    for (const auto& cube : cubes) {
        float halfWidth = cube.width * 0.5f;
        float halfDepth = cube.depth * 0.5f;
        float y0 = 0.0f;
        float y1 = cube.height;
        float cosYaw = std::cos(cube.yaw);
        float sinYaw = std::sin(cube.yaw);

        auto rotated = [&](float localX, float localZ, float y) {
            float worldX = localX * cosYaw - localZ * sinYaw;
            float worldZ = localX * sinYaw + localZ * cosYaw;
            return dust3d::Vector3(cube.x + worldX, y, cube.z + worldZ);
        };

        std::array<dust3d::Vector3, 8> corners = {
            rotated(-halfWidth, -halfDepth, y0),
            rotated(halfWidth, -halfDepth, y0),
            rotated(halfWidth, halfDepth, y0),
            rotated(-halfWidth, halfDepth, y0),
            rotated(-halfWidth, -halfDepth, y1),
            rotated(halfWidth, -halfDepth, y1),
            rotated(halfWidth, halfDepth, y1),
            rotated(-halfWidth, halfDepth, y1)
        };

        const QRectF blankUvRect(0.0f, 0.0f, 0.0f, 0.0f);
        const dust3d::Vector3 interiorPoint(cube.x, (y0 + y1) * 0.5f, cube.z);
        appendFace({ corners[4], corners[7], corners[6], corners[5] }, blankUvRect, cube.color, interiorPoint); // top
        appendFace({ corners[0], corners[1], corners[2], corners[3] }, blankUvRect, cube.color, interiorPoint); // bottom
        appendFace({ corners[0], corners[3], corners[7], corners[4] }, blankUvRect, cube.color, interiorPoint); // left
        appendFace({ corners[1], corners[5], corners[6], corners[2] }, blankUvRect, cube.color, interiorPoint); // right
        appendFace({ corners[3], corners[2], corners[6], corners[7] }, cube.uvRect, cube.color, interiorPoint); // front
        appendFace({ corners[1], corners[0], corners[4], corners[5] }, blankUvRect, cube.color, interiorPoint); // back
    }

    if (triangleVertices.empty())
        return std::make_unique<ModelMesh>();

    auto mesh = std::make_unique<ModelMesh>(new ModelOpenGLVertex[triangleVertices.size()], (int)triangleVertices.size());
    std::copy(triangleVertices.begin(), triangleVertices.end(), mesh->triangleVertices());
    return mesh;
}

static QVector2D rotateXz(float x, float z, float yaw)
{
    float cosYaw = std::cos(yaw);
    float sinYaw = std::sin(yaw);
    return QVector2D(x * cosYaw - z * sinYaw, x * sinYaw + z * cosYaw);
}

static std::array<QVector2D, 4> makeCubeFootprint(float x, float z, float halfWidth, float halfDepth, float yaw)
{
    return std::array<QVector2D, 4> {
        rotateXz(-halfWidth, -halfDepth, yaw) + QVector2D(x, z),
        rotateXz(halfWidth, -halfDepth, yaw) + QVector2D(x, z),
        rotateXz(halfWidth, halfDepth, yaw) + QVector2D(x, z),
        rotateXz(-halfWidth, halfDepth, yaw) + QVector2D(x, z)
    };
}

static void projectOnAxis(const std::array<QVector2D, 4>& points, const QVector2D& axis, float& minValue, float& maxValue)
{
    minValue = maxValue = QVector2D::dotProduct(points[0], axis);
    for (int i = 1; i < 4; ++i) {
        float projection = QVector2D::dotProduct(points[i], axis);
        minValue = std::min(minValue, projection);
        maxValue = std::max(maxValue, projection);
    }
}

static bool testSeparatingAxis(const std::array<QVector2D, 4>& a,
    const std::array<QVector2D, 4>& b,
    const QVector2D& axis)
{
    float minA, maxA, minB, maxB;
    projectOnAxis(a, axis, minA, maxA);
    projectOnAxis(b, axis, minB, maxB);
    return !(maxA < minB || maxB < minA);
}

static bool footprintsOverlap(float ax, float az, float awidth, float adepth, float ayaw,
    float bx, float bz, float bwidth, float bdepth, float byaw)
{
    const auto aCorners = makeCubeFootprint(ax, az, awidth * 0.5f, adepth * 0.5f, ayaw);
    const auto bCorners = makeCubeFootprint(bx, bz, bwidth * 0.5f, bdepth * 0.5f, byaw);

    std::array<QVector2D, 4> axes = {
        QVector2D(aCorners[1].x() - aCorners[0].x(), aCorners[1].y() - aCorners[0].y()),
        QVector2D(aCorners[3].x() - aCorners[0].x(), aCorners[3].y() - aCorners[0].y()),
        QVector2D(bCorners[1].x() - bCorners[0].x(), bCorners[1].y() - bCorners[0].y()),
        QVector2D(bCorners[3].x() - bCorners[0].x(), bCorners[3].y() - bCorners[0].y())
    };

    for (const QVector2D& edge : axes) {
        QVector2D axis(-edge.y(), edge.x());
        if (!testSeparatingAxis(aCorners, bCorners, axis))
            return false;
    }
    return true;
}

static float findSupportHeight(const SceneWidget::PhysicsCube& cube,
    const std::vector<SceneWidget::PhysicsCube>& cubes,
    int selfIndex)
{
    float height = 0.0f;
    for (int index = 0; index < (int)cubes.size(); ++index) {
        if (index == selfIndex)
            continue;
        const auto& other = cubes[index];
        if (other.y + other.height > cube.y + 0.02f)
            continue;
        if (footprintsOverlap(cube.x, cube.z, cube.width, cube.depth, cube.yaw,
                other.x, other.z, other.width, other.depth, other.yaw)) {
            height = std::max(height, other.y + other.height);
        }
    }
    return height;
}

static int countCubeSupports(const SceneWidget::PhysicsCube& cube,
    const std::vector<SceneWidget::PhysicsCube>& cubes,
    int selfIndex)
{
    int count = 0;
    for (int index = 0; index < (int)cubes.size(); ++index) {
        if (index == selfIndex)
            continue;
        const auto& other = cubes[index];
        if (!footprintsOverlap(cube.x, cube.z, cube.width, cube.depth, cube.yaw,
                other.x, other.z, other.width, other.depth, other.yaw))
            continue;
        float top = other.y + other.height;
        if (std::abs(cube.y - top) < 0.03f)
            count++;
    }
    return count;
}

static std::unique_ptr<ModelMesh> buildPhysicsCubeMesh(const std::vector<SceneWidget::PhysicsCube>& cubes,
    std::vector<SceneWidget::NameCube>* cubesOut = nullptr)
{
    std::vector<ModelOpenGLVertex> triangleVertices;
    triangleVertices.reserve(4096);

    const float metalness = 0.1f;
    const float roughness = 0.8f;

    auto appendTriangle = [&](const dust3d::Vector3& v0,
                              const dust3d::Vector3& v1,
                              const dust3d::Vector3& v2,
                              const std::array<QVector2D, 3>& uvTri,
                              const dust3d::Color& faceColor,
                              const dust3d::Vector3& interiorPoint) {
        auto normal = dust3d::Vector3::normal(v0, v1, v2);
        dust3d::Vector3 centroid((v0.x() + v1.x() + v2.x()) / 3.0,
            (v0.y() + v1.y() + v2.y()) / 3.0,
            (v0.z() + v1.z() + v2.z()) / 3.0);
        std::array<dust3d::Vector3, 3> tri = { { v0, v1, v2 } };
        std::array<QVector2D, 3> uv = uvTri;
        if (dust3d::Vector3::dotProduct(normal, centroid - interiorPoint) < 0.0) {
            normal = dust3d::Vector3::normal(v0, v2, v1);
            tri = { { v0, v2, v1 } };
            uv = { uvTri[0], uvTri[2], uvTri[1] };
        }
        for (int vertexIndex = 0; vertexIndex < 3; ++vertexIndex) {
            const auto& src = tri[vertexIndex];
            const auto& texCoord = uv[vertexIndex];
            ModelOpenGLVertex dest;
            dest.posX = static_cast<GLfloat>(src.x());
            dest.posY = static_cast<GLfloat>(src.y());
            dest.posZ = static_cast<GLfloat>(src.z());
            dest.normX = static_cast<GLfloat>(normal.x());
            dest.normY = static_cast<GLfloat>(normal.y());
            dest.normZ = static_cast<GLfloat>(normal.z());
            dest.colorR = static_cast<GLfloat>(faceColor.r());
            dest.colorG = static_cast<GLfloat>(faceColor.g());
            dest.colorB = static_cast<GLfloat>(faceColor.b());
            dest.texU = static_cast<GLfloat>(texCoord.x());
            dest.texV = static_cast<GLfloat>(texCoord.y());
            dest.metalness = metalness;
            dest.roughness = roughness;
            dest.tangentX = 0.0f;
            dest.tangentY = 0.0f;
            dest.tangentZ = 0.0f;
            dest.alpha = 1.0f;
            triangleVertices.push_back(dest);
        }
    };

    auto appendFace = [&](const std::array<dust3d::Vector3, 4>& vertices,
                          const QRectF& uvRect,
                          const dust3d::Color& faceColor,
                          const dust3d::Vector3& interiorPoint) {
        const std::array<std::array<dust3d::Vector3, 3>, 2> faces = { { std::array<dust3d::Vector3, 3> { vertices[0], vertices[1], vertices[2] },
            std::array<dust3d::Vector3, 3> { vertices[0], vertices[2], vertices[3] } } };
        const std::array<QVector2D, 4> uvCorners = { { QVector2D(uvRect.left(), uvRect.bottom()),
            QVector2D(uvRect.right(), uvRect.bottom()),
            QVector2D(uvRect.right(), uvRect.top()),
            QVector2D(uvRect.left(), uvRect.top()) } };
        const std::array<std::array<QVector2D, 3>, 2> faceUvs = { { std::array<QVector2D, 3> { uvCorners[0], uvCorners[1], uvCorners[2] },
            std::array<QVector2D, 3> { uvCorners[0], uvCorners[2], uvCorners[3] } } };
        for (int faceIndex = 0; faceIndex < 2; ++faceIndex)
            appendTriangle(faces[faceIndex][0], faces[faceIndex][1], faces[faceIndex][2], faceUvs[faceIndex], faceColor, interiorPoint);
    };

    auto makeUvForCirclePoint = [&](float localX, float localZ, float radius, const QRectF& uvRect) {
        if (uvRect.width() <= 0.0f || uvRect.height() <= 0.0f)
            return QVector2D(0.0f, 0.0f);
        float u = (localX / radius * 0.5f + 0.5f) * uvRect.width() + uvRect.left();
        float v = (localZ / radius * 0.5f + 0.5f) * uvRect.height() + uvRect.top();
        return QVector2D(u, v);
    };

    if (cubesOut) {
        cubesOut->clear();
        cubesOut->reserve(cubes.size());
        for (const auto& cube : cubes)
            cubesOut->push_back({ cube.x, cube.z, cube.width, cube.depth, cube.height });
    }

    for (const auto& cube : cubes) {
        float y0 = cube.y;
        float y1 = cube.y + cube.height;

        const float cyaw = std::cos(cube.yaw);
        const float syaw = std::sin(cube.yaw);
        const float cpitch = std::cos(cube.pitch);
        const float spitch = std::sin(cube.pitch);
        const float croll = std::cos(cube.roll);
        const float sroll = std::sin(cube.roll);

        auto rotated = [&](float localX, float localZ, float y) {
            // yaw around Y, pitch around X, roll around Z
            float x1 = localX * cyaw - localZ * syaw;
            float z1 = localX * syaw + localZ * cyaw;
            float y1r = y;

            float y2 = y1r * cpitch - z1 * spitch;
            float z2 = y1r * spitch + z1 * cpitch;
            float x2 = x1;

            float x3 = x2 * croll - y2 * sroll;
            float y3 = x2 * sroll + y2 * croll;
            float z3 = z2;

            return dust3d::Vector3(cube.x + x3, y3, cube.z + z3);
        };

        const QRectF blankUvRect(0.0f, 0.0f, 0.0f, 0.0f);
        const dust3d::Vector3 interiorPoint(cube.x, (y0 + y1) * 0.5f, cube.z);
        if (cube.shapeType == SceneWidget::PhysicsCube::ShapeType::Cuboid) {
            float halfWidth = cube.width * 0.5f;
            float halfDepth = cube.depth * 0.5f;
            std::array<dust3d::Vector3, 8> corners = {
                rotated(-halfWidth, -halfDepth, y0),
                rotated(halfWidth, -halfDepth, y0),
                rotated(halfWidth, halfDepth, y0),
                rotated(-halfWidth, halfDepth, y0),
                rotated(-halfWidth, -halfDepth, y1),
                rotated(halfWidth, -halfDepth, y1),
                rotated(halfWidth, halfDepth, y1),
                rotated(-halfWidth, halfDepth, y1)
            };

            appendFace({ corners[4], corners[7], corners[6], corners[5] }, blankUvRect, cube.color, interiorPoint);
            appendFace({ corners[0], corners[1], corners[2], corners[3] }, blankUvRect, cube.color, interiorPoint);
            appendFace({ corners[0], corners[3], corners[7], corners[4] }, blankUvRect, cube.color, interiorPoint);
            appendFace({ corners[1], corners[5], corners[6], corners[2] }, blankUvRect, cube.color, interiorPoint);
            appendFace({ corners[3], corners[2], corners[6], corners[7] }, cube.uvRect, cube.color, interiorPoint);
            appendFace({ corners[1], corners[0], corners[4], corners[5] }, blankUvRect, cube.color, interiorPoint);
        } else {
            float radius = cube.width * 0.5f;
            float topRadius = cube.shapeType == SceneWidget::PhysicsCube::ShapeType::Cone ? 0.0f : radius;
            const int segmentCount = 16;
            std::vector<dust3d::Vector3> bottomRing(segmentCount);
            std::vector<dust3d::Vector3> topRing(segmentCount);
            std::vector<QVector2D> bottomUvs(segmentCount);
            std::vector<QVector2D> topUvs(segmentCount);
            const QVector2D uvCenter(cube.uvRect.left() + cube.uvRect.width() * 0.5f,
                cube.uvRect.top() + cube.uvRect.height() * 0.5f);
            for (int i = 0; i < segmentCount; ++i) {
                float angle = float(2.0f * M_PI * i / segmentCount);
                float cosA = std::cos(angle);
                float sinA = std::sin(angle);
                bottomRing[i] = rotated(cosA * radius, sinA * radius, y0);
                topRing[i] = rotated(cosA * topRadius, sinA * topRadius, y1);
                bottomUvs[i] = makeUvForCirclePoint(cosA * radius, sinA * radius, radius, cube.uvRect);
                topUvs[i] = makeUvForCirclePoint(cosA * radius, sinA * radius, radius, cube.uvRect);
            }

            if (cube.shapeType == SceneWidget::PhysicsCube::ShapeType::Tube) {
                for (int i = 0; i < segmentCount; ++i) {
                    int next = (i + 1) % segmentCount;
                    const auto& p0 = bottomRing[i];
                    const auto& p1 = bottomRing[next];
                    const auto& p2 = topRing[next];
                    const auto& p3 = topRing[i];
                    appendFace({ p0, p1, p2, p3 }, blankUvRect, cube.color, interiorPoint);
                }

                dust3d::Vector3 topCenter = rotated(0.0f, 0.0f, y1);
                for (int i = 0; i < segmentCount; ++i) {
                    int next = (i + 1) % segmentCount;
                    appendTriangle(topCenter, topRing[i], topRing[next], { uvCenter, topUvs[i], topUvs[next] }, cube.color, interiorPoint);
                }

                dust3d::Vector3 bottomCenter = rotated(0.0f, 0.0f, y0);
                for (int i = 0; i < segmentCount; ++i) {
                    int next = (i + 1) % segmentCount;
                    appendTriangle(bottomCenter, bottomRing[next], bottomRing[i], { uvCenter, bottomUvs[next], bottomUvs[i] }, cube.color, interiorPoint);
                }
            } else {
                dust3d::Vector3 apex = rotated(0.0f, 0.0f, y1);
                for (int i = 0; i < segmentCount; ++i) {
                    int next = (i + 1) % segmentCount;
                    appendTriangle(bottomRing[i], bottomRing[next], apex, { bottomUvs[i], bottomUvs[next], uvCenter }, cube.color, interiorPoint);
                }

                dust3d::Vector3 bottomCenter = rotated(0.0f, 0.0f, y0);
                for (int i = 0; i < segmentCount; ++i) {
                    int next = (i + 1) % segmentCount;
                    appendTriangle(bottomCenter, bottomRing[next], bottomRing[i], { uvCenter, bottomUvs[next], bottomUvs[i] }, cube.color, interiorPoint);
                }
            }
        }
    }

    if (triangleVertices.empty())
        return std::make_unique<ModelMesh>();

    auto mesh = std::make_unique<ModelMesh>(new ModelOpenGLVertex[triangleVertices.size()], (int)triangleVertices.size());
    std::copy(triangleVertices.begin(), triangleVertices.end(), mesh->triangleVertices());
    return mesh;
}

}

const QVector3D& SceneWidget::eyePosition()
{
    return m_eyePosition;
}

const std::vector<SceneWidget::NameCube>& SceneWidget::nameCubes() const
{
    return m_nameCubes;
}

const QVector3D& SceneWidget::moveToPosition()
{
    return m_moveToPosition;
}

std::vector<QVector2D> SceneWidget::dropSpawnCenters() const
{
    return {
        QVector2D(-1.3f, -0.6f),
        QVector2D(0.2f, 1.3f),
        QVector2D(1.25f, -0.35f)
    };
}

int SceneWidget::xRot() { return m_xRot; }
int SceneWidget::yRot() { return m_yRot; }
int SceneWidget::zRot() { return m_zRot; }

void SceneWidget::setEyePosition(const QVector3D& eyePosition)
{
    m_eyePosition = eyePosition;
    emit eyePositionChanged(m_eyePosition);
    update();
}

void SceneWidget::setMoveToPosition(const QVector3D& moveToPosition)
{
    m_moveToPosition = moveToPosition;
}

void SceneWidget::reRender()
{
    emit renderParametersChanged();
    update();
}

void SceneWidget::setXRotation(int angle)
{
    normalizeAngle(angle);
    if (angle != m_xRot) {
        m_xRot = angle;
        emit xRotationChanged(angle);
        emit renderParametersChanged();
        update();
    }
}

void SceneWidget::setYRotation(int angle)
{
    normalizeAngle(angle);
    if (angle != m_yRot) {
        m_yRot = angle;
        emit yRotationChanged(angle);
        emit renderParametersChanged();
        update();
    }
}

void SceneWidget::setZRotation(int angle)
{
    normalizeAngle(angle);
    if (angle != m_zRot) {
        m_zRot = angle;
        emit zRotationChanged(angle);
        emit renderParametersChanged();
        update();
    }
}

void SceneWidget::cleanup()
{
    if (!m_shadowFBOId && !m_worldOpenGLProgram)
        return;

    // During widget teardown the GL context can already be gone.
    // Only issue GL deletes when we actually have a current context.
    const bool canUseGlContext = nullptr != context() && context()->isValid();
    if (canUseGlContext)
        makeCurrent();

    if (m_physicsTimer) {
        m_physicsTimer->stop();
        delete m_physicsTimer;
        m_physicsTimer = nullptr;
    }

    if (nullptr != QOpenGLContext::currentContext()) {
        cleanupShadowFBO();
    } else {
        m_shadowDepthTexture = 0;
        m_shadowFBOId = 0;
    }

    m_modelOpenGLObject.reset();
    m_shadowOpenGLProgram.reset();
    m_worldOpenGLProgram.reset();
    m_groundOpenGLProgram.reset();
    m_groundOpenGLObject.reset();
    m_wireframeOpenGLObject.reset();
    m_monochromeOpenGLProgram.reset();
    if (m_nameAtlasTexture) {
        if (canUseGlContext && nullptr != QOpenGLContext::currentContext())
            m_nameAtlasTexture->destroy();
        m_nameAtlasTexture.reset();
    }
    if (canUseGlContext && nullptr != QOpenGLContext::currentContext())
        doneCurrent();
}

void SceneWidget::cleanupShadowFBO()
{
    QOpenGLContext* currentContext = QOpenGLContext::currentContext();
    if (nullptr == currentContext)
        return;
    QOpenGLFunctions* f = currentContext->functions();
    if (m_shadowDepthTexture) {
        f->glDeleteTextures(1, &m_shadowDepthTexture);
        m_shadowDepthTexture = 0;
    }
    if (m_shadowFBOId) {
        f->glDeleteFramebuffers(1, &m_shadowFBOId);
        m_shadowFBOId = 0;
    }
}

void SceneWidget::updateProjectionMatrix()
{
    m_projection.setToIdentity();
    m_projection.translate(m_moveToPosition.x(), m_moveToPosition.y(), m_moveToPosition.z());
    m_projection.perspective(45.0f, GLfloat(width()) / height(), 0.01f, 100.0f);
}

void SceneWidget::normalizeAngle(int& angle)
{
    while (angle < 0)
        angle += 360 * 16;
    while (angle > 360 * 16)
        angle -= 360 * 16;
}

void SceneWidget::enableMove(bool enabled)
{
    m_moveEnabled = enabled;
}

void SceneWidget::enableZoom(bool enabled)
{
    m_zoomEnabled = enabled;
}

void SceneWidget::setMoveAndZoomByWindow(bool byWindow)
{
    m_moveAndZoomByWindow = byWindow;
}

void SceneWidget::updateMesh(ModelMesh* mesh)
{
    // Create the program early (before paintGL) so texture images can be stored.
    if (!m_worldOpenGLProgram)
        m_worldOpenGLProgram = std::make_unique<SceneOpenGLProgram>();

    m_modelTextureImage.reset(mesh != nullptr ? mesh->takeTextureImage() : nullptr);
    m_modelNormalMapImage.reset(mesh != nullptr ? mesh->takeNormalMapImage() : nullptr);
    m_modelMetalnessRoughnessAmbientOcclusionMapImage.reset(mesh != nullptr ? mesh->takeMetalnessRoughnessAmbientOcclusionMapImage() : nullptr);
    m_modelHasMetalnessInImage = mesh && mesh->hasMetalnessInImage();
    m_modelHasRoughnessInImage = mesh && mesh->hasRoughnessInImage();
    m_modelHasAmbientOcclusionInImage = mesh && mesh->hasAmbientOcclusionInImage();

    if (!m_modelOpenGLObject)
        m_modelOpenGLObject = std::make_unique<ModelOpenGLObject>();
    m_modelOpenGLObject->update(std::unique_ptr<ModelMesh>(mesh));

    emit renderParametersChanged();
    update();
}

void SceneWidget::updatePreviewMesh(int index, ModelMesh* mesh)
{
    if (!m_worldOpenGLProgram)
        m_worldOpenGLProgram = std::make_unique<SceneOpenGLProgram>();

    if (index < 0)
        return;

    int targetSize = index + 1;
    if (static_cast<int>(m_previewOpenGLObjects.size()) < targetSize)
        m_previewOpenGLObjects.resize(targetSize);
    if (static_cast<int>(m_previewTextureImages.size()) < targetSize)
        m_previewTextureImages.resize(targetSize);
    if (static_cast<int>(m_previewNormalMapImages.size()) < targetSize)
        m_previewNormalMapImages.resize(targetSize);
    if (static_cast<int>(m_previewMetalnessRoughnessAmbientOcclusionMapImages.size()) < targetSize)
        m_previewMetalnessRoughnessAmbientOcclusionMapImages.resize(targetSize);
    if (static_cast<int>(m_previewHasMetalnessInImage.size()) < targetSize)
        m_previewHasMetalnessInImage.resize(targetSize);
    if (static_cast<int>(m_previewHasRoughnessInImage.size()) < targetSize)
        m_previewHasRoughnessInImage.resize(targetSize);
    if (static_cast<int>(m_previewHasAmbientOcclusionInImage.size()) < targetSize)
        m_previewHasAmbientOcclusionInImage.resize(targetSize);

    m_previewTextureImages[index].reset(mesh != nullptr ? mesh->takeTextureImage() : nullptr);
    m_previewNormalMapImages[index].reset(mesh != nullptr ? mesh->takeNormalMapImage() : nullptr);
    m_previewMetalnessRoughnessAmbientOcclusionMapImages[index].reset(mesh != nullptr ? mesh->takeMetalnessRoughnessAmbientOcclusionMapImage() : nullptr);
    m_previewHasMetalnessInImage[index] = mesh && mesh->hasMetalnessInImage();
    m_previewHasRoughnessInImage[index] = mesh && mesh->hasRoughnessInImage();
    m_previewHasAmbientOcclusionInImage[index] = mesh && mesh->hasAmbientOcclusionInImage();

    if (!m_previewOpenGLObjects[index])
        m_previewOpenGLObjects[index] = std::make_unique<ModelOpenGLObject>();
    m_previewOpenGLObjects[index]->update(std::unique_ptr<ModelMesh>(mesh));

    emit renderParametersChanged();
    update();
}

void SceneWidget::updateWireframeMesh(MonochromeMesh* mesh)
{
    if (!m_wireframeOpenGLObject)
        m_wireframeOpenGLObject = std::make_unique<MonochromeOpenGLObject>();
    m_wireframeOpenGLObject->update(std::unique_ptr<MonochromeMesh>(mesh));
    emit renderParametersChanged();
    update();
}

void SceneWidget::setGroundOffset(float offsetX, float offsetZ)
{
    m_groundOffsetX = offsetX;
    m_groundOffsetZ = offsetZ;
}

void SceneWidget::startDropSimulation(const std::vector<QRectF>& uvRects, const std::vector<float>& widthFactors)
{
    if (m_physicsTimer) {
        m_physicsTimer->stop();
        delete m_physicsTimer;
        m_physicsTimer = nullptr;
    }

    m_physicsCubes.clear();
    m_activePhysicsCubeIndex = 0;

    const int count = std::max(18, int(uvRects.size()));
    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_int_distribution<int> spawnIndexDist(0, 2);
    std::uniform_int_distribution<int> colorIndexDist(0, 5);
    std::uniform_real_distribution<float> jitterDist(-0.26f, 0.26f);
    std::uniform_real_distribution<float> yawDist(0.0f, 2.0f * float(M_PI));
    std::uniform_real_distribution<float> sizeDist(0.28f, 0.52f);
    std::discrete_distribution<int> shapeDist({ 90, 5, 5 });

    const std::array<QVector2D, 3> spawnCenters = {
        QVector2D(-1.3f, -0.6f),
        QVector2D(0.2f, 1.3f),
        QVector2D(1.25f, -0.35f)
    };

    const std::array<dust3d::Color, 6> colorOptions = {
        dust3d::Color(0.949f, 0.416f, 0.302f),
        dust3d::Color(0.952f, 0.698f, 0.416f),
        dust3d::Color(0.467f, 0.780f, 0.941f),
        dust3d::Color(0.498f, 0.443f, 0.961f),
        dust3d::Color(0.345f, 0.831f, 0.647f),
        dust3d::Color(1.000f, 0.620f, 0.898f)
    };

    const float spawnHeight = 4.8f;
    for (int index = 0; index < count; ++index) {
        float size = sizeDist(rng);
        float widthFactor = 1.0f;
        if (index < (int)widthFactors.size())
            widthFactor = widthFactors[index];
        widthFactor = std::clamp(widthFactor, 0.40f, 1.0f);
        float baseDiameter = size * 2.3f * widthFactor;
        float width = baseDiameter;
        float depth = size * 0.75f;
        float height = size * 1.05f;
        PhysicsCube::ShapeType shapeType = PhysicsCube::ShapeType::Cuboid;
        int shapeCode = shapeDist(rng);
        if (shapeCode == 1) {
            shapeType = PhysicsCube::ShapeType::Tube;
            depth = baseDiameter;
            height = size * 1.15f;
        } else if (shapeCode == 2) {
            shapeType = PhysicsCube::ShapeType::Cone;
            depth = baseDiameter;
            height = size * 1.45f;
        }
        int spawnIndex = spawnIndexDist(rng);
        float x = spawnCenters[spawnIndex].x() + jitterDist(rng);
        float z = spawnCenters[spawnIndex].y() + jitterDist(rng);
        float yaw = yawDist(rng);
        dust3d::Color castColor = colorOptions[colorIndexDist(rng)];
        QRectF uvRect;
        if (!uvRects.empty() && index < (int)uvRects.size())
            uvRect = uvRects[index];

        PhysicsCube cube;
        cube.x = x;
        cube.z = z;
        cube.y = spawnHeight + (index % 4) * 0.24f;
        cube.vx = jitterDist(rng) * 0.5f;
        cube.vy = 0.0f;
        cube.vz = jitterDist(rng) * 0.5f;
        cube.width = width;
        cube.depth = depth;
        cube.height = height;
        cube.yaw = yaw;
        cube.pitch = jitterDist(rng) * 0.35f;
        cube.roll = jitterDist(rng) * 0.35f;
        cube.angularVelocityX = jitterDist(rng) * 1.0f;
        cube.angularVelocityY = jitterDist(rng) * 1.0f;
        cube.angularVelocityZ = jitterDist(rng) * 1.0f;
        cube.mass = width * depth * height;
        cube.shapeType = shapeType;
        cube.color = castColor;
        cube.uvRect = uvRect;
        cube.settled = false;
        m_physicsCubes.push_back(cube);
    }

    if (m_tubeOpenGLObject)
        m_tubeOpenGLObject->update(buildPhysicsCubeMesh(m_physicsCubes, &m_nameCubes));

    m_physicsTimer = new QTimer(this);
    m_physicsTimer->setInterval(16);
    m_physicsTimer->setSingleShot(false);
    connect(m_physicsTimer, &QTimer::timeout, this, &SceneWidget::updatePhysicsStep);
    m_physicsTimer->start();
}

void SceneWidget::updatePhysicsStep()
{
    const float dt = 1.0f / 60.0f;
    const float gravity = -12.0f;
    const float restitution = 0.12f;
    const float friction = 0.92f;
    bool anyMoving = false;

    for (auto& cube : m_physicsCubes) {
        cube.vy += gravity * dt;
        cube.vx *= 0.995f;
        cube.vz *= 0.995f;
        cube.angularVelocityX *= 0.98f;
        cube.angularVelocityY *= 0.98f;
        cube.angularVelocityZ *= 0.98f;
        cube.x += cube.vx * dt;
        cube.z += cube.vz * dt;
        cube.y += cube.vy * dt;
        cube.pitch += cube.angularVelocityX * dt;
        cube.yaw += cube.angularVelocityY * dt;
        cube.roll += cube.angularVelocityZ * dt;
        cube.settled = false;
    }

    const int count = (int)m_physicsCubes.size();
    for (int i = 0; i < count; ++i) {
        for (int j = i + 1; j < count; ++j) {
            auto& a = m_physicsCubes[i];
            auto& b = m_physicsCubes[j];
            if (!footprintsOverlap(a.x, a.z, a.width, a.depth, a.yaw,
                    b.x, b.z, b.width, b.depth, b.yaw))
                continue;

            float aMinY = a.y;
            float aMaxY = a.y + a.height;
            float bMinY = b.y;
            float bMaxY = b.y + b.height;
            if (aMaxY <= bMinY || bMaxY <= aMinY)
                continue;

            float penetration = std::min(aMaxY, bMaxY) - std::max(aMinY, bMinY);
            if (penetration <= 0.0f)
                continue;

            bool aAbove = a.y >= b.y;
            PhysicsCube* top = aAbove ? &a : &b;
            PhysicsCube* bottom = aAbove ? &b : &a;

            if (top->y < bottom->y + bottom->height)
                top->y = bottom->y + bottom->height;

            float relVy = top->vy - bottom->vy;
            if (relVy < 0.0f) {
                float impulse = -(1.0f + restitution) * relVy * 0.35f / (1.0f / top->mass + 1.0f / bottom->mass);
                top->vy += impulse / top->mass;
                bottom->vy -= impulse / bottom->mass;
            }

            QVector2D dir(bottom->x - top->x, bottom->z - top->z);
            if (dir.lengthSquared() > 1e-6f) {
                dir.normalize();
                float push = std::min(1.0f, std::abs(relVy) * 0.1f);
                top->vx -= dir.x() * push;
                top->vz -= dir.y() * push;
                bottom->vx += dir.x() * push;
                bottom->vz += dir.y() * push;
                top->angularVelocityX += dir.y() * push * 1.2f;
                top->angularVelocityZ -= dir.x() * push * 1.2f;
                bottom->angularVelocityX -= dir.y() * push * 1.2f;
                bottom->angularVelocityZ += dir.x() * push * 1.2f;
            }
        }
    }

    for (int index = 0; index < (int)m_physicsCubes.size(); ++index) {
        auto& cube = m_physicsCubes[index];
        if (cube.y < 0.0f) {
            cube.y = 0.0f;
            if (cube.vy < 0.0f)
                cube.vy = -cube.vy * restitution;
            cube.vx *= friction;
            cube.vz *= friction;
            cube.angularVelocityX *= 0.85f;
            cube.angularVelocityZ *= 0.85f;
        }

        int supportCount = countCubeSupports(cube, m_physicsCubes, index);
        bool groundContact = cube.y <= 0.001f;
        if (groundContact && supportCount == 0) {
            cube.pitch = 0.0f;
            cube.roll = 0.0f;
            cube.angularVelocityX = 0.0f;
            cube.angularVelocityZ = 0.0f;
        }

        if (std::abs(cube.vx) > 0.01f || std::abs(cube.vz) > 0.01f || std::abs(cube.vy) > 0.02f)
            anyMoving = true;
        else if (groundContact)
            cube.settled = true;
    }

    if (m_tubeOpenGLObject)
        m_tubeOpenGLObject->update(buildPhysicsCubeMesh(m_physicsCubes, &m_nameCubes));
    update();

    if (!anyMoving) {
        if (m_physicsTimer) {
            m_physicsTimer->stop();
            delete m_physicsTimer;
            m_physicsTimer = nullptr;
        }
    }
}

void SceneWidget::setDropSimulationData(const std::vector<QRectF>& uvRects, const std::vector<float>& widthFactors)
{
    m_dropSimulationUvRects = uvRects;
    m_dropSimulationWidthFactors = widthFactors;
    m_dropSimulationReady = true;
    m_dropSimulationStarted = false;
}

void SceneWidget::triggerDropSimulation()
{
    if (!m_dropSimulationReady)
        return;

    if (m_dropSimulationStarted) {
        resumeSimulation();
        return;
    }

    m_dropSimulationStarted = true;
    startDropSimulation(m_dropSimulationUvRects, m_dropSimulationWidthFactors);
}

void SceneWidget::stopSimulation()
{
    if (m_physicsTimer) {
        m_physicsTimer->stop();
        delete m_physicsTimer;
        m_physicsTimer = nullptr;
    }
}

void SceneWidget::resumeSimulation()
{
    if (!m_dropSimulationStarted)
        return;
    if (m_physicsTimer)
        return;
    if (m_physicsCubes.empty())
        return;

    m_physicsTimer = new QTimer(this);
    m_physicsTimer->setInterval(16);
    m_physicsTimer->setSingleShot(false);
    connect(m_physicsTimer, &QTimer::timeout, this, &SceneWidget::updatePhysicsStep);
    m_physicsTimer->start();
}

void SceneWidget::toggleWireframe()
{
    m_isWireframeVisible = !m_isWireframeVisible;
    update();
}

void SceneWidget::setWireframeVisible(bool visible)
{
    if (m_isWireframeVisible != visible) {
        m_isWireframeVisible = visible;
        update();
    }
}

bool SceneWidget::isWireframeVisible()
{
    return m_isWireframeVisible;
}

void SceneWidget::toggleRotation()
{
    if (nullptr != m_rotationTimer) {
        delete m_rotationTimer;
        m_rotationTimer = nullptr;
    } else {
        m_rotationTimer = new QTimer(this);
        m_rotationTimer->setInterval(42);
        m_rotationTimer->setSingleShot(false);
        connect(m_rotationTimer, &QTimer::timeout, this, [&]() {
            setYRotation(m_yRot - 8);
        });
        m_rotationTimer->start();
    }
}

void SceneWidget::canvasResized()
{
    resize(parentWidget()->size());
}

void SceneWidget::initializeShadowFBO()
{
    if (m_shadowFBOId)
        return;

    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
    QOpenGLExtraFunctions* ef = QOpenGLContext::currentContext()->extraFunctions();

    // Create depth texture
    f->glGenTextures(1, &m_shadowDepthTexture);
    f->glBindTexture(GL_TEXTURE_2D, m_shadowDepthTexture);
    f->glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
        m_shadowMapSize, m_shadowMapSize, 0,
        GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    f->glBindTexture(GL_TEXTURE_2D, 0);

    // Create depth-only FBO
    f->glGenFramebuffers(1, &m_shadowFBOId);
    f->glBindFramebuffer(GL_FRAMEBUFFER, m_shadowFBOId);
    f->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
        GL_TEXTURE_2D, m_shadowDepthTexture, 0);

    // No color buffer: signal this explicitly
    GLenum drawBufs[] = { GL_NONE };
    ef->glDrawBuffers(1, drawBufs);
    ef->glReadBuffer(GL_NONE);

    f->glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());
}

void SceneWidget::initializeGL()
{
    connect(context(), &QOpenGLContext::aboutToBeDestroyed, this, &SceneWidget::cleanup);
    initializeShadowFBO();

    std::vector<QString> names = loadNamesFromRepository();
    std::vector<QRectF> uvRects;
    std::vector<float> widthFactors;
    QImage nameAtlasImage = buildNameAtlasImage(names, uvRects, widthFactors);
    if (!nameAtlasImage.isNull()) {
        if (m_nameAtlasTexture)
            m_nameAtlasTexture->destroy();
        m_nameAtlasTexture = std::make_unique<QOpenGLTexture>(nameAtlasImage);
        m_nameAtlasTexture->setMinificationFilter(QOpenGLTexture::Linear);
        m_nameAtlasTexture->setMagnificationFilter(QOpenGLTexture::Linear);
        m_nameAtlasTexture->setWrapMode(QOpenGLTexture::Repeat);
    }

    if (!m_tubeOpenGLObject)
        m_tubeOpenGLObject = std::make_unique<ModelOpenGLObject>();
    setDropSimulationData(uvRects, widthFactors);
}

void SceneWidget::resizeGL(int w, int h)
{
    m_widthInPixels = w * window()->devicePixelRatio();
    m_heightInPixels = h * window()->devicePixelRatio();
    updateProjectionMatrix();
    emit renderParametersChanged();
}

void SceneWidget::paintGL()
{
    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
    bool isCoreProfile = format().profile() == QSurfaceFormat::CoreProfile;

    // Lazily initialize programs
    if (!m_shadowOpenGLProgram) {
        m_shadowOpenGLProgram = std::make_unique<ShadowOpenGLProgram>();
        m_shadowOpenGLProgram->load(isCoreProfile);
    }
    if (!m_worldOpenGLProgram) {
        m_worldOpenGLProgram = std::make_unique<SceneOpenGLProgram>();
        m_worldOpenGLProgram->load(isCoreProfile);
    } else if (!m_worldOpenGLProgram->isLinked()) {
        m_worldOpenGLProgram->load(isCoreProfile);
    }
    if (!m_groundOpenGLProgram) {
        m_groundOpenGLProgram = std::make_unique<SceneGroundOpenGLProgram>();
        m_groundOpenGLProgram->load(isCoreProfile);
    }
    if (!m_groundOpenGLObject) {
        m_groundOpenGLObject = std::make_unique<WorldGroundOpenGLObject>();
        m_groundOpenGLObject->create(0.0f, 50.0f);
    }

    // Model rotation and camera
    m_world.setToIdentity();
    m_world.rotate(m_xRot / 16.0f, 1, 0, 0);
    m_world.rotate(m_yRot / 16.0f, 0, 1, 0);
    m_world.rotate(m_zRot / 16.0f, 0, 0, 1);

    m_camera.setToIdentity();
    m_camera.translate(m_eyePosition.x(), m_eyePosition.y(), m_eyePosition.z());

    // Light space matrix — fixed directional light matching LIGHT_POS in shaders
    static const QVector3D s_lightDir = QVector3D(10.0f, 15.0f, 10.0f).normalized();
    static const float s_lightDist = 25.0f;
    QVector3D lightPos = s_lightDir * s_lightDist;
    m_lightView.setToIdentity();
    m_lightView.lookAt(lightPos, QVector3D(0, 0, 0), QVector3D(0, 1, 0));
    m_lightProjection.setToIdentity();
    m_lightProjection.ortho(-10.0f, 10.0f, -10.0f, 10.0f, 1.0f, 50.0f);
    m_lightSpaceMatrix = m_lightProjection * m_lightView;

    if (m_isWireframeVisible) {
        if (!m_monochromeOpenGLProgram) {
            m_monochromeOpenGLProgram = std::make_unique<MonochromeOpenGLProgram>();
            m_monochromeOpenGLProgram->load(isCoreProfile);
        }
    }

    // --- Pass 1: Shadow depth ---
    drawShadowPass();

    // --- Pass 2: Scene ---
    f->glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());
    f->glViewport(0, 0, m_widthInPixels, m_heightInPixels);
    const QColor clearColor = Theme::darkBackground;
    f->glClearColor(clearColor.redF(), clearColor.greenF(), clearColor.blueF(), clearColor.alphaF());
    f->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    f->glEnable(GL_BLEND);
    f->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    f->glEnable(GL_DEPTH_TEST);
    f->glEnable(GL_POLYGON_OFFSET_FILL);
    f->glPolygonOffset(1.0f, 1.0f);

    f->glEnable(GL_CULL_FACE);
    f->glCullFace(GL_BACK);
    drawWorldModel();

    f->glDisable(GL_CULL_FACE);
    drawGround();

    if (m_isWireframeVisible)
        drawWireframe();

    f->glDisable(GL_POLYGON_OFFSET_FILL);
}

void SceneWidget::drawShadowPass()
{
    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();

    f->glBindFramebuffer(GL_FRAMEBUFFER, m_shadowFBOId);
    f->glViewport(0, 0, m_shadowMapSize, m_shadowMapSize);
    f->glClear(GL_DEPTH_BUFFER_BIT);
    f->glEnable(GL_DEPTH_TEST);

    // Cull front faces to reduce self-shadowing (peter-panning fix)
    f->glEnable(GL_CULL_FACE);
    f->glCullFace(GL_FRONT);

    m_shadowOpenGLProgram->bind();
    m_shadowOpenGLProgram->setUniformValue(
        m_shadowOpenGLProgram->getUniformLocationByName("lightSpaceMatrix"), m_lightSpaceMatrix);
    m_shadowOpenGLProgram->setUniformValue(
        m_shadowOpenGLProgram->getUniformLocationByName("modelMatrix"), m_world);

    if (m_modelOpenGLObject)
        m_modelOpenGLObject->draw();
    for (const auto& previewObject : m_previewOpenGLObjects) {
        if (previewObject)
            previewObject->draw();
    }
    if (m_tubeOpenGLObject)
        m_tubeOpenGLObject->draw();

    m_shadowOpenGLProgram->release();
    f->glCullFace(GL_BACK);
}

void SceneWidget::drawWorldModel()
{
    m_worldOpenGLProgram->bind();

    m_worldOpenGLProgram->setUniformValue(
        m_worldOpenGLProgram->getUniformLocationByName("eyePosition"), m_eyePosition);
    m_worldOpenGLProgram->setUniformValue(
        m_worldOpenGLProgram->getUniformLocationByName("projectionMatrix"), m_projection);
    m_worldOpenGLProgram->setUniformValue(
        m_worldOpenGLProgram->getUniformLocationByName("modelMatrix"), m_world);
    m_worldOpenGLProgram->setUniformValue(
        m_worldOpenGLProgram->getUniformLocationByName("normalMatrix"), m_world.normalMatrix());
    m_worldOpenGLProgram->setUniformValue(
        m_worldOpenGLProgram->getUniformLocationByName("viewMatrix"), m_camera);
    m_worldOpenGLProgram->setUniformValue(
        m_worldOpenGLProgram->getUniformLocationByName("lightSpaceMatrix"), m_lightSpaceMatrix);

    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
    if (m_modelOpenGLObject) {
        m_worldOpenGLProgram->updateTextureImage(m_modelTextureImage ? std::make_unique<QImage>(*m_modelTextureImage) : nullptr);
        m_worldOpenGLProgram->updateNormalMapImage(m_modelNormalMapImage ? std::make_unique<QImage>(*m_modelNormalMapImage) : nullptr);
        m_worldOpenGLProgram->updateMetalnessRoughnessAmbientOcclusionMapImage(
            m_modelMetalnessRoughnessAmbientOcclusionMapImage ? std::make_unique<QImage>(*m_modelMetalnessRoughnessAmbientOcclusionMapImage) : nullptr,
            m_modelHasMetalnessInImage,
            m_modelHasRoughnessInImage,
            m_modelHasAmbientOcclusionInImage);
        m_worldOpenGLProgram->bindMaps(m_shadowDepthTexture);
        m_modelOpenGLObject->draw();
        m_worldOpenGLProgram->releaseMaps();
    }

    for (int i = 0; i < static_cast<int>(m_previewOpenGLObjects.size()); ++i) {
        const auto& previewObject = m_previewOpenGLObjects[i];
        if (!previewObject)
            continue;

        m_worldOpenGLProgram->updateTextureImage(m_previewTextureImages[i] ? std::make_unique<QImage>(*m_previewTextureImages[i]) : nullptr);
        m_worldOpenGLProgram->updateNormalMapImage(m_previewNormalMapImages[i] ? std::make_unique<QImage>(*m_previewNormalMapImages[i]) : nullptr);
        m_worldOpenGLProgram->updateMetalnessRoughnessAmbientOcclusionMapImage(
            m_previewMetalnessRoughnessAmbientOcclusionMapImages[i] ? std::make_unique<QImage>(*m_previewMetalnessRoughnessAmbientOcclusionMapImages[i]) : nullptr,
            m_previewHasMetalnessInImage[i],
            m_previewHasRoughnessInImage[i],
            m_previewHasAmbientOcclusionInImage[i]);
        m_worldOpenGLProgram->bindMaps(m_shadowDepthTexture);
        previewObject->draw();
        m_worldOpenGLProgram->releaseMaps();
    }

    if (m_tubeOpenGLObject) {
        m_worldOpenGLProgram->updateTextureImage(nullptr);
        m_worldOpenGLProgram->updateNormalMapImage(nullptr);
        m_worldOpenGLProgram->updateMetalnessRoughnessAmbientOcclusionMapImage(nullptr, false, false, false);
        m_worldOpenGLProgram->bindMaps(m_shadowDepthTexture);
        if (m_nameAtlasTexture) {
            f->glActiveTexture(GL_TEXTURE0 + 2);
            m_nameAtlasTexture->bind();
            m_worldOpenGLProgram->setUniformValue(
                m_worldOpenGLProgram->getUniformLocationByName("textureId"), 2);
            m_worldOpenGLProgram->setUniformValue(
                m_worldOpenGLProgram->getUniformLocationByName("textureEnabled"), 1);
            f->glActiveTexture(GL_TEXTURE0);
        }
        m_tubeOpenGLObject->draw();
        m_worldOpenGLProgram->releaseMaps();
    }

    m_worldOpenGLProgram->release();
}

void SceneWidget::drawGround()
{
    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();

    m_groundOpenGLProgram->bind();

    m_groundOpenGLProgram->setUniformValue(
        m_groundOpenGLProgram->getUniformLocationByName("projectionMatrix"), m_projection);
    m_groundOpenGLProgram->setUniformValue(
        m_groundOpenGLProgram->getUniformLocationByName("viewMatrix"), m_camera);
    m_groundOpenGLProgram->setUniformValue(
        m_groundOpenGLProgram->getUniformLocationByName("modelMatrix"), m_world);
    m_groundOpenGLProgram->setUniformValue(
        m_groundOpenGLProgram->getUniformLocationByName("lightSpaceMatrix"), m_lightSpaceMatrix);

    // Bind shadow depth texture at unit 1
    f->glActiveTexture(GL_TEXTURE1);
    f->glBindTexture(GL_TEXTURE_2D, m_shadowDepthTexture);
    m_groundOpenGLProgram->setUniformValue(
        m_groundOpenGLProgram->getUniformLocationByName("shadowMap"), 1);

    // Rotate ground offset to match the model's world rotation so the
    // checkerboard scrolls in the correct direction regardless of camera angle
    QMatrix4x4 rot;
    rot.rotate(m_xRot / 16.0f, 1, 0, 0);
    rot.rotate(m_yRot / 16.0f, 0, 1, 0);
    rot.rotate(m_zRot / 16.0f, 0, 0, 1);
    QVector3D rotatedOffset = rot.map(QVector3D(m_groundOffsetX, 0.0f, m_groundOffsetZ));
    m_groundOpenGLProgram->setUniformValue(
        m_groundOpenGLProgram->getUniformLocationByName("groundOffset"), QVector2D(rotatedOffset.x(), rotatedOffset.z()));

    if (m_groundOpenGLObject)
        m_groundOpenGLObject->draw();

    // Unbind shadow texture
    f->glActiveTexture(GL_TEXTURE1);
    f->glBindTexture(GL_TEXTURE_2D, 0);
    f->glActiveTexture(GL_TEXTURE0);

    m_groundOpenGLProgram->release();
}

void SceneWidget::drawWireframe()
{
    m_monochromeOpenGLProgram->bind();

    m_monochromeOpenGLProgram->setUniformValue(m_monochromeOpenGLProgram->getUniformLocationByName("projectionMatrix"), m_projection);
    m_monochromeOpenGLProgram->setUniformValue(m_monochromeOpenGLProgram->getUniformLocationByName("modelMatrix"), m_world);
    m_monochromeOpenGLProgram->setUniformValue(m_monochromeOpenGLProgram->getUniformLocationByName("viewMatrix"), m_camera);

    if (m_monochromeOpenGLProgram->isCoreProfile()) {
        m_monochromeOpenGLProgram->setUniformValue("viewportSize", QVector2D(m_widthInPixels, m_heightInPixels));
    }

    if (m_wireframeOpenGLObject)
        m_wireframeOpenGLObject->draw();

    m_monochromeOpenGLProgram->release();
}

void SceneWidget::zoom(float delta)
{
    if (m_moveAndZoomByWindow) {
        QMargins margins(delta, delta, delta, delta);
        if (0 == m_modelInitialHeight) {
            m_modelInitialHeight = height();
        } else {
            float ratio = (float)height() / m_modelInitialHeight;
            if (ratio <= m_minZoomRatio) {
                if (delta < 0)
                    return;
            } else if (ratio >= m_maxZoomRatio) {
                if (delta > 0)
                    return;
            }
        }
        setGeometry(geometry().marginsAdded(margins));
        emit renderParametersChanged();
        update();
    } else {
        m_eyePosition += QVector3D(0, 0, m_eyePosition.z() * (delta > 0 ? -0.1 : 0.1));
        if (m_eyePosition.z() < -15)
            m_eyePosition.setZ(-15);
        else if (m_eyePosition.z() > -0.1)
            m_eyePosition.setZ(-0.1f);
        emit eyePositionChanged(m_eyePosition);
        emit renderParametersChanged();
        update();
    }
}

void SceneWidget::mousePressEvent(QMouseEvent* event)
{
    bool shouldStartMove = false;
    if (event->button() == Qt::LeftButton) {
        if (QGuiApplication::queryKeyboardModifiers().testFlag(Qt::AltModifier) && !QGuiApplication::queryKeyboardModifiers().testFlag(Qt::ControlModifier))
            shouldStartMove = m_moveEnabled;
    } else if (event->button() == Qt::MiddleButton) {
        shouldStartMove = m_moveEnabled;
    }
    if (shouldStartMove) {
        m_lastPos = event->pos();
        if (!m_moveStarted) {
            m_moveStartPos = mapToParent(event->pos());
            m_moveStartGeometry = geometry();
            m_moveStarted = true;
            m_directionOnMoveStart = abs(m_xRot) > 180 * 8 ? -1 : 1;
        }
    }
}

void SceneWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (!m_moveStarted)
        return;

    QPoint pos = event->pos();
    int dx = pos.x() - m_lastPos.x();
    int dy = pos.y() - m_lastPos.y();

    if ((event->buttons() & Qt::MiddleButton) || (m_moveStarted && (event->buttons() & Qt::LeftButton))) {
        if (QGuiApplication::queryKeyboardModifiers().testFlag(Qt::ShiftModifier)) {
            if (m_moveAndZoomByWindow) {
                QPoint posInParent = mapToParent(pos);
                QRect rect = m_moveStartGeometry;
                rect.translate(posInParent.x() - m_moveStartPos.x(), posInParent.y() - m_moveStartPos.y());
                setGeometry(rect);
            } else {
                m_moveToPosition.setX(m_moveToPosition.x() + (float)2 * dx / width());
                m_moveToPosition.setY(m_moveToPosition.y() + (float)2 * -dy / height());
                if (m_moveToPosition.x() < -1.0)
                    m_moveToPosition.setX(-1.0);
                if (m_moveToPosition.x() > 1.0)
                    m_moveToPosition.setX(1.0);
                if (m_moveToPosition.y() < -1.0)
                    m_moveToPosition.setY(-1.0);
                if (m_moveToPosition.y() > 1.0)
                    m_moveToPosition.setY(1.0);
                updateProjectionMatrix();
                emit moveToPositionChanged(m_moveToPosition);
                emit renderParametersChanged();
                update();
            }
        } else {
            setXRotation(m_xRot + 8 * dy);
            setYRotation(m_yRot + 8 * dx * m_directionOnMoveStart);
        }
    }
    m_lastPos = pos;
}

void SceneWidget::wheelEvent(QWheelEvent* event)
{
    if (m_moveStarted)
        return;
    if (!m_zoomEnabled)
        return;

    qreal delta = geometry().height() * 0.1f;
    if (event->pixelDelta().y() < 0)
        delta = -delta;
    zoom(delta);
}

void SceneWidget::mouseReleaseEvent(QMouseEvent* event)
{
    Q_UNUSED(event);
    if (m_moveStarted)
        m_moveStarted = false;
}
