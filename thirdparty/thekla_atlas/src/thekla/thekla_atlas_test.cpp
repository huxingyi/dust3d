
#include "thekla_atlas.h"
#include "thekla_mesh_load.h"

#include <stdio.h>
#include <assert.h>


using namespace Thekla;

int main(int argc, char * argv[]) {

    if (argc != 2) {
        printf("Usage: %s input_file.obj\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Load Obj_Mesh.
    Obj_Load_Options load_options = {0};
    Obj_Mesh * obj_mesh = obj_mesh_load(argv[1], &load_options);

    if (obj_mesh == NULL) {
        printf("Error loading obj file.\n");
        return 1;
    }

    // Convert Obj_Mesh to Atlast_Input_Mesh.
    assert(sizeof(Atlas_Input_Vertex) == sizeof(Obj_Vertex));
    assert(sizeof(Atlas_Input_Face) == sizeof(Obj_Face));

    Atlas_Input_Mesh input_mesh;
    input_mesh.vertex_count = obj_mesh->vertex_count;
    input_mesh.vertex_array = (Atlas_Input_Vertex *)obj_mesh->vertex_array;
    input_mesh.face_count = obj_mesh->face_count;
    input_mesh.face_array = (Atlas_Input_Face *)obj_mesh->face_array;


    // Generate Atlas_Output_Mesh.
    Atlas_Options atlas_options;
    atlas_set_default_options(&atlas_options);

    // Avoid brute force packing, since it can be unusably slow in some situations.
    atlas_options.packer_options.witness.packing_quality = 1;

    Atlas_Error error = Atlas_Error_Success;
    Atlas_Output_Mesh * output_mesh = atlas_generate(&input_mesh, &atlas_options, &error);

    printf("Atlas mesh has %d verts\n", output_mesh->vertex_count);
    printf("Atlas mesh has %d triangles\n", output_mesh->index_count / 3);
    printf("Produced debug_packer_final.tga\n");

    // Free meshes.
    obj_mesh_free(obj_mesh);
    atlas_free(output_mesh);
 
    return 0;
}
