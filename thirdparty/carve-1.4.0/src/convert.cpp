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

#include "geometry.hpp"
#include "glu_triangulator.hpp"

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



struct Options : public opt::Parser {
  bool ascii;
  bool obj;
  bool vtk;
  bool canonicalize;
  bool triangulate;

  std::string file;
  
  virtual void optval(const std::string &o, const std::string &v) {
    if (o == "--canonicalize" || o == "-c") { canonicalize = true; return; }
    if (o == "--binary"       || o == "-b") { ascii = false; return; }
    if (o == "--obj"          || o == "-O") { obj = true; return; }
    if (o == "--vtk"          || o == "-V") { vtk = true; return; }
    if (o == "--ascii"        || o == "-a") { ascii = true; return; }
    if (o == "--triangulate"  || o == "-t") { triangulate = true; return; }
    if (o == "--help"         || o == "-h") { help(std::cout); exit(0); }
  }

  virtual std::string usageStr() {
    return std::string ("Usage: ") + progname + std::string(" [options] file");
  };

  virtual void arg(const std::string &a) {
    file = a;
  }

  virtual void help(std::ostream &out) {
    this->opt::Parser::help(out);
  }

  Options() {
    ascii = true;
    obj = false;
    vtk = false;
    triangulate = false;

    option("canonicalize", 'c', false, "Canonicalize before output (for comparing output).");
    option("binary",       'b', false, "Produce binary output.");
    option("ascii",        'a', false, "ASCII output (default).");
    option("obj",          'O', false, "Output in .obj format.");
    option("vtk",          'V', false, "Output in .vtk format.");
    option("triangulate",  't', false, "Triangulate output.");
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

int main(int argc, char **argv) {
  options.parse(argc, argv);

  carve::input::Input inputs;
  std::vector<carve::poly::Polyhedron *> polys;
  std::vector<carve::line::PolylineSet *> lines;
  std::vector<carve::point::PointSet *> points;

  if (options.file == "") {
    readPLY(std::cin, inputs);
  } else {
    if (endswith(options.file, ".ply")) {
      readPLY(options.file, inputs);
    } else if (endswith(options.file, ".vtk")) {
      readVTK(options.file, inputs);
    } else if (endswith(options.file, ".obj")) {
      readOBJ(options.file, inputs);
    }
  }

  for (std::list<carve::input::Data *>::const_iterator i = inputs.input.begin(); i != inputs.input.end(); ++i) {
    carve::poly::Polyhedron *p;
    carve::point::PointSet *ps;
    carve::line::PolylineSet *l;

    if ((p = carve::input::Input::create<carve::poly::Polyhedron>(*i)) != NULL)  {
      if (options.canonicalize) p->canonicalize();
      if (options.obj) {
        writeOBJ(std::cout, p);
      } else if (options.vtk) {
        writeVTK(std::cout, p);
      } else {
        writePLY(std::cout, p, options.ascii);
      }
      delete p;
    } else if ((l = carve::input::Input::create<carve::line::PolylineSet>(*i)) != NULL)  {
      if (options.obj) {
        writeOBJ(std::cout, l);
      } else if (options.vtk) {
        writeVTK(std::cout, l);
      } else {
        writePLY(std::cout, l, options.ascii);
      }
      delete l;
    } else if ((ps = carve::input::Input::create<carve::point::PointSet>(*i)) != NULL)  {
      if (options.obj) {
        std::cerr << "Can't write a point set in .obj format" << std::endl;
      } else if (options.vtk) {
        std::cerr << "Can't write a point set in .vtk format" << std::endl;
      } else {
        writePLY(std::cout, ps, options.ascii);
      }
      delete ps;
    }
  }

  return 0;
}
