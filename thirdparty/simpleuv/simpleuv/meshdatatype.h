#ifndef SIMPLEUV_MESH_DATA_TYPE_H
#define SIMPLEUV_MESH_DATA_TYPE_H
#include <vector>
#include <cstdlib>

namespace simpleuv 
{

struct Vector3
{
    float xyz[3];
};

typedef Vector3 Vertex;

struct Rect
{
    float left;
    float top;
    float width;
    float height;
};

struct Face
{
    size_t indices[3];
};

struct TextureCoord
{
    float uv[2];
};

struct FaceTextureCoords
{
    TextureCoord coords[3];
};

struct Mesh
{
    std::vector<Vertex> vertices;
    std::vector<Face> faces;
    std::vector<Vector3> faceNormals;
    std::vector<int> facePartitions;
};

float dotProduct(const Vector3 &first, const Vector3 &second);
Vector3 crossProduct(const Vector3 &first, const Vector3 &second);

}

#endif
