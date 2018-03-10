#include "mesh.h"
#include "meshlite.h"
#include <assert.h>

Mesh::Mesh(void *meshlite, int mesh_id) :
    m_vertices(NULL),
    m_vertexCount(0)
{
    int positionCount = meshlite_get_vertex_count(meshlite, mesh_id);
    GLfloat *positions = new GLfloat[positionCount * 3];
    int loadedPositionItemCount = meshlite_get_vertex_position_array(meshlite, mesh_id, positions, positionCount * 3);
    
    int triangleCount = meshlite_get_face_count(meshlite, mesh_id);
    int *triangleIndices = new int[triangleCount * 3];
    int loadedIndexItemCount = meshlite_get_triangle_index_array(meshlite, mesh_id, triangleIndices, triangleCount * 3);
    GLfloat *normals = new GLfloat[triangleCount * 3];
    int loadedNormalItemCount = meshlite_get_triangle_normal_array(meshlite, mesh_id, normals, triangleCount * 3);
    
    m_vertexCount = triangleCount * 3;
    m_vertices = new Vertex[m_vertexCount * 3];
    for (int i = 0; i < triangleCount; i++) {
        int firstIndex = i * 3;
        for (int j = 0; j < 3; j++) {
            assert(firstIndex + j < loadedIndexItemCount);
            int posIndex = triangleIndices[firstIndex + j] * 3;
            assert(posIndex < loadedPositionItemCount);
            Vertex *v = &m_vertices[firstIndex + j];
            v->posX = positions[posIndex + 0];
            v->posY = positions[posIndex + 1];
            v->posZ = positions[posIndex + 2];
            assert(firstIndex + 2 < loadedNormalItemCount);
            v->normX = normals[firstIndex + 0];
            v->normY = normals[firstIndex + 1];
            v->normZ = normals[firstIndex + 2];
        }
    }
    
    delete[] positions;
    delete[] triangleIndices;
    delete[] normals;
}

Mesh::~Mesh()
{
    delete[] m_vertices;
    m_vertexCount = 0;
}

Vertex *Mesh::vertices()
{
    return m_vertices;
}

int Mesh::vertexCount()
{
    return m_vertexCount;
}
