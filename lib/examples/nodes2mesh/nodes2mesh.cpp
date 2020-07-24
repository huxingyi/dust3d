#include <iostream>
#include <stdio.h>
#include <dust3d.h>

int main(int argc, char *argv[])
{
	dust3dInitialize(argc, argv);

	printf("dust3d version:%s\n", dust3dVersion());

	dust3d *ds3 = dust3dOpen("screwdriver.xml");
	if (ds3) {
		int error = dust3dGenerateMesh(ds3);
		printf("error: %d\n", error);
		{
			FILE *fp = nullptr;
			fopen_s(&fp, "test-triangles.obj", "wb");
			int vertexCount = dust3dGetMeshVertexCount(ds3);
			for (int i = 0; i < vertexCount; ++i) {
				float x, y, z;
				dust3dGetMeshVertexPosition(ds3, i, &x, &y, &z);
				fprintf(fp, "v %f %f %f\n", x, y, z);
			}
			int triangleCount = dust3dGetMeshTriangleCount(ds3);
			if (triangleCount > 0) {
				int *triangleIndices = new int[triangleCount * 3];
				dust3dGetMeshTriangleIndices(ds3, triangleIndices);
				for (int i = 0, offset = 0; i < triangleCount; ++i, offset += 3) {
					fprintf(fp, "f %d %d %d\n",
						1 + triangleIndices[offset],
						1 + triangleIndices[offset + 1],
						1 + triangleIndices[offset + 2]);
				}
				delete[] triangleIndices;
			}
			fclose(fp);
		}
		{
			FILE *fp = nullptr;
			fopen_s(&fp, "test-triangleandquads.obj", "wb");
			int vertexCount = dust3dGetMeshVertexCount(ds3);
			for (int i = 0; i < vertexCount; ++i) {
				float x, y, z;
				dust3dGetMeshVertexPosition(ds3, i, &x, &y, &z);
				fprintf(fp, "v %f %f %f\n", x, y, z);
			}
			int triangleAndQuadCount = dust3dGetMeshTriangleAndQuadCount(ds3);
			if (triangleAndQuadCount > 0) {
				int *faceIndices = new int[triangleAndQuadCount * 4];
				dust3dGetMeshTriangleAndQuadIndices(ds3, faceIndices);
				for (int i = 0, offset = 0; i < triangleAndQuadCount; ++i, offset += 4) {
					if (-1 == faceIndices[offset + 3]) {
						fprintf(fp, "f %d %d %d\n",
							1 + faceIndices[offset],
							1 + faceIndices[offset + 1],
							1 + faceIndices[offset + 2]);
					} else {
						fprintf(fp, "f %d %d %d %d\n",
							1 + faceIndices[offset],
							1 + faceIndices[offset + 1],
							1 + faceIndices[offset + 2],
							1 + faceIndices[offset + 3]);
					}
				}
				delete[] faceIndices;
			}
			fclose(fp);
		}
		dust3dClose(ds3);
	}
}
