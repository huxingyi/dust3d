#ifndef SIMPLEUV_PARAMETRIZE_H
#define SIMPLEUV_PARAMETRIZE_H
#include <simpleuv/meshdatatype.h>

namespace simpleuv
{

bool parametrize(const std::vector<Vertex> &verticies, 
        const std::vector<Face> &faces, 
        std::vector<TextureCoord> &vertexUvs);

}

#endif
