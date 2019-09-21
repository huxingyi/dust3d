#include <assert.h>
#include <QTextStream>
#include <QFile>
#include <cmath>
#include "meshloader.h"
#include "ds3file.h"

#define MAX_VERTICES_PER_FACE   100

float MeshLoader::m_defaultMetalness = 0.0;
float MeshLoader::m_defaultRoughness = 1.0;

MeshLoader::MeshLoader(const MeshLoader &mesh) :
    m_triangleVertices(nullptr),
    m_triangleVertexCount(0),
    m_edgeVertices(nullptr),
    m_edgeVertexCount(0),
    m_textureImage(nullptr)
{
    if (nullptr != mesh.m_triangleVertices &&
            mesh.m_triangleVertexCount > 0) {
        this->m_triangleVertices = new ShaderVertex[mesh.m_triangleVertexCount];
        this->m_triangleVertexCount = mesh.m_triangleVertexCount;
        for (int i = 0; i < mesh.m_triangleVertexCount; i++)
            this->m_triangleVertices[i] = mesh.m_triangleVertices[i];
    }
    if (nullptr != mesh.m_edgeVertices &&
            mesh.m_edgeVertexCount > 0) {
        this->m_edgeVertices = new ShaderVertex[mesh.m_edgeVertexCount];
        this->m_edgeVertexCount = mesh.m_edgeVertexCount;
        for (int i = 0; i < mesh.m_edgeVertexCount; i++)
            this->m_edgeVertices[i] = mesh.m_edgeVertices[i];
    }
    if (nullptr != mesh.m_toolVertices &&
            mesh.m_toolVertexCount > 0) {
        this->m_toolVertices = new ShaderVertex[mesh.m_toolVertexCount];
        this->m_toolVertexCount = mesh.m_toolVertexCount;
        for (int i = 0; i < mesh.m_toolVertexCount; i++)
            this->m_toolVertices[i] = mesh.m_toolVertices[i];
    }
    if (nullptr != mesh.m_textureImage) {
        this->m_textureImage = new QImage(*mesh.m_textureImage);
    }
    if (nullptr != mesh.m_normalMapImage) {
        this->m_normalMapImage = new QImage(*mesh.m_normalMapImage);
    }
    if (nullptr != mesh.m_metalnessRoughnessAmbientOcclusionImage) {
        this->m_metalnessRoughnessAmbientOcclusionImage = new QImage(*mesh.m_metalnessRoughnessAmbientOcclusionImage);
        this->m_hasMetalnessInImage = mesh.m_hasMetalnessInImage;
        this->m_hasRoughnessInImage = mesh.m_hasRoughnessInImage;
        this->m_hasAmbientOcclusionInImage = mesh.m_hasAmbientOcclusionInImage;
    }
    this->m_vertices = mesh.m_vertices;
    this->m_faces = mesh.m_faces;
    this->m_triangulatedVertices = mesh.m_triangulatedVertices;
    this->m_triangulatedFaces = mesh.m_triangulatedFaces;
    this->m_meshId = mesh.meshId();
}

MeshLoader::MeshLoader(ShaderVertex *triangleVertices, int vertexNum, ShaderVertex *edgeVertices, int edgeVertexCount) :
    m_triangleVertices(triangleVertices),
    m_triangleVertexCount(vertexNum),
    m_edgeVertices(edgeVertices),
    m_edgeVertexCount(edgeVertexCount),
    m_textureImage(nullptr)
{
}

MeshLoader::MeshLoader(const std::vector<QVector3D> &vertices, const std::vector<std::vector<size_t>> &triangles,
    const std::vector<std::vector<QVector3D>> &triangleVertexNormals,
    const QColor &color)
{
    m_triangleVertexCount = triangles.size() * 3;
    m_triangleVertices = new ShaderVertex[m_triangleVertexCount];
    int destIndex = 0;
    for (size_t i = 0; i < triangles.size(); ++i) {
        for (auto j = 0; j < 3; j++) {
            int vertexIndex = triangles[i][j];
            const QVector3D *srcVert = &vertices[vertexIndex];
            const QVector3D *srcNormal = &(triangleVertexNormals)[i][j];
            ShaderVertex *dest = &m_triangleVertices[destIndex];
            dest->colorR = color.redF();
            dest->colorG = color.greenF();
            dest->colorB = color.blueF();
            dest->posX = srcVert->x();
            dest->posY = srcVert->y();
            dest->posZ = srcVert->z();
            dest->texU = 0;
            dest->texV = 0;
            dest->normX = srcNormal->x();
            dest->normY = srcNormal->y();
            dest->normZ = srcNormal->z();
            dest->metalness = m_defaultMetalness;
            dest->roughness = m_defaultRoughness;
            dest->tangentX = 0;
            dest->tangentY = 0;
            dest->tangentZ = 0;
            destIndex++;
        }
    }
}

