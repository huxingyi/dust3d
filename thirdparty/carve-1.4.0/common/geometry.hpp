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

#include <carve/poly.hpp>
#include <carve/matrix.hpp>

#ifdef WIN32
#undef rad1
#undef rad2
#endif

carve::poly::Polyhedron *makeCube(const carve::math::Matrix &transform = carve::math::Matrix());
carve::poly::Polyhedron *makeSubdividedCube(int sub_x = 3, int sub_y = 3, int sub_z = 3,
                                            bool (*inc)(int, int, int) = NULL, const carve::math::Matrix &transform = carve::math::Matrix());
carve::poly::Polyhedron *makeDoubleCube(const carve::math::Matrix &transform = carve::math::Matrix());
carve::poly::Polyhedron *makeTorus(int slices, 
                                   int rings, 
                                   double rad1, 
                                   double rad2, 
                                   const carve::math::Matrix &transform = carve::math::Matrix());
carve::poly::Polyhedron *makeCylinder(int slices, 
                                      double rad, 
                                      double height, 
                                      const carve::math::Matrix &transform = carve::math::Matrix());
carve::poly::Polyhedron *makeCone(int slices, 
                                  double rad, 
                                  double height, 
                                  const carve::math::Matrix &transform = carve::math::Matrix());
