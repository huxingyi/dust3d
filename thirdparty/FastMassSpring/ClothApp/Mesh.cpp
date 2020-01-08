#include "Mesh.h"

// M E S H /////////////////////////////////////////////////////////////////////////////////////
float* Mesh::vbuff() { return VERTEX_DATA(this); }
float* Mesh::nbuff() { return NORMAL_DATA(this); }
float* Mesh::tbuff() { return TEXTURE_DATA(this); }
unsigned int* Mesh::ibuff() { return &_ibuff[0]; }
void Mesh::useIBuff(std::vector<unsigned int>& _ibuff) { this->_ibuff = _ibuff; }

unsigned int Mesh::vbuffLen() { return (unsigned int)n_vertices() * 3; }
unsigned int Mesh::nbuffLen() { return (unsigned int)n_vertices() * 3; }
unsigned int Mesh::tbuffLen() { return (unsigned int)n_vertices() * 2; }
unsigned int Mesh::ibuffLen() { return (unsigned int)_ibuff.size(); }


// M E S H  B U I L D E R /////////////////////////////////////////////////////////////////////
void MeshBuilder::uniformGrid(float w, int n) {
	result = new Mesh;
	unsigned int ibuffLen = 6 * (n - 1) * (n - 1);
	std::vector<unsigned int> ibuff(ibuffLen);

	// request mesh properties
	result->request_vertex_normals();
	result->request_vertex_normals();
	result->request_vertex_texcoords2D();

	// generate mesh
	unsigned int idx = 0; // vertex index
	const float d = w / (n - 1); // step distance
	const float ud = 1.0f / (n - 1); // unit step distance
	const OpenMesh::Vec3f o = OpenMesh::Vec3f(-w/2.0f, w/2.0f, 0.0f); // origin
	const OpenMesh::Vec3f ux = OpenMesh::Vec3f(1.0f, 0.0f, 0.0f); // unit x direction
	const OpenMesh::Vec3f uy = OpenMesh::Vec3f(0.0f, -1.0f, 0.0f); // unit y direction
	std::vector<OpenMesh::VertexHandle> handle_table(n * n); // table storing vertex handles for easy grid connectivity establishment

	for (int i = 0; i < n; i++) {
		for (int j = 0; j < n; j++) {
			handle_table[j + i * n] = result->add_vertex(o + d*j*ux + d*i*uy); // add vertex
			result->set_texcoord2D(handle_table[j + i * n], OpenMesh::Vec2f(ud*j, ud*i)); // add texture coordinates

			//add connectivity
			if (i > 0 && j < n - 1) {
				result->add_face(
					handle_table[j + i * n],
					handle_table[j + 1 + (i - 1) * n],
					handle_table[j + (i - 1) * n]
				);

				ibuff[idx++] = j + i * n;
				ibuff[idx++] = j + 1 + (i - 1) * n;
				ibuff[idx++] = j + (i - 1) * n;
			}

			if (j > 0 && i > 0) {
				result->add_face(
					handle_table[j + i * n],
					handle_table[j + (i - 1) * n],
					handle_table[j - 1 + i * n]
				);

				ibuff[idx++] = j + i * n;
				ibuff[idx++] = j + (i - 1) * n;
				ibuff[idx++] = j - 1 + i * n;
			}
		}
	}

	// calculate normals
	result->request_face_normals();
	result->update_normals();
	result->release_face_normals();

	// set index buffer
	result->useIBuff(ibuff);
}
Mesh* MeshBuilder::getResult() { return result; }