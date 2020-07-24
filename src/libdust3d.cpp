#include <string>
#include <fstream>
#include <streambuf>
#include <sstream>
#include <QVector3D>
#include <QString>
#include <QCoreApplication>
#include "dust3d.h"
#include "meshgenerator.h"
#include "snapshot.h"
#include "snapshotxml.h"
#include "model.h"
#include "version.h"

struct _dust3d
{
    void *userData = nullptr;
    GeneratedCacheContext *cacheContext = nullptr;
    Model *resultMesh = nullptr;
    Snapshot *snapshot = nullptr;
    Outcome *outcome = nullptr;
    int error = DUST3D_ERROR;
};

DUST3D_DLL int DUST3D_API dust3dError(dust3d *ds3)
{
    return ds3->error;
}

DUST3D_DLL void DUST3D_API dust3dClose(dust3d *ds3)
{
    delete ds3->resultMesh;
    ds3->resultMesh = nullptr;
    
    delete ds3->cacheContext;
    ds3->cacheContext = nullptr;
    
    delete ds3->snapshot;
    ds3->snapshot = nullptr;
    
    delete ds3->outcome;
    ds3->outcome = nullptr;
    
    delete ds3;
}

DUST3D_DLL void DUST3D_API dust3dInitialize(int argc, char *argv[])
{
    if (nullptr == QCoreApplication::instance())
        new QCoreApplication(argc, argv);
}

DUST3D_DLL dust3d * DUST3D_API dust3dOpenFromMemory(const char *documentType, const char *buffer, int size)
{
    dust3d *ds3 = new dust3d;
    
    ds3->error = DUST3D_ERROR;
    
    delete ds3->outcome;
    ds3->outcome = new Outcome;
    
    if (0 == strcmp(documentType, "xml")) {
        QByteArray data(buffer, size);
        
        QXmlStreamReader stream(data);
        
        delete ds3->snapshot;
        ds3->snapshot = new Snapshot;
        
        loadSkeletonFromXmlStream(ds3->snapshot, stream);
        
        ds3->error = DUST3D_OK;
    } else {
        ds3->error = DUST3D_UNSUPPORTED;
    }
    
    return ds3;
}

DUST3D_DLL dust3d * DUST3D_API dust3dOpen(const char *fileName)
{
    std::string name = fileName;
    auto dotIndex = name.rfind('.');
    if (std::string::npos == dotIndex)
        return nullptr;

    std::string extension = name.substr(dotIndex + 1);

    std::ifstream is(name, std::ifstream::binary);
    if (!is)
        return nullptr;
    
    is.seekg (0, is.end);
    int length = is.tellg();
    is.seekg (0, is.beg);

    char *buffer = new char[length + 1];
    is.read(buffer, length);
    is.close();
    buffer[length] = '\0';
    
    dust3d *ds3 = dust3dOpenFromMemory(extension.c_str(), buffer, length);
    
    delete[] buffer;
    
    return ds3;
}

DUST3D_DLL void DUST3D_API dust3dSetUserData(dust3d *ds3, void *userData)
{
    ds3->userData = userData;
}

DUST3D_DLL void * DUST3D_API dust3dGetUserData(dust3d *ds3)
{
    return ds3->userData;
}

DUST3D_DLL const char * DUST3D_API dust3dVersion(void)
{
    return APP_NAME " " APP_HUMAN_VER;
}

DUST3D_DLL int DUST3D_API dust3dGenerateMesh(dust3d *ds3)
{
    ds3->error = DUST3D_ERROR;
    
    if (nullptr == ds3->snapshot)
        return ds3->error;
    
    if (nullptr == ds3->cacheContext)
        ds3->cacheContext = new GeneratedCacheContext();
    
    Snapshot *snapshot = ds3->snapshot;
    ds3->snapshot = nullptr;
    
    MeshGenerator *meshGenerator = new MeshGenerator(snapshot);
    meshGenerator->setGeneratedCacheContext(ds3->cacheContext);
    meshGenerator->generate();
    
    delete ds3->outcome;
    ds3->outcome = meshGenerator->takeOutcome();
    if (nullptr == ds3->outcome)
        ds3->outcome = new Outcome;
    
    if (meshGenerator->isSuccessful())
        ds3->error = DUST3D_OK;
    
    delete meshGenerator;
    
    return ds3->error;
}

DUST3D_DLL int DUST3D_API dust3dGetMeshVertexCount(dust3d *ds3)
{
    return (int)ds3->outcome->vertices.size();
}

DUST3D_DLL int DUST3D_API dust3dGetMeshTriangleCount(dust3d *ds3)
{
    return (int)ds3->outcome->triangles.size();
}

DUST3D_DLL void DUST3D_API dust3dGetMeshTriangleIndices(dust3d *ds3, int *indices)
{
    for (const auto &it: ds3->outcome->triangles) {
        *(indices++) = (int)it[0];
        *(indices++) = (int)it[1];
        *(indices++) = (int)it[2];
    }
}

DUST3D_DLL void DUST3D_API dust3dGetMeshTriangleColors(dust3d *ds3, unsigned int *colors)
{
    for (const auto &it: ds3->outcome->triangleColors) {
        *(colors++) = ((unsigned int)it.red() << 16) | ((unsigned int)it.green() << 8) | ((unsigned int)it.blue() << 0);
    }
}

DUST3D_DLL void DUST3D_API dust3dGetMeshVertexPosition(dust3d *ds3, int vertexIndex, float *x, float *y, float *z)
{
    if (vertexIndex >= 0 && vertexIndex < ds3->outcome->vertices.size()) {
        const auto &v = ds3->outcome->vertices[vertexIndex];
        *x = v.x();
        *y = v.y();
        *z = v.z();
    }
}

DUST3D_DLL void DUST3D_API dust3dGetMeshVertexSource(dust3d *ds3, int vertexIndex, unsigned char partId[16], unsigned char nodeId[16])
{
    if (vertexIndex >= 0 && vertexIndex < ds3->outcome->vertices.size()) {
        const auto &source = ds3->outcome->vertexSourceNodes[vertexIndex];
        
        auto sourcePartUuid = source.first.toByteArray(QUuid::Id128);
        memcpy(partId, sourcePartUuid.constData(), sizeof(partId));
        
        auto sourceNodeUuid = source.second.toByteArray(QUuid::Id128);
        memcpy(partId, sourceNodeUuid.constData(), sizeof(nodeId));
    }
}

DUST3D_DLL int DUST3D_API dust3dGetMeshTriangleAndQuadCount(dust3d *ds3)
{
    return (int)ds3->outcome->triangleAndQuads.size();
}

DUST3D_DLL void DUST3D_API dust3dGetMeshTriangleAndQuadIndices(dust3d *ds3, int *indices)
{
    for (const auto &it: ds3->outcome->triangleAndQuads) {
        *(indices++) = (int)it[0];
        *(indices++) = (int)it[1];
        *(indices++) = (int)it[2];
        if (it.size() > 3)
            *(indices++) = (int)it[3];
        else
            *(indices++) = (int)-1;
    }
}

