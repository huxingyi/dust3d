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

#include <carve/vector.hpp>
#include <iostream>

double triangularPrismVolume(const carve::geom3d::Vector &a,
                             const carve::geom3d::Vector &b,
                             const carve::geom3d::Vector &c,
                             double z) {
  double v1 = carve::geom3d::tetrahedronVolume(carve::geom::VECTOR(a.x, a.y, z),
                                               carve::geom::VECTOR(c.x, c.y, z),
                                               carve::geom::VECTOR(b.x, b.y, z),
                                               a);
  double v2 = carve::geom3d::tetrahedronVolume(carve::geom::VECTOR(a.x, a.y, z),
                                               carve::geom::VECTOR(c.x, c.y, z),
                                               b,
                                               a);
  double v3 = carve::geom3d::tetrahedronVolume(carve::geom::VECTOR(c.x, c.y, z),
                                               c,
                                               b,
                                               a);

  std::cerr << "[components:" << v1 << "," << v2 << "," << v3 << "]" << std::endl;
  return v1 + v2 + v3;
}
                             
int main(int argc, char **argv) {
  std::cerr << "result: "
            << triangularPrismVolume(carve::geom::VECTOR(1,0,1),
                                     carve::geom::VECTOR(0,1,1),
                                     carve::geom::VECTOR(0,0,3),
                                     0)
            << std::endl;
  std::cerr << "result: "
            << triangularPrismVolume(carve::geom::VECTOR(11,10,1),
                                     carve::geom::VECTOR(10,11,1),
                                     carve::geom::VECTOR(10,10,3),
                                     0)
            << std::endl;
}
