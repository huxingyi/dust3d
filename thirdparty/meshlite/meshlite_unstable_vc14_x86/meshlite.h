#ifndef MESHLITE_H
#define MESHLITE_H

#ifdef __cplusplus
extern "C" {
#endif

void *meshlite_create_context(void);
int meshlite_destroy_context(void *context);
int meshlite_merge(void *context, int first_mesh_id, int second_mesh_id);
int meshlite_import(void *context, const char *filename);
int meshlite_export(void *context, int mesh_id, const char *filename);
int meshlite_clone(void *context, int from_mesh_id);
int meshlite_triangulate(void *context, int mesh_id);
int meshlite_is_triangulated_manifold(void *context, int mesh_id);
int meshlite_subdivide(void *context, int mesh_id);
int meshlite_union(void *context, int first_mesh_id, int second_mesh_id);
int meshlite_diff(void *context, int first_mesh_id, int second_mesh_id);
int meshlite_intersect(void *context, int first_mesh_id, int second_mesh_id);
int meshlite_scale(void *context, int mesh_id, float value);
int meshlite_get_vertex_count(void *context, int mesh_id);
int meshlite_get_vertex_position_array(void *context, int mesh_id, float *buffer, int max_buffer_len);
int meshlite_get_vertex_source_array(void *context, int mesh_id, int *buffer, int max_buffer_len);
int meshlite_get_face_count(void *context, int mesh_id);
int meshlite_get_face_index_array(void *context, int mesh_id, int *buffer, int max_buffer_len);
int meshlite_get_triangle_index_array(void *context, int mesh_id, int *buffer, int max_buffer_len);
int meshlite_get_triangle_normal_array(void *context, int mesh_id, float *buffer, int max_buffer_len);
int meshlite_get_edge_count(void *context, int mesh_id);
int meshlite_get_edge_index_array(void *context, int mesh_id, int *buffer, int max_buffer_len);
int meshlite_get_edge_normal_array(void *context, int mesh_id, float *buffer, int max_buffer_len);
int meshlite_get_halfedge_count(void *context, int mesh_id);
int meshlite_get_halfedge_index_array(void *context, int mesh_id, int *buffer, int max_buffer_len);
int meshlite_get_halfedge_normal_array(void *context, int mesh_id, float *buffer, int max_buffer_len);
int meshlite_build(void *context, float *vertex_position_buffer, int vertex_count, int *face_index_buffer, int face_index_buffer_len);
int meshlite_bmesh_create(void *context);
int meshlite_bmesh_set_cut_subdiv_count(void *context, int bmesh_id, int subdiv_count);
int meshlite_bmesh_set_round_way(void *context, int bmesh_id, int round_way);
int meshlite_bmesh_set_deform_thickness(void *context, int bmesh_id, float thickness);
int meshlite_bmesh_set_deform_width(void *context, int bmesh_id, float width);
int meshlite_bmesh_enable_debug(void *context, int bmesh_id, int enable);
int meshlite_bmesh_add_node(void *context, int bmesh_id, float x, float y, float z, float radius);
int meshlite_bmesh_set_node_cut_subdiv_count(void *context, int bmesh_id, int node_id, int subdiv_count);
int meshlite_bmesh_set_node_round_way(void *context, int bmesh_id, int node_id, int round_way);
int meshlite_bmesh_add_edge(void *context, int bmesh_id, int first_node_id, int second_node_id);
int meshlite_bmesh_generate_mesh(void *context, int bmesh_id);
int meshlite_bmesh_get_node_base_norm(void *context, int bmesh_id, int node_id, float *norm_buffer);
int meshlite_bmesh_destroy(void *context, int bmesh_id);
int meshlite_bmesh_error_count(void *context, int bmesh_id);
int meshlite_bmesh_add_seam_requirement(void *context, int bmesh_id);
int meshlite_bmesh_get_seam_count(void *context, int bmesh_id);
int meshlite_bmesh_get_seam_index_array(void *context, int bmesh_id, int *buffer, int max_buffer_len);
int meshlite_combine_adj_faces(void *context, int mesh_id);
int meshlite_combine_coplanar_faces(void *context, int mesh_id);
int meshlite_trim(void *context, int mesh_id, int normalize);
int meshlite_mirror_in_x(void *context, int mesh_id, float center_x);
int meshlite_mirror_in_z(void *context, int mesh_id, float center_z);
int meshlite_fix_hole(void *context, int mesh_id);
int meshlite_skeletonmesh_create(void *context);
int meshlite_skeletonmesh_set_end_radius(void *context, float radius);
int meshlite_skeletonmesh_add_bone(void *context, int sklt_id, float from_x, float from_y, float from_z, float to_x, float to_y, float to_z);
int meshlite_skeletonmesh_generate_mesh(void *context, int sklt_id);

#ifdef __cplusplus
}
#endif

#endif