MeshLoader::MeshLoader(Outcome &outcome) :
    m_triangleVertices(nullptr),
    m_triangleVertexCount(0),
    m_edgeVertices(nullptr),
    m_edgeVertexCount(0),
    m_textureImage(nullptr)
{
    m_meshId = outcome.meshId;
    m_vertices = outcome.vertices;
    m_faces = outcome.triangleAndQuads;
    
    m_triangleVertexCount = outcome.triangles.size() * 3;
    m_triangleVertices = new ShaderVertex[m_triangleVertexCount];
    int destIndex = 0;
    const auto triangleVertexNormals = outcome.triangleVertexNormals();
    const auto triangleVertexUvs = outcome.triangleVertexUvs();
    const auto triangleTangents = outcome.triangleTangents();
    const QVector3D defaultNormal = QVector3D(0, 0, 0);
    const QVector2D defaultUv = QVector2D(0, 0);
    const QVector3D defaultTangent = QVector3D(0, 0, 0);
    for (size_t i = 0; i < outcome.triangles.size(); ++i) {
        const auto &triangleColor = &outcome.triangleColors[i];
        for (auto j = 0; j < 3; j++) {
            int vertexIndex = outcome.triangles[i][j];
            const QVector3D *srcVert = &outcome.vertices[vertexIndex];
            const QVector3D *srcNormal = &defaultNormal;
            if (triangleVertexNormals)
                srcNormal = &(*triangleVertexNormals)[i][j];
            const QVector2D *srcUv = &defaultUv;
            if (triangleVertexUvs)
                srcUv = &(*triangleVertexUvs)[i][j];
            const QVector3D *srcTangent = &defaultTangent;
            if (triangleTangents)
                srcTangent = &(*triangleTangents)[i];
            ShaderVertex *dest = &m_triangleVertices[destIndex];
            dest->colorR = triangleColor->redF();
            dest->colorG = triangleColor->greenF();
            dest->colorB = triangleColor->blueF();
            dest->posX = srcVert->x();
            dest->posY = srcVert->y();
            dest->posZ = srcVert->z();
            dest->texU = srcUv->x();
            dest->texV = srcUv->y();
            dest->normX = srcNormal->x();
            dest->normY = srcNormal->y();
            dest->normZ = srcNormal->z();
            dest->metalness = m_defaultMetalness;
            dest->roughness = m_defaultRoughness;
            dest->tangentX = srcTangent->x();
            dest->tangentY = srcTangent->y();
            dest->tangentZ = srcTangent->z();
            destIndex++;
        }
    }
    
    // Uncomment out to show wireframes
    size_t edgeCount = 0;
    for (const auto &face: outcome.triangleAndQuads) {
        edgeCount += face.size();
    }
    m_edgeVertexCount = edgeCount * 2;
    m_edgeVertices = new ShaderVertex[m_edgeVertexCount];
    size_t edgeVertexIndex = 0;
    for (size_t faceIndex = 0; faceIndex < outcome.triangleAndQuads.size(); ++faceIndex) {
        const auto &face = outcome.triangleAndQuads[faceIndex];
        for (size_t i = 0; i < face.size(); ++i) {
            for (size_t x = 0; x < 2; ++x) {
                size_t sourceIndex = face[(i + x) % face.size()];
                const QVector3D *srcVert = &outcome.vertices[sourceIndex];
                ShaderVertex *dest = &m_edgeVertices[edgeVertexIndex];
                memset(dest, 0, sizeof(ShaderVertex));
                dest->colorR = 0.0;
                dest->colorG = 0.0;
                dest->colorB = 0.0;
                dest->posX = srcVert->x();
                dest->posY = srcVert->y();
                dest->posZ = srcVert->z();
                dest->metalness = m_defaultMetalness;
                dest->roughness = m_defaultRoughness;
                edgeVertexIndex++;
            }
        }
    }
}

MeshLoader::MeshLoader() :
    m_triangleVertices(nullptr),
    m_triangleVertexCount(0),
    m_edgeVertices(nullptr),
    m_edgeVertexCount(0),
    m_textureImage(nullptr)
{
}

MeshLoader::~MeshLoader()
{
    delete[] m_triangleVertices;
    m_triangleVertexCount = 0;
    delete[] m_edgeVertices;
    m_edgeVertexCount = 0;
    delete[] m_toolVertices;
    m_toolVertexCount = 0;
    delete m_textureImage;
    delete m_normalMapImage;
    delete m_metalnessRoughnessAmbientOcclusionImage;
}

