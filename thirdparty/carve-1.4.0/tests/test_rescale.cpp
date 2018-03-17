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

#include <carve/rescale.hpp>

#include "mersenne_twister.h"

int main(int argc, char **argv) {
  MTRand rand;
  double sx, sy, sz;
  double minx, maxx, miny,maxy, minz, maxz;

  sx = rand.rand(1e5) + 1;
  sy = rand.rand(1e5) + 1;
  sz = rand.rand(1e5) + 1;

  minx = rand.rand(1e10) - sx / 2; maxx = minx + sx;
  miny = rand.rand(1e10) - sy / 2; maxy = miny + sy;
  minz = rand.rand(1e10) - sz / 2; maxz = minz + sz;

  if (minx > maxx) std::swap(minx, maxx);
  if (miny > maxy) std::swap(miny, maxy);
  if (minz > maxz) std::swap(minz, maxz);

  carve::rescale::rescale r(minx, miny, minz, maxx, maxy, maxz);
  carve::rescale::fwd fwd(r);
  carve::rescale::rev rev(r);

  std::cout << "x: [" << minx << "," << maxx << "]" << std::endl;
  std::cout << "y: [" << miny << "," << maxy << "]" << std::endl;
  std::cout << "z: [" << minz << "," << maxz << "]" << std::endl;
  std::cout << std::endl;
  std::cout << "r.dx=" << r.dx << " r.dy=" << r.dy << " r.dz=" << r.dz << " r.scale=" << r.scale << std::endl;
  std::cout << std::endl;

  for (int i = 0; i < 10000; i++) {
    carve::geom3d::Vector in, temp, out;
    in.x = rand.rand(maxx-minx) + minx;
    in.y = rand.rand(maxy-miny) + miny;
    in.z = rand.rand(maxz-minz) + minz;
    temp = fwd(in);
    out = rev(temp);
    std::cout << in << " -> " << temp << " -> " << out << std::endl;
    CARVE_ASSERT(fabs(temp.x) < 1.0 && fabs(temp.y) < 1.0 && fabs(temp.z) < 1.0);
    CARVE_ASSERT(out.x == in.x && out.y == in.y && out.z == in.z);
  }
}
