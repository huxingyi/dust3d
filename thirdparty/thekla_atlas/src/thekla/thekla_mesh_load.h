
#include <stdio.h> // FILE

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

namespace Thekla {

struct Obj_Vertex {
    float position[3];
    float normal[3];
    float uv[2];
    int first_colocal;
};

struct Obj_Face {
    int vertex_index[3];
    int material_index;
};

struct Obj_Material {
    // @@ Read obj mtl parameters as is.
};

struct Obj_Mesh {
    int vertex_count;
    Obj_Vertex * vertex_array;

    int face_count;
    Obj_Face * face_array;

    int material_count;
    Obj_Material * material_array;
};

enum Load_Flags {
    Load_Flag_Weld_Attributes,
};

struct Obj_Load_Options {
    int load_flags;
};

Obj_Mesh * obj_mesh_load(const char * filename, const Obj_Load_Options * options);
void obj_mesh_free(Obj_Mesh * mesh);



Obj_Mesh * obj_mesh_load(const char * filename, const Obj_Load_Options * options) {

    using namespace std;
    vector<tinyobj::shape_t> shapes;
    vector<tinyobj::material_t> materials;
    string err;
    bool ret = tinyobj::LoadObj(shapes, materials, err, filename);
    if (!ret) {
        printf("%s\n", err.c_str());
        return NULL;
    }

    printf("%zu shapes\n", shapes.size());
    printf("%zu materials\n", materials.size());

    assert(shapes.size() > 0);

    Obj_Mesh* mesh = new Obj_Mesh();

    // Count vertices and faces in all shapes.
    mesh->vertex_count = 0;
    mesh->face_count = 0;
    for (int s = 0; s < shapes.size(); s++) {
        mesh->vertex_count += (int)shapes[s].mesh.positions.size() / 3;
        mesh->face_count += (int)shapes[s].mesh.indices.size() / 3;
    }

    mesh->vertex_array = new Obj_Vertex[mesh->vertex_count];
    for (int s = 0, nv = 0; s < shapes.size(); s++) {
        const int shape_vertex_count = (int)shapes[s].mesh.positions.size() / 3;

        for (int v = 0; v < shape_vertex_count; v++) {
            mesh->vertex_array[nv+v].position[0] = shapes[s].mesh.positions[v * 3 + 0];
            mesh->vertex_array[nv+v].position[1] = shapes[s].mesh.positions[v * 3 + 1];
            mesh->vertex_array[nv+v].position[2] = shapes[s].mesh.positions[v * 3 + 2];
            mesh->vertex_array[nv+v].normal[0] = shapes[s].mesh.normals[v * 3 + 0];
            mesh->vertex_array[nv+v].normal[1] = shapes[s].mesh.normals[v * 3 + 1];
            mesh->vertex_array[nv+v].normal[2] = shapes[s].mesh.normals[v * 3 + 2];
            mesh->vertex_array[nv+v].uv[0] = shapes[s].mesh.texcoords[v * 3 + 0];      // The input UVs are provided as a hint to the chart generator.
            mesh->vertex_array[nv+v].uv[1] = shapes[s].mesh.texcoords[v * 3 + 1];
            mesh->vertex_array[nv+v].first_colocal = nv+v;

            // Link colocals. You probably want to do this more efficiently! Sort by one axis or use a hash or grid.
            for (int vv = 0; vv < v; vv++) {
                if (mesh->vertex_array[nv+v].position[0] == mesh->vertex_array[nv+vv].position[0] &&
                    mesh->vertex_array[nv+v].position[1] == mesh->vertex_array[nv+vv].position[1] &&
                    mesh->vertex_array[nv+v].position[2] == mesh->vertex_array[nv+vv].position[2])
                {
                    mesh->vertex_array[nv+v].first_colocal = nv+vv;
                }
            }
        }
        nv += shape_vertex_count;
    }

    mesh->face_array = new Obj_Face[mesh->face_count];
    for (int s = 0, nf = 0; s < shapes.size(); s++) {
        const int shape_face_count = (int)shapes[s].mesh.indices.size() / 3;
        for (int f = 0; f < shape_face_count; f++) {
            mesh->face_array[nf+f].material_index = 0;
            mesh->face_array[nf+f].vertex_index[0] = shapes[s].mesh.indices[f * 3 + 0];
            mesh->face_array[nf+f].vertex_index[1] = shapes[s].mesh.indices[f * 3 + 1];
            mesh->face_array[nf+f].vertex_index[2] = shapes[s].mesh.indices[f * 3 + 2];
        }
        nf += shape_face_count;
    }

    // @@ Add support for obj materials! Charter also uses material boundaries as a hint to cut charts.

    printf("Reading %d verts\n", mesh->vertex_count);
    printf("Reading %d triangles\n", mesh->face_count);

    mesh->material_count = 0;
    mesh->material_array = 0;

    return mesh;
}


void obj_mesh_free(Obj_Mesh * mesh) {
    if (mesh != NULL) {
        delete [] mesh->vertex_array;
        delete [] mesh->face_array;
        delete [] mesh->material_array;
        delete mesh;
    }
}

} // Thekla namespace
