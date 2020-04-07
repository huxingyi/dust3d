#ifndef DUST3D_BOUNDING_BOX_MESH
#define DUST3D_BOUNDING_BOX_MESH
#include <vector>
#include <QVector3D>
#include <QColor>
#include "shadervertex.h"
#include "model.h"

ShaderVertex *buildBoundingBoxMeshEdges(const std::vector<std::tuple<QVector3D, QVector3D, float, float, QColor>> &boxes,
        int *edgeVerticesNum);
Model *buildBoundingBoxMesh(const std::vector<std::tuple<QVector3D, QVector3D, float, float, QColor>> &boxes);

#endif
