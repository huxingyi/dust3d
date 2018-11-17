#ifndef SIMPLEUV_MESH_DATA_TYPE_H
#define SIMPLEUV_MESH_DATA_TYPE_H
#include <vector>
#include <cstdlib>

namespace simpleuv 
{

struct Vertex
{
    float xyz[3];
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
    std::vector<Vertex> verticies;
    std::vector<Face> faces;
    std::vector<int> facePartitions;
};

}

#endif
