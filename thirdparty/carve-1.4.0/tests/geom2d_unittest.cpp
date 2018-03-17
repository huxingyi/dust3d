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

#include <gtest/gtest.h>

#if defined(HAVE_CONFIG_H)
#  include <carve_config.h>
#endif

#include <carve/carve.hpp>
#include <carve/geom.hpp>
#include <carve/geom2d.hpp>
#include <carve/geom3d.hpp>
#include <carve/matrix.hpp>

using namespace carve::geom;
using namespace carve::geom2d;

typedef std::vector<P2> P2vec;

P2vec TRI(P2 a, P2 b, P2 c) { P2vec r; r.reserve(3); r.push_back(a); r.push_back(b); r.push_back(c); return r; }

TEST(GeomTest, Geom2D) {
  ASSERT_EQ(0.0, dot(VECTOR(1,1), VECTOR(-2,2)));
  ASSERT_EQ(1.0, dot(VECTOR(1,1), VECTOR(0,1)));

  ASSERT_EQ(-2.0, cross(VECTOR(1,2), VECTOR(3,4)));

  ASSERT_TRUE(internalToAngle(VECTOR(1,1), VECTOR(0,0), VECTOR(-1,1), VECTOR(2,3)));
  ASSERT_FALSE(internalToAngle(VECTOR(-1,1), VECTOR(0,0), VECTOR(1,1), VECTOR(2,3)));

  // rounding errors mean that N=100 fails with inexact orient2d.
#define N 100
  for (size_t i = 0; i < N; ++i) {
    for (size_t j = 0; j < N; ++j) {
      double x, y;
      x = i / double(N/2) - 1.0;
      y = j / double(N/2) - 1.0;
      ASSERT_NE(
          internalToAngle(VECTOR(+1,1), VECTOR(0,0), VECTOR(-1,1), VECTOR(x, y)),
          internalToAngle(VECTOR(-1,1), VECTOR(0,0), VECTOR(+1,1), VECTOR(x, y)));
      if (x != 0 && y != 0) {
        double a = atan2(y, x);
        const double a_min = M_PI / 4;
        const double a_max = M_PI * 3 / 4;
        ASSERT_EQ(y > fabs(x), internalToAngle(VECTOR(+1,1), VECTOR(0,0), VECTOR(-1,1), VECTOR(x, y)));
        ASSERT_EQ(y <= fabs(x), internalToAngle(VECTOR(-1,1), VECTOR(0,0), VECTOR(+1,1), VECTOR(x, y)));
      }
    }
  }

  P2vec tri;

  // anticlockwise triangle
  tri = TRI(VECTOR(0,0), VECTOR(0,1), VECTOR(1,0));

  ASSERT_EQ(pointIntersectsTriangle(VECTOR(.25, .25), tri), true);
  ASSERT_EQ(pointIntersectsTriangle(VECTOR(0, 0), tri), true);
  ASSERT_EQ(pointIntersectsTriangle(VECTOR(0.5, 0.5), tri), true);
  ASSERT_EQ(pointIntersectsTriangle(VECTOR(.75, .75), tri), false);
  ASSERT_EQ(pointIntersectsTriangle(VECTOR(-.25, .25), tri), false);
  ASSERT_EQ(pointIntersectsTriangle(VECTOR(.25, -.25), tri), false);

  ASSERT_EQ(lineIntersectsTriangle(VECTOR(1,1), VECTOR(2,2), tri), false);

  ASSERT_EQ(lineIntersectsTriangle(VECTOR(-1,-1), VECTOR(3,-1), tri), false);
  ASSERT_EQ(lineIntersectsTriangle(VECTOR(-1,-1), VECTOR(-1,3), tri), false);
  ASSERT_EQ(lineIntersectsTriangle(VECTOR(3,-1), VECTOR(-1,3), tri), false);

  ASSERT_EQ(lineIntersectsTriangle(VECTOR(.25,.25), VECTOR(.5,.5), tri), true);
  ASSERT_EQ(lineIntersectsTriangle(VECTOR(0,0), VECTOR(-.25,-.25), tri), true);
  ASSERT_EQ(lineIntersectsTriangle(VECTOR(0,0), VECTOR(-.25,-.25), tri), true);
  ASSERT_EQ(lineIntersectsTriangle(VECTOR(.5,0), VECTOR(0,.5), tri), true);
  ASSERT_EQ(lineIntersectsTriangle(VECTOR(1.5,-1), VECTOR(-1,1.5), tri), true);

  ASSERT_EQ(lineIntersectsTriangle(VECTOR(2,-1), VECTOR(-1,2), tri), true);
  ASSERT_EQ(lineIntersectsTriangle(VECTOR(-1,0), VECTOR(2,0), tri), true);
  ASSERT_EQ(lineIntersectsTriangle(VECTOR(0,-1), VECTOR(0,2), tri), true);

  ASSERT_EQ(lineIntersectsTriangle(VECTOR(0.25,0.25), VECTOR(1,1), tri), true);
  ASSERT_EQ(lineIntersectsTriangle(VECTOR(0.5,0.5), VECTOR(1,1), tri), true);

  ASSERT_EQ(triangleIntersectsTriangle(TRI(VECTOR(1,0), VECTOR(1,1), VECTOR(0,1)), tri), true);
  ASSERT_EQ(triangleIntersectsTriangle(TRI(VECTOR(.5,.5), VECTOR(.5,1), VECTOR(1,.5)), tri), true);
  ASSERT_EQ(triangleIntersectsTriangle(TRI(VECTOR(.25,.25), VECTOR(.25,1), VECTOR(1,.25)), tri), true);
  ASSERT_EQ(triangleIntersectsTriangle(TRI(VECTOR(2,0), VECTOR(2,2), VECTOR(0,2)), tri), false);

  // clockwise triangle - results should be the same.
  tri = TRI(VECTOR(0,0), VECTOR(0,1), VECTOR(1,0));

  ASSERT_EQ(pointIntersectsTriangle(VECTOR(.25, .25), tri), true);
  ASSERT_EQ(pointIntersectsTriangle(VECTOR(0, 0), tri), true);
  ASSERT_EQ(pointIntersectsTriangle(VECTOR(0.5, 0.5), tri), true);
  ASSERT_EQ(pointIntersectsTriangle(VECTOR(.75, .75), tri), false);
  ASSERT_EQ(pointIntersectsTriangle(VECTOR(-.25, .25), tri), false);
  ASSERT_EQ(pointIntersectsTriangle(VECTOR(.25, -.25), tri), false);

  ASSERT_EQ(lineIntersectsTriangle(VECTOR(1,1), VECTOR(2,2), tri), false);

  ASSERT_EQ(lineIntersectsTriangle(VECTOR(-1,-1), VECTOR(3,-1), tri), false);
  ASSERT_EQ(lineIntersectsTriangle(VECTOR(-1,-1), VECTOR(-1,3), tri), false);
  ASSERT_EQ(lineIntersectsTriangle(VECTOR(3,-1), VECTOR(-1,3), tri), false);

  ASSERT_EQ(lineIntersectsTriangle(VECTOR(.25,.25), VECTOR(.5,.5), tri), true);
  ASSERT_EQ(lineIntersectsTriangle(VECTOR(0,0), VECTOR(-.25,-.25), tri), true);
  ASSERT_EQ(lineIntersectsTriangle(VECTOR(0,0), VECTOR(-.25,-.25), tri), true);
  ASSERT_EQ(lineIntersectsTriangle(VECTOR(.5,0), VECTOR(0,.5), tri), true);
  ASSERT_EQ(lineIntersectsTriangle(VECTOR(1.5,-1), VECTOR(-1,1.5), tri), true);

  ASSERT_EQ(lineIntersectsTriangle(VECTOR(2,-1), VECTOR(-1,2), tri), true);
  ASSERT_EQ(lineIntersectsTriangle(VECTOR(-1,0), VECTOR(2,0), tri), true);
  ASSERT_EQ(lineIntersectsTriangle(VECTOR(0,-1), VECTOR(0,2), tri), true);

  ASSERT_EQ(lineIntersectsTriangle(VECTOR(0.25,0.25), VECTOR(1,1), tri), true);
  ASSERT_EQ(lineIntersectsTriangle(VECTOR(0.5,0.5), VECTOR(1,1), tri), true);

  ASSERT_EQ(triangleIntersectsTriangle(TRI(VECTOR(1,0), VECTOR(1,1), VECTOR(0,1)), tri), true);
  ASSERT_EQ(triangleIntersectsTriangle(TRI(VECTOR(.5,.5), VECTOR(.5,1), VECTOR(1,.5)), tri), true);
  ASSERT_EQ(triangleIntersectsTriangle(TRI(VECTOR(.25,.25), VECTOR(.25,1), VECTOR(1,.25)), tri), true);
  ASSERT_EQ(triangleIntersectsTriangle(TRI(VECTOR(2,0), VECTOR(2,2), VECTOR(0,2)), tri), false);

  ASSERT_EQ(lineIntersectsTriangle(
        VECTOR(1119.40699999999992542143,213543.176000000006752089),
        VECTOR(1118.40699999999992542143,213542.883000000001629815),
        TRI(VECTOR(1121.40699999999992542143,213543.761999999987892807),
            VECTOR(1119.40699999999992542143,213544.662000000011175871),
            VECTOR(1120.40699999999992542143,213543.469000000011874363))), false);
}
