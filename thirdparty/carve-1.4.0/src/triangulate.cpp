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

#include <carve/csg.hpp>
#include <carve/tree.hpp>
#include <carve/csg_triangulator.hpp>

#include "read_ply.hpp"
#include "write_ply.hpp"

#include "opts.hpp"

#include <fstream>
#include <algorithm>
#include <string>
#include <utility>
#include <set>
#include <iostream>
#include <iomanip>

#include <time.h>
typedef std::vector<std::string>::iterator TOK;



struct Options : public opt::Parser {
  bool improve;
  bool ascii;
  bool obj;
  bool vtk;
  bool canonicalize;

  std::string file;
  
  virtual void optval(const std::string &o, const std::string &v) {
    if (o == "--canonicalize" || o == "-c") { canonicalize = true; return; }
    if (o == "--binary"       || o == "-b") { ascii = false; return; }
    if (o == "--obj"          || o == "-O") { obj = true; return; }
    if (o == "--vtk"          || o == "-V") { vtk = true; return; }
    if (o == "--ascii"        || o == "-a") { ascii = true; return; }
    if (o == "--help"         || o == "-h") { help(std::cout); exit(0); }
    if (o == "--improve"      || o == "-i") { improve = true; return; }
  }

  virtual std::string usageStr() {
    return std::string ("Usage: ") + progname + std::string(" [options] expression");
  };

  virtual void arg(const std::string &a) {
    file = a;
  }

  virtual void help(std::ostream &out) {
    this->opt::Parser::help(out);
  }

  Options() {
    improve = false;
    ascii = true;
    obj = false;
    vtk = false;
    canonicalize = false;
    file = "";

    option("canonicalize", 'c', false, "Canonicalize before output (for comparing output).");
    option("binary",       'b', false, "Produce binary output.");
    option("ascii",        'a', false, "ASCII output (default).");
    option("obj",          'O', false, "Output in .obj format.");
    option("vtk",          'V', false, "Output in .vtk format.");
    option("improve",      'i', false, "Improve triangulation by minimising internal edge lengths.");
    option("help",         'h', false, "This help message.");
  }
};



static Options options;




static bool endswith(const std::string &a, const std::string &b) {
  if (a.size() < b.size()) return false;

  for (unsigned i = a.size(), j = b.size(); j; ) {
    if (tolower(a[--i]) != tolower(b[--j])) return false;
  }
  return true;
}

carve::poly::Polyhedron *readModel(const std::string &file) {
  carve::poly::Polyhedron *poly;

  if (file == "") {
    if (options.obj) {
      poly = readOBJ(std::cin);
    } else if (options.vtk) {
      poly = readVTK(std::cin);
    } else {
      poly = readPLY(std::cin);
    }
  } else if (endswith(file, ".ply")) {
    poly = readPLY(file);
  } else if (endswith(file, ".vtk")) {
    poly = readVTK(file);
  } else if (endswith(file, ".obj")) {
    poly = readOBJ(file);
  }

  if (poly == NULL) return NULL;

  std::cerr << "loaded polyhedron " << poly << " has "
    << poly->vertices.size() << " vertices "
    << poly->faces.size() << " faces "
    << poly->manifold_is_closed.size() << " manifolds (" << std::count(poly->manifold_is_closed.begin(), poly->manifold_is_closed.end(), true) << " closed)" << std::endl;

  return poly;
}

int main(int argc, char **argv) {
  options.parse(argc, argv);

  carve::poly::Polyhedron *poly = readModel(options.file);
  if (!poly) exit(1);

  std::vector<carve::poly::Vertex<3> > out_vertices = poly->vertices;
  std::vector<carve::poly::Face<3> > out_faces;

  size_t N = 0;
  for (size_t i = 0; i < poly->faces.size(); ++i) {
    carve::poly::Face<3> &f = poly->faces[i];
    N += f.nVertices() - 2;
  }
  out_faces.reserve(N);

  for (size_t i = 0; i < poly->faces.size(); ++i) {
    carve::poly::Face<3> &f = poly->faces[i];
    std::vector<carve::triangulate::tri_idx> result;

    std::vector<const carve::poly::Polyhedron::vertex_t *> vloop;
    f.getVertexLoop(vloop);

    carve::triangulate::triangulate(carve::poly::p2_adapt_project<3>(f.project), vloop, result);
    if (options.improve) {
      carve::triangulate::improve(carve::poly::p2_adapt_project<3>(f.project), vloop, result);
    }

    for (size_t j = 0; j < result.size(); ++j) {
      out_faces.push_back(carve::poly::Face<3>(
            &out_vertices[poly->vertexToIndex_fast(vloop[result[j].a])],
            &out_vertices[poly->vertexToIndex_fast(vloop[result[j].b])],
            &out_vertices[poly->vertexToIndex_fast(vloop[result[j].c])]
            ));
    }
  }

  carve::poly::Polyhedron *result = new carve::poly::Polyhedron(out_faces, out_vertices);

  if (options.canonicalize) result->canonicalize();

  if (options.obj) {
    writeOBJ(std::cout, result);
  } else if (options.vtk) {
    writeVTK(std::cout, result);
  } else {
    writePLY(std::cout, result, options.ascii);
  }

  delete result;
  delete poly;
}
