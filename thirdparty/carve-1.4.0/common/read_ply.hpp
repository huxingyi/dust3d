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

#include <istream>
#include <fstream>

#include <carve/input.hpp>



bool readPLY(std::istream &in, carve::input::Input &inputs, const carve::math::Matrix &transform = carve::math::Matrix::IDENT());
bool readPLY(const std::string &in_file, carve::input::Input &inputs, const carve::math::Matrix &transform = carve::math::Matrix::IDENT());

carve::poly::Polyhedron *readPLY(std::istream &in, const carve::math::Matrix &transform = carve::math::Matrix::IDENT());
carve::poly::Polyhedron *readPLY(const std::string &in_file, const carve::math::Matrix &transform = carve::math::Matrix::IDENT());



bool readOBJ(std::istream &in, carve::input::Input &inputs, const carve::math::Matrix &transform = carve::math::Matrix::IDENT());
bool readOBJ(const std::string &in_file, carve::input::Input &inputs, const carve::math::Matrix &transform = carve::math::Matrix::IDENT());

carve::poly::Polyhedron *readOBJ(std::istream &in, const carve::math::Matrix &transform = carve::math::Matrix::IDENT());
carve::poly::Polyhedron *readOBJ(const std::string &in_file, const carve::math::Matrix &transform = carve::math::Matrix::IDENT());



bool readVTK(std::istream &in, carve::input::Input &inputs, const carve::math::Matrix &transform = carve::math::Matrix::IDENT());
bool readVTK(const std::string &in_file, carve::input::Input &inputs, const carve::math::Matrix &transform = carve::math::Matrix::IDENT());

carve::poly::Polyhedron *readVTK(std::istream &in, const carve::math::Matrix &transform = carve::math::Matrix::IDENT());
carve::poly::Polyhedron *readVTK(const std::string &in_file, const carve::math::Matrix &transform = carve::math::Matrix::IDENT());



