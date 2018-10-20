#include <assert.h>
#include <QTextStream>
#include <QFile>
#include <cmath>
#include "meshloader.h"
#include "meshlite.h"
#include "theme.h"
#include "positionmap.h"
#include "ds3file.h"
#include "anglesmooth.h"

#define MAX_VERTICES_PER_FACE   100

float MeshLoader::m_defaultMetalness = 0.0;
float MeshLoader::m_defaultRoughness = 1.0;

MeshLoader::MeshLoader(void *meshlite, int meshId, int triangulatedMeshId, QColor defaultColor, const std::vector<Material> *triangleMaterials, bool smoothNormal) :
    m_triangleVertices(nullptr),
    m_triangleVertexCount(0),
    m_edgeVertices(nullptr),
    m_edgeVertexCount(0),
    m_textureImage(nullptr)
{
    int edgeVertexPositionCount = meshlite_get_vertex_count(meshlite, meshId);
    GLfloat *edgeVertexPositions = new GLfloat[edgeVertexPositionCount * 3];
    int loadedEdgeVertexPositionItemCount = meshlite_get_vertex_position_array(meshlite, meshId, edgeVertexPositions, edgeVertexPositionCount * 3);
    
    int offset = 0;
    while (offset < loadedEdgeVertexPositionItemCount) {
        QVector3D position = QVector3D(edgeVertexPositions[offset], edgeVertexPositions[offset + 1], edgeVertexPositions[offset + 2]);
        m_vertices.push_back(position);
        offset += 3;
    }
    int faceCount = meshlite_get_face_count(meshlite, meshId);
    int *faceVertexNumAndIndices = new int[faceCount * (1 + MAX_VERTICES_PER_FACE)];
    int loadedFaceVertexNumAndIndicesItemCount = meshlite_get_face_index_array(meshlite, meshId, faceVertexNumAndIndices, faceCount * (1 + MAX_VERTICES_PER_FACE));
    offset = 0;
    while (offset < loadedFaceVertexNumAndIndicesItemCount) {
        int indicesNum = faceVertexNumAndIndices[offset++];
        assert(indicesNum >= 0 && indicesNum <= MAX_VERTICES_PER_FACE);
        std::vector<int> face;
        for (int i = 0; i < indicesNum && offset < loadedFaceVertexNumAndIndicesItemCount; i++) {
            int index = faceVertexNumAndIndices[offset++];
            assert(index >= 0 && index < loadedEdgeVertexPositionItemCount);
            face.push_back(index);
        }
        m_faces.push_back(face);
    }
    delete[] faceVertexNumAndIndices;
    faceVertexNumAndIndices = NULL;
    
    int edgeCount = meshlite_get_halfedge_count(meshlite, meshId);
    int *edgeIndices = new int[edgeCount * 2];
    int loadedEdgeVertexIndexItemCount = meshlite_get_halfedge_index_array(meshlite, meshId, edgeIndices, edgeCount * 2);
    GLfloat *edgeNormals = new GLfloat[edgeCount * 3];
    int loadedEdgeNormalItemCount = meshlite_get_halfedge_normal_array(meshlite, meshId, edgeNormals, edgeCount * 3);
    
    m_edgeVertexCount = edgeCount * 2;
    m_edgeVertices = new Vertex[m_edgeVertexCount * 3];
    for (int i = 0; i < edgeCount; i++) {
        int firstIndex = i * 2;
        for (int j = 0; j < 2; j++) {
            assert(firstIndex + j < loadedEdgeVertexIndexItemCount);
            int posIndex = edgeIndices[firstIndex + j] * 3;
            assert(posIndex < loadedEdgeVertexPositionItemCount);
            Vertex *v = &m_edgeVertices[firstIndex + j];
            v->posX = edgeVertexPositions[posIndex + 0];
            v->posY = edgeVertexPositions[posIndex + 1];
            v->posZ = edgeVertexPositions[posIndex + 2];
            assert(firstIndex + 2 < loadedEdgeNormalItemCount);
            v->normX = edgeNormals[firstIndex + 0];
            v->normY = edgeNormals[firstIndex + 1];
            v->normZ = edgeNormals[firstIndex + 2];
            v->colorR = 0.0;
            v->colorG = 0.0;
            v->colorB = 0.0;
            v->texU = 0.0;
            v->texV = 0.0;
            v->metalness = 0;
            v->roughness = 1.0;
        }
    }
    
    int triangleMesh = -1 == triangulatedMeshId ? meshlite_triangulate(meshlite, meshId) : triangulatedMeshId;
    
    int triangleVertexPositionCount = meshlite_get_vertex_count(meshlite, triangleMesh);
    GLfloat *triangleVertexPositions = new GLfloat[triangleVertexPositionCount * 3];
    int loadedTriangleVertexPositionItemCount = meshlite_get_vertex_position_array(meshlite, triangleMesh, triangleVertexPositions, triangleVertexPositionCount * 3);
    
    GLfloat *triangleVertexSmoothNormals = new GLfloat[triangleVertexPositionCount * 3];
    if (smoothNormal)
        memset(triangleVertexSmoothNormals, 0, sizeof(GLfloat) * triangleVertexPositionCount * 3);
    
    offset = 0;
    while (offset < loadedTriangleVertexPositionItemCount) {
        QVector3D position = QVector3D(triangleVertexPositions[offset], triangleVertexPositions[offset + 1], triangleVertexPositions[offset + 2]);
        m_triangulatedVertices.push_back(position);
        offset += 3;
    }
    
    int triangleCount = meshlite_get_face_count(meshlite, triangleMesh);
    int *triangleIndices = new int[triangleCount * 3];
    int loadedTriangleVertexIndexItemCount = meshlite_get_triangle_index_array(meshlite, triangleMesh, triangleIndices, triangleCount * 3);
    
    GLfloat *triangleNormals = new GLfloat[triangleCount * 3];
    int loadedTriangleNormalItemCount = meshlite_get_triangle_normal_array(meshlite, triangleMesh, triangleNormals, triangleCount * 3);
    
    float modelR = defaultColor.redF();
    float modelG = defaultColor.greenF();
    float modelB = defaultColor.blueF();
    m_triangleVertexCount = triangleCount * 3;
    m_triangleVertices = new Vertex[m_triangleVertexCount * 3];
    const std::vector<QVector3D> &inputVerticesForSmoothAngle = m_triangulatedVertices;
    std::vector<std::tuple<size_t, size_t, size_t>> inputTrianglesForSmoothAngle;
    std::vector<QVector3D> inputNormalsForSmoothAngle;
    float thresholdAngleDegrees = 60;
    for (int i = 0; i < triangleCount; i++) {
        int firstIndex = i * 3;
        float useColorR = modelR;
        float useColorG = modelG;
        float useColorB = modelB;
        float useMetalness = m_defaultMetalness;
        float useRoughness = m_defaultRoughness;
        if (triangleMaterials && i < (int)triangleMaterials->size()) {
            auto triangleMaterial = (*triangleMaterials)[i];
            useColorR = triangleMaterial.color.redF();
            useColorG = triangleMaterial.color.greenF();
            useColorB = triangleMaterial.color.blueF();
        }
        TriangulatedFace triangulatedFace;
        triangulatedFace.color.setRedF(useColorR);
        triangulatedFace.color.setGreenF(useColorG);
        triangulatedFace.color.setBlueF(useColorB);
        QVector3D positions[3];
        float area = 1.0;
        float angles[3] = {1.0, 1.0, 1.0};
        if (smoothNormal) {
            inputTrianglesForSmoothAngle.push_back({
                triangleIndices[firstIndex + 0],
                triangleIndices[firstIndex + 1],
                triangleIndices[firstIndex + 2]
            });
            inputNormalsForSmoothAngle.push_back(QVector3D(triangleNormals[firstIndex + 0],
                triangleNormals[firstIndex + 1],
                triangleNormals[firstIndex + 2]));
        }
        for (int j = 0; j < 3; j++) {
            assert(firstIndex + j < loadedTriangleVertexIndexItemCount);
            int posIndex = triangleIndices[firstIndex + j] * 3;
            assert(posIndex < loadedTriangleVertexPositionItemCount);
            triangulatedFace.indicies[j] = triangleIndices[firstIndex + j];
            Vertex *v = &m_triangleVertices[firstIndex + j];
            v->posX = triangleVertexPositions[posIndex + 0];
            v->posY = triangleVertexPositions[posIndex + 1];
            v->posZ = triangleVertexPositions[posIndex + 2];
            assert(firstIndex + 2 < loadedTriangleNormalItemCount);
            v->normX = triangleNormals[firstIndex + 0];
            v->normY = triangleNormals[firstIndex + 1];
            v->normZ = triangleNormals[firstIndex + 2];
            v->colorR = useColorR;
            v->colorG = useColorG;
            v->colorB = useColorB;
            v->metalness = useMetalness;
            v->roughness = useRoughness;
        }
        m_triangulatedFaces.push_back(triangulatedFace);
    }
    
    if (smoothNormal) {
        std::vector<QVector3D> resultNormals;
        angleSmooth(inputVerticesForSmoothAngle, inputTrianglesForSmoothAngle, inputNormalsForSmoothAngle,
            thresholdAngleDegrees, resultNormals);
        size_t normalIndex = 0;
        for (int i = 0; i < triangleCount; i++) {
            int firstIndex = i * 3;
            for (int j = 0; j < 3; j++) {
                assert(firstIndex + j < loadedTriangleVertexIndexItemCount);
                Vertex *v = &m_triangleVertices[firstIndex + j];
                const auto &normal = resultNormals[normalIndex++];
                v->normX = normal.x();
                v->normY = normal.y();
                v->normZ = normal.z();
            }
        }
    }
    
    delete[] triangleVertexPositions;
    delete[] triangleVertexSmoothNormals;
    delete[] triangleIndices;
    delete[] triangleNormals;
    
    delete[] edgeVertexPositions;
    delete[] edgeIndices;
    delete[] edgeNormals;
}

