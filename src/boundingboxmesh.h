#ifndef DUST3D_BOUNDING_BOX_MESH
#define DUST3D_BOUNDING_BOX_MESH
#include <vector>
#include <QVector3D>
#include <QColor>
#include "shadervertex.h"
#include "meshloader.h"

ShaderVertex *buildBoundingBoxMeshEdges(const std::vector<std::tuple<QVector3D, QVector3D, float, float, QColor>> &boxes,
        int *edgeVerticesNum);
MeshLoader *buildBoundingBoxMesh(const std::vector<std::tuple<QVector3D, QVector3D, float, float, QColor>> &boxes);

#endif
