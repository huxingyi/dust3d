#ifndef SIMPLEUV_TRIANGULATE_H
#define SIMPLEUV_TRIANGULATE_H
#include <simpleuv/meshdatatype.h>

namespace simpleuv
{

void triangulate(const std::vector<Vertex> &vertices, std::vector<Face> &faces, const std::vector<size_t> &ring);

}

#endif