MeshLoader::MeshLoader(const MeshLoader &mesh) :
    m_triangleVertices(nullptr),
    m_triangleVertexCount(0),
    m_edgeVertices(nullptr),
    m_edgeVertexCount(0),
    m_textureImage(nullptr)
{
    if (nullptr != mesh.m_triangleVertices &&
            mesh.m_triangleVertexCount > 0) {
        this->m_triangleVertices = new Vertex[mesh.m_triangleVertexCount];
        this->m_triangleVertexCount = mesh.m_triangleVertexCount;
        for (int i = 0; i < mesh.m_triangleVertexCount; i++)
            this->m_triangleVertices[i] = mesh.m_triangleVertices[i];
    }
    if (nullptr != mesh.m_edgeVertices &&
            mesh.m_edgeVertexCount > 0) {
        this->m_edgeVertices = new Vertex[mesh.m_edgeVertexCount];
        this->m_edgeVertexCount = mesh.m_edgeVertexCount;
        for (int i = 0; i < mesh.m_edgeVertexCount; i++)
            this->m_edgeVertices[i] = mesh.m_edgeVertices[i];
    }
    if (nullptr != mesh.m_textureImage) {
        this->m_textureImage = new QImage(*mesh.m_textureImage);
    }
    if (nullptr != mesh.m_normalMapImage) {
        this->m_normalMapImage = new QImage(*mesh.m_normalMapImage);
    }
    this->m_vertices = mesh.m_vertices;
    this->m_faces = mesh.m_faces;
    this->m_triangulatedVertices = mesh.m_triangulatedVertices;
    this->m_triangulatedFaces = mesh.m_triangulatedFaces;
}

