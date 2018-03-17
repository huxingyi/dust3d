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

#include <carve/vector.hpp>
#include <carve/poly.hpp>

#include <carve/octree_decl.hpp>
#include <carve/octree_impl.hpp>

#include "rgb.hpp"

void drawPolyhedronWireframe(carve::poly::Polyhedron *poly, bool normal = true, int group = -1);
void drawPolyhedron(carve::poly::Polyhedron *poly, float r, float g, float b, float a, bool offset = false, int group = -1);

void drawFace(carve::poly::Face<3> *face, cRGBA fc, bool offset);
void drawColourFace(carve::poly::Face<3> *face, const std::vector<cRGBA> &vc, bool offset);

void installDebugHooks();
void drawCube(const carve::geom3d::Vector &, const carve::geom3d::Vector &);
void drawOctree(const carve::csg::Octree &);

extern carve::geom3d::Vector g_translation;
extern double g_scale;
