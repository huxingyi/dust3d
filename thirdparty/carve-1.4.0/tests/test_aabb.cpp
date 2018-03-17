// Begin License:
// Copyright (C) 2006-2008 Tobias Sargeant (tobias.sargeant@gmail.com).
// All rights reserved.
//
// This file is part of the Carve CSG Library (http://carve-csg.com/)
//
// This file may be used under the terms of the GNU General Public
// License version 2.0 as published by the Free Software Foundation
// and appearing in the file LICENSE.GPL2 included in the packaging of
// this file.
//
// This file is provided "AS IS" with NO WARRANTY OF ANY KIND,
// INCLUDING THE WARRANTIES OF DESIGN, MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE.
// End:


#if defined(HAVE_CONFIG_H)
#  include <carve_config.h>
#endif

#include <carve/aabb.hpp>
#include <carve/geom3d.hpp>

#include <iostream>
#include <fstream>
#include <string>
#include <cstring>

int main(int argc, char **argv) {
  carve::geom3d::AABB aabb(carve::geom::VECTOR(0,0,0), carve::geom::VECTOR(1,1,1));

  std::ifstream in("aabb.test");
  while (in.good()) {
    double x1, y1, z1;
    double x2, y2, z2;
    char ray_intersects[10];
    char lineseg_intersects[10];
    bool ri;
    bool li;
    std::string line;

    std::getline(in, line);
    sscanf(line.c_str(),
           "<%lf,%lf,%lf>\t<%lf,%lf,%lf>\t%s\t%s",
           &x1, &y1, &z1,
           &x2, &y2, &z2,
           ray_intersects, lineseg_intersects);

    carve::geom3d::Vector v1 = carve::geom::VECTOR(x1, y1, z1);
    carve::geom3d::Vector v2 = carve::geom::VECTOR(x2, y2, z2);

    carve::geom3d::Ray r(v2 - v1, v1);
    carve::geom3d::LineSegment l(v1, v2);

    ri = !std::strcmp(ray_intersects, "True");
    li = !std::strcmp(lineseg_intersects, "True");

    bool ri_t = aabb.intersects(r);
    bool li_t = aabb.intersectsLineSegment(l.v1, l.v2);

    if (li != li_t || ri != ri_t) std::cout << line << std::endl;

    if (ri != ri_t) {
      std::cout << "RAY: " << ri << " " << ri_t << std::endl;
    }
    if (li != li_t) {
      std::cout << "LINE: " << li << " " << li_t << std::endl;
      std::cout << "LINE MIDPOINT = " << l.midpoint.asStr() << std::endl;
      aabb.intersectsLineSegment(l.v1, l.v2);
    }
    if (li != li_t || ri != ri_t) std::cout << std::endl;
  }
}