MeshLoader::MeshLoader(Vertex *triangleVertices, int vertexNum) :
    m_triangleVertices(triangleVertices),
    m_triangleVertexCount(vertexNum),
    m_edgeVertices(nullptr),
    m_edgeVertexCount(0),
    m_textureImage(nullptr)
{
}

MeshLoader::MeshLoader(MeshResultContext &resultContext) :
    m_triangleVertices(nullptr),
    m_triangleVertexCount(0),
    m_edgeVertices(nullptr),
    m_edgeVertexCount(0),
    m_textureImage(nullptr)
{
    for (const auto &part: resultContext.parts()) {
        m_triangleVertexCount += part.second.triangles.size() * 3;
    }
    m_triangleVertices = new Vertex[m_triangleVertexCount];
    int destIndex = 0;
    for (const auto &part: resultContext.parts()) {
        for (int x = 0; x < (int)part.second.triangles.size(); x++) {
            const auto &it = part.second.triangles[x];
            for (auto i = 0; i < 3; i++) {
                int vertexIndex = it.indicies[i];
                const ResultVertex *srcVert = &part.second.vertices[vertexIndex];
                const QVector3D *srcNormal = &part.second.interpolatedVertexNormals[vertexIndex];
                const ResultVertexUv *srcUv = &part.second.vertexUvs[vertexIndex];
                //const Material *srcMaterial = &part.second.material;
                const QVector3D *srcTangent = &part.second.triangleTangents[x];
                Vertex *dest = &m_triangleVertices[destIndex];
                dest->colorR = 0;
                dest->colorG = 0;
                dest->colorB = 0;
                dest->posX = srcVert->position.x();
                dest->posY = srcVert->position.y();
                dest->posZ = srcVert->position.z();
                dest->texU = srcUv->uv[0];
                dest->texV = srcUv->uv[1];
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
    delete m_textureImage;
    delete m_normalMapImage;
}

const std::vector<QVector3D> &MeshLoader::vertices()
{
    return m_vertices;
}

const std::vector<std::vector<int>> &MeshLoader::faces()
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

Vertex *MeshLoader::triangleVertices()
{
    return m_triangleVertices;
}

int MeshLoader::triangleVertexCount()
{
    return m_triangleVertexCount;
}

Vertex *MeshLoader::edgeVertices()
{
    return m_edgeVertices;
}

int MeshLoader::edgeVertexCount()
{
    return m_edgeVertexCount;
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

void MeshLoader::exportAsObj(const QString &filename)
{
    QFile file(filename);
    if (file.open(QIODevice::WriteOnly)) {
        QTextStream stream(&file);
        stream << "# " << Ds3FileReader::m_applicationName << endl;
        for (std::vector<QVector3D>::const_iterator it = vertices().begin() ; it != vertices().end(); ++it) {
            stream << "v " << (*it).x() << " " << (*it).y() << " " << (*it).z() << endl;
        }
        for (std::vector<std::vector<int>>::const_iterator it = faces().begin() ; it != faces().end(); ++it) {
            stream << "f";
            for (std::vector<int>::const_iterator subIt = (*it).begin() ; subIt != (*it).end(); ++subIt) {
                stream << " " << (1 + *subIt);
            }
            stream << endl;
        }
    }
}
