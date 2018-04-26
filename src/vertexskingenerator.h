#ifndef VERTEX_SKIN_GENERATOR_H
#define VERTEX_SKIN_GENERATOR_H
#include "meshresultcontext.h"

// https://raw.githubusercontent.com/KhronosGroup/glTF/master/specification/2.0/figures/gltfOverview-2.0.0a.png

class VertexSkinGenerator
{
public:
    VertexSkinGenerator(MeshResultContext *meshResultContext);
signals:
    void finished();
public slots:
    void process();
private:
    MeshResultContext *m_meshResultContext;
public:
    static float m_positionMapGridSize;
};

#endif