const std::vector<QVector3D> &MeshLoader::vertices()
{
    return m_vertices;
}

const std::vector<std::vector<size_t>> &MeshLoader::faces()
{
    return m_faces;
}

const std::vector<QVector3D> &MeshLoader::triangulatedVertices()
{
    return m_triangulatedVertices;
}

const std::vector<TriangulatedFace> &MeshLoader::triangulatedFaces()
{
    return m_triangulatedFaces;
}

ShaderVertex *MeshLoader::triangleVertices()
{
    return m_triangleVertices;
}

int MeshLoader::triangleVertexCount()
{
    return m_triangleVertexCount;
}

ShaderVertex *MeshLoader::edgeVertices()
{
    return m_edgeVertices;
}

int MeshLoader::edgeVertexCount()
{
    return m_edgeVertexCount;
}

ShaderVertex *MeshLoader::toolVertices()
{
    return m_toolVertices;
}

int MeshLoader::toolVertexCount()
{
    return m_toolVertexCount;
}

void MeshLoader::setTextureImage(QImage *textureImage)
{
    m_textureImage = textureImage;
}

const QImage *MeshLoader::textureImage()
{
    return m_textureImage;
}

void MeshLoader::setNormalMapImage(QImage *normalMapImage)
{
    m_normalMapImage = normalMapImage;
}

const QImage *MeshLoader::normalMapImage()
{
    return m_normalMapImage;
}

const QImage *MeshLoader::metalnessRoughnessAmbientOcclusionImage()
{
    return m_metalnessRoughnessAmbientOcclusionImage;
}

void MeshLoader::setMetalnessRoughnessAmbientOcclusionImage(QImage *image)
{
    m_metalnessRoughnessAmbientOcclusionImage = image;
}

bool MeshLoader::hasMetalnessInImage()
{
    return m_hasMetalnessInImage;
}

void MeshLoader::setHasMetalnessInImage(bool hasInImage)
{
    m_hasMetalnessInImage = hasInImage;
}

bool MeshLoader::hasRoughnessInImage()
{
    return m_hasRoughnessInImage;
}

void MeshLoader::setHasRoughnessInImage(bool hasInImage)
{
    m_hasRoughnessInImage = hasInImage;
}

bool MeshLoader::hasAmbientOcclusionInImage()
{
    return m_hasAmbientOcclusionInImage;
}

void MeshLoader::setHasAmbientOcclusionInImage(bool hasInImage)
{
    m_hasAmbientOcclusionInImage = hasInImage;
}

void MeshLoader::exportAsObj(QTextStream *textStream)
{
    auto &stream = *textStream;
    stream << "# " << Ds3FileReader::m_applicationName << endl;
    for (std::vector<QVector3D>::const_iterator it = vertices().begin() ; it != vertices().end(); ++it) {
        stream << "v " << (*it).x() << " " << (*it).y() << " " << (*it).z() << endl;
    }
    for (std::vector<std::vector<size_t>>::const_iterator it = faces().begin() ; it != faces().end(); ++it) {
        stream << "f";
        for (std::vector<size_t>::const_iterator subIt = (*it).begin() ; subIt != (*it).end(); ++subIt) {
            stream << " " << (1 + *subIt);
        }
        stream << endl;
    }
}

void MeshLoader::exportAsObj(const QString &filename)
{
    QFile file(filename);
    if (file.open(QIODevice::WriteOnly)) {
        QTextStream stream(&file);
        exportAsObj(&stream);
    }
}

void MeshLoader::updateTool(ShaderVertex *toolVertices, int vertexNum)
{
    delete[] m_toolVertices;
    m_toolVertices = nullptr;
    m_toolVertexCount = 0;
    
    m_toolVertices = toolVertices;
    m_toolVertexCount = vertexNum;
}

void MeshLoader::updateEdges(ShaderVertex *edgeVertices, int edgeVertexCount)
{
    delete[] m_edgeVertices;
    m_edgeVertices = nullptr;
    m_edgeVertexCount = 0;
    
    m_edgeVertices = edgeVertices;
    m_edgeVertexCount = edgeVertexCount;
}

void MeshLoader::updateTriangleVertices(ShaderVertex *triangleVertices, int triangleVertexCount)
{
    delete[] m_triangleVertices;
    m_triangleVertices = 0;
    m_triangleVertexCount = 0;
    
    m_triangleVertices = triangleVertices;
    m_triangleVertexCount = triangleVertexCount;
}

quint64 MeshLoader::meshId() const
{
    return m_meshId;
}

void MeshLoader::setMeshId(quint64 id)
{
    m_meshId = id;
}
