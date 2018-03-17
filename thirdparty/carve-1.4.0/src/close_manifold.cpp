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
#include <carve/csg_triangulator.hpp>
#include <carve/poly.hpp>
#include <carve/geom3d.hpp>

#include "opts.hpp"
#include "read_ply.hpp"
#include "write_ply.hpp"


#include <fstream>
#include <algorithm>
#include <string>
#include <utility>
#include <set>
#include <iostream>
#include <iomanip>

struct Options : public opt::Parser {
  bool ascii;
  bool obj;
  bool vtk;
  bool flip;

  double pos;
  enum { ERR = -1, X = 0, Y = 1, Z = 2 } axis;

  std::string file;

  virtual void optval(const std::string &o, const std::string &v) {
    if (o == "--binary"       || o == "-b") { ascii = false; return; }
    if (o == "--obj"          || o == "-O") { obj = true; return; }
    if (o == "--vtk"          || o == "-V") { vtk = true; return; }
    if (o == "--ascii"        || o == "-a") { ascii = true; return; }
    if (o == "--flip"         || o == "-f") { flip = true; return; }
    if (                         o == "-x") { axis = X; pos = strtod(v.c_str(), NULL); }
    if (                         o == "-y") { axis = Y; pos = strtod(v.c_str(), NULL); }
    if (                         o == "-z") { axis = Z; pos = strtod(v.c_str(), NULL); }
  }

  virtual std::string usageStr() {
    return std::string ("Usage: ") + progname + std::string(" [options] expression");
  };

  virtual void arg(const std::string &a) {
    if (file == "") {
      file = a;
    }
  }

  virtual void help(std::ostream &out) {
    this->opt::Parser::help(out);
  }

  Options() {
    ascii = true;
    obj = false;
    vtk = false;
    flip = false;
    pos = 0.0;
    axis = ERR;
    file = "";

    option("binary",       'b', false, "Produce binary output.");
    option("ascii",        'a', false, "ASCII output (default).");
    option("obj",          'O', false, "Output in .obj format.");
    option("vtk",          'V', false, "Output in .vtk format.");
    option("flip",         'f', false, "Flip orientation of input faces.");
    option(                'x', true,  "close with plane x={arg}.");
    option(                'y', true,  "close with plane y={arg}.");
    option(                'z', true,  "close with plane z={arg}.");
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

int main(int argc, char **argv) {
  options.parse(argc, argv);
  carve::poly::Polyhedron *poly;

  if (options.axis == Options::ERR) {
    std::cerr << "need to specify a closure plane." << std::endl;
    exit(1);
  }

  if (options.file == "-") {
    poly = readPLY(std::cin);
  } else if (endswith(options.file, ".ply")) {
    poly = readPLY(options.file);
  } else if (endswith(options.file, ".vtk")) {
    poly = readVTK(options.file);
  } else if (endswith(options.file, ".obj")) {
    poly = readOBJ(options.file);
  }

  if (poly == NULL) {
    std::cerr << "failed to load polyhedron" << std::endl;
    exit(1);
  }

  std::cerr << "poly aabb = " << poly->aabb << std::endl;

  if (poly->aabb.compareAxis(carve::geom::axis_pos(options.axis, options.pos)) == 0) {
    std::cerr << "poly aabb intersects closure plane." << std::endl;
    exit(1);
  }

  std::vector<size_t> open_edges;
  for (size_t edge_num = 0; edge_num < poly->edges.size(); ++edge_num) {
    std::vector<const carve::poly::Polyhedron::face_t *> &ef = poly->connectivity.edge_to_face[edge_num];
    if (std::find(ef.begin(), ef.end(), (const carve::poly::Polyhedron::face_t *)NULL) != ef.end()) {
      open_edges.push_back(edge_num);
    }
  }

  std::vector<carve::poly::Polyhedron::face_t> out_faces;
  out_faces.reserve(open_edges.size() + 1 + poly->faces.size());
  for (size_t face = 0; face < poly->faces.size(); ++face) {
    carve::poly::Polyhedron::face_t *temp = poly->faces[face].clone(options.flip);
    out_faces.push_back(*temp);
    delete temp;
  }

  std::vector<carve::poly::Polyhedron::vertex_t> proj_vertices;
  proj_vertices.reserve(poly->vertices.size());
  std::copy(poly->vertices.begin(), poly->vertices.end(), std::back_inserter(proj_vertices));

  for (size_t vert = 0; vert != proj_vertices.size(); ++vert) {
    proj_vertices[vert].v.v[options.axis] = options.pos;
  }

  std::map<const carve::poly::Polyhedron::vertex_t *, const carve::poly::Polyhedron::vertex_t *> base_edges;


  for (size_t edge_index = 0; edge_index < open_edges.size(); ++edge_index) {
    size_t edge_num = open_edges[edge_index];
    carve::poly::Polyhedron::edge_t *edge = &poly->edges[edge_num];
    size_t v1i = poly->vertexToIndex_fast(edge->v1);
    size_t v2i = poly->vertexToIndex_fast(edge->v2);
    std::vector<const carve::poly::Polyhedron::vertex_t *> vertices;
    vertices.push_back(edge->v2);
    vertices.push_back(edge->v1);
    vertices.push_back(&proj_vertices[v1i]);
    vertices.push_back(&proj_vertices[v2i]);

    std::vector<const carve::poly::Polyhedron::face_t *> &ef = poly->connectivity.edge_to_face[edge_num];
    CARVE_ASSERT(ef.size() == 2);
    if (ef[0] == NULL) { std::swap(vertices[0], vertices[1]); std::swap(vertices[2], vertices[3]); }
    if (options.flip) { std::swap(vertices[0], vertices[1]); std::swap(vertices[2], vertices[3]); }
    out_faces.push_back(carve::poly::Polyhedron::face_t(vertices));
    base_edges[vertices[3]] = vertices[2];
  }

  while (base_edges.size()) {
    std::vector<const carve::poly::Polyhedron::vertex_t *> fv;
    const carve::poly::Polyhedron::vertex_t *vert = base_edges.begin()->first;
    const carve::poly::Polyhedron::vertex_t *start = vert;
    do {
      const carve::poly::Polyhedron::vertex_t *next = base_edges[vert];
      base_edges.erase(vert);
      fv.push_back(next);
      vert = next;
    } while (vert != start);
    out_faces.push_back(carve::poly::Polyhedron::face_t(fv));
  }

  std::vector<carve::poly::Polyhedron::vertex_t> vertices;
  carve::csg::VVMap vmap;
  carve::poly::Polyhedron::collectFaceVertices(out_faces, vertices, vmap);
  carve::poly::Polyhedron *result = new carve::poly::Polyhedron(out_faces, vertices);
  std::cerr << "result: " << result->faces.size() << " faces\n";

  if (options.obj) {
    writeOBJ(std::cout, result);
  } else if (options.vtk) {
    writeVTK(std::cout, result);
  } else {
    writePLY(std::cout, result, options.ascii);
  }

  return 0;
}
