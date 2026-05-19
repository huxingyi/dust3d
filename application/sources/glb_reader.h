#ifndef DUST3D_APPLICATION_GLB_READER_H_
#define DUST3D_APPLICATION_GLB_READER_H_

#include <QByteArray>
#include <QImage>
#include <dust3d/base/vector2.h>
#include <dust3d/base/vector3.h>
#include <dust3d/mesh/mesh_generator.h>
#include <vector>

class GlbReader {
public:
    static bool read(const QByteArray& data, dust3d::MeshGenerator::ImportedModelData& result, QImage* textureImage = nullptr);
};

#endif
