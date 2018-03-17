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

#include <carve/matrix.hpp>
#include <carve/math.hpp>

#include <iostream>

#define D(x) strtod(x, NULL)

int main(int argc, char **argv) {
  carve::math::Matrix3 m;
  m._11 = D(argv[1]); m._12 = D(argv[2]); m._13 = D(argv[3]);
  m._21 = D(argv[2]); m._22 = D(argv[4]); m._23 = D(argv[5]);
  m._31 = D(argv[3]); m._32 = D(argv[5]); m._33 = D(argv[6]);

  double l1, l2, l3;
  carve::geom3d::Vector e1, e2, e3;

  carve::math::eigSolveSymmetric(m, l1, e1, l2, e2, l3, e3);
  std::cout << l1 << " " << e1 << std::endl;
  std::cout << l2 << " " << e2 << std::endl;
  std::cout << l3 << " " << e3 << std::endl;

  std::cout << m * e1 - l1 * e1 << "  " << (m * e1 - l1 * e1).isZero() << std::endl;
  std::cout << m * e2 - l2 * e2 << "  " << (m * e2 - l2 * e2).isZero() << std::endl;
  std::cout << m * e3 - l3 * e3 << "  " << (m * e3 - l3 * e3).isZero() << std::endl;

  eigSolve(m, l1, l2, l3);

  std::cout << l1 << " " << e1 << std::endl;
  std::cout << l2 << " " << e2 << std::endl;
  std::cout << l3 << " " << e3 << std::endl;

}
