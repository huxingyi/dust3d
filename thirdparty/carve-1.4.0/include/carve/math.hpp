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


#pragma once

#include <carve/carve.hpp>

#include <carve/geom3d.hpp>
#include <carve/math_constants.hpp>

#include <math.h>

namespace carve {
  namespace math {
    struct Matrix3;
    using carve::geom3d::Vector;

    int cubic_roots(double c3, double c2, double c1, double c0, double *roots);

    void eigSolveSymmetric(const Matrix3 &m,
                           double &l1, Vector &e1,
                           double &l2, Vector &e2,
                           double &l3, Vector &e3);

    void eigSolve(const Matrix3 &m, double &l1, double &l2, double &l3);

    static inline bool ZERO(double x) { return fabs(x) < carve::EPSILON; }

    static inline double radians(double deg) { return deg * M_PI / 180.0; }
    static inline double degrees(double rad) { return rad * 180.0 / M_PI; }

    static inline double ANG(double x) {
      return (x < 0) ? x + M_TWOPI : x;
    }

  }
}
