#ifndef DUST3D_H
#define DUST3D_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
# ifdef DUST3D_EXPORTING
#  define DUST3D_DLL                    __declspec(dllexport)
# else
#  define DUST3D_DLL                    __declspec(dllimport)
# endif
#else
# define DUST3D_DLL
#endif

# define DUST3D_API                     __stdcall

#define DUST3D_OK                   0
#define DUST3D_ERROR                1
#define DUST3D_UNSUPPORTED          2

typedef struct _dust3d dust3d;

DUST3D_DLL void         DUST3D_API dust3dInitialize(int argc, char *argv[]);
DUST3D_DLL dust3d *     DUST3D_API dust3dOpenFromMemory(const char *documentType, const char *buffer, int size);
DUST3D_DLL dust3d *     DUST3D_API dust3dOpen(const char *fileName);
DUST3D_DLL void         DUST3D_API dust3dSetUserData(dust3d *ds3, void *userData);
DUST3D_DLL void *       DUST3D_API dust3dGetUserData(dust3d *ds3);
DUST3D_DLL int          DUST3D_API dust3dGenerateMesh(dust3d *ds3);
DUST3D_DLL int          DUST3D_API dust3dGetMeshVertexCount(dust3d *ds3);
DUST3D_DLL int          DUST3D_API dust3dGetMeshTriangleCount(dust3d *ds3);
DUST3D_DLL void         DUST3D_API dust3dGetMeshTriangleIndices(dust3d *ds3, int *indices);
DUST3D_DLL void         DUST3D_API dust3dGetMeshTriangleColors(dust3d *ds3, unsigned int *colors);
DUST3D_DLL void         DUST3D_API dust3dGetMeshVertexPosition(dust3d *ds3, int vertexIndex, float *x, float *y, float *z);
DUST3D_DLL void         DUST3D_API dust3dGetMeshVertexSource(dust3d *ds3, int vertexIndex, unsigned char partId[16], unsigned char nodeId[16]);
DUST3D_DLL int          DUST3D_API dust3dGetMeshTriangleAndQuadCount(dust3d *ds3);
DUST3D_DLL void         DUST3D_API dust3dGetMeshTriangleAndQuadIndices(dust3d *ds3, int *indices);
DUST3D_DLL void         DUST3D_API dust3dClose(dust3d *ds3);
DUST3D_DLL int          DUST3D_API dust3dError(dust3d *ds3);
DUST3D_DLL const char * DUST3D_API dust3dVersion(void);

#ifdef __cplusplus
}
#endif

#endif
