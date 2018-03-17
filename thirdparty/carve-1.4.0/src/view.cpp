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

#include <carve/carve.hpp>
#include <carve/poly.hpp>
#include <carve/polyline.hpp>
#include <carve/pointset.hpp>

#include "geom_draw.hpp"
#include "read_ply.hpp"
#include "scene.hpp"
#include "opts.hpp"

#include <gloop/gloopgl.hpp>
#include <gloop/gloopglu.hpp>
#include <gloop/gloopglut.hpp>

#include <fstream>
#include <string>
#include <utility>
#include <set>
#include <algorithm>

#include <time.h>

struct Options : public opt::Parser {
  bool wireframe;
  bool normal;
  bool fit;
  bool obj;
  bool vtk;
  std::vector<std::string> files;

  virtual void optval(const std::string &o, const std::string &v) {
    if (o == "--obj"          || o == "-O") { obj = true; return; }
    if (o == "--vtk"          || o == "-V") { vtk = true; return; }
    if (o == "--no-wireframe" || o == "-n") { wireframe = false; return; }
    if (o == "--no-fit"       || o == "-f") { fit = false; return; }
    if (o == "--no-normals"   || o == "-N") { normal = false; return; }
    if (o == "--help"         || o == "-h") { help(std::cout); exit(0); }
  }

  virtual void arg(const std::string &a) {
    files.push_back(a);
  }

  Options() {
    obj = false;
    vtk = false;
    fit = true;
    wireframe = true;
    normal = true;

    option("obj",          'O', false, "Read input in .obj format.");
    option("vtk",          'V', false, "Read input in .vtk format.");
    option("no-wireframe", 'n', false, "Don't display wireframes.");
    option("no-fit",       'f', false, "Don't scale/translate for viewing.");
    option("no-normals",   'N', false, "Don't display normals.");
    option("help",         'h', false, "This help message.");
  }
};



static Options options;



bool odd(int x, int y, int z) {
  return ((x + y + z) & 1) == 1;
}

bool even(int x, int y, int z) {
  return ((x + y + z) & 1) == 0;
}

#undef min
#undef max

GLuint genSceneDisplayList(std::vector<carve::poly::Polyhedron *> &polys,
                           std::vector<carve::line::PolylineSet *> &lines,
                           std::vector<carve::point::PointSet *> &points,
                           size_t *listSize,
                           std::vector<bool> &is_wireframe) {

  int n = 0;
  int N = 1;

  is_wireframe.clear();

  if (options.wireframe) N = 2;

  for (size_t p = 0; p < polys.size(); ++p) n += polys[p]->manifold_is_closed.size() * N;
  for (size_t p = 0; p < lines.size(); ++p) n += lines[p]->lines.size() * 2;
  n += points.size();

  if (n == 0) return 0;

  carve::geom3d::AABB aabb;
  if (polys.size()) {
    aabb = polys[0]->aabb;
  } else if (lines.size()) {
    aabb = lines[0]->aabb;
  } else if (points.size()) {
    aabb = points[0]->aabb;
  }
  for (size_t p = 0; p < polys.size(); ++p) aabb.unionAABB(polys[p]->aabb);
  for (size_t p = 0; p < lines.size(); ++p) aabb.unionAABB(lines[p]->aabb);
  for (size_t p = 0; p < points.size(); ++p) aabb.unionAABB(points[p]->aabb);

  GLuint dlist = glGenLists((GLsizei)(*listSize = n));
  is_wireframe.resize(n, false);

  double scale_fac = 20.0 / aabb.extent[carve::geom::largestAxis(aabb.extent)];

  if (options.fit) {
    g_translation = -aabb.pos;
    g_scale = scale_fac;
  } else {
    g_translation = carve::geom::VECTOR(0.0,0.0,0.0);
    g_scale = 1.0;
  }

  unsigned list_num = 0;

  for (size_t p = 0; p < polys.size(); ++p) {
    carve::poly::Polyhedron *poly = polys[p];
    for (unsigned i = 0; i < poly->manifold_is_closed.size(); i++) {
      if (!poly->manifold_is_closed[i]) {
        is_wireframe[list_num] = false;
        glNewList(dlist + list_num++, GL_COMPILE);
        glCullFace(GL_BACK);
        drawPolyhedron(poly, 0.0f, 0.0f, 0.5f, 1.0f, false, i);
        glCullFace(GL_FRONT);
        drawPolyhedron(poly, 0.0f, 0.0f, 1.0f, 1.0f, false, i);
        glCullFace(GL_BACK);
        glEndList();

        if (options.wireframe) {
          is_wireframe[list_num] = true;
          glNewList(dlist + list_num++, GL_COMPILE);
          drawPolyhedronWireframe(poly, options.normal, i);
          glEndList();
        }
      }
    }

    for (unsigned i = 0; i < poly->manifold_is_closed.size(); i++) {
      if (poly->manifold_is_closed[i]) {
        is_wireframe[list_num] = false;
        glNewList(dlist + list_num++, GL_COMPILE);
        glCullFace(GL_BACK);
        drawPolyhedron(poly, 0.3f, 0.5f, 0.8f, 1.0f, false, i);
        glCullFace(GL_FRONT);
        drawPolyhedron(poly, 1.0f, 0.0f, 0.0f, 1.0f, false, i);
        glCullFace(GL_BACK);
        glEndList();

        if (options.wireframe) {
          is_wireframe[list_num] = true;
          glNewList(dlist + list_num++, GL_COMPILE);
          drawPolyhedronWireframe(poly, options.normal, i);
          glEndList();
        }
      }
    }
  }

  for (size_t l = 0; l < lines.size(); ++l) {
    carve::line::PolylineSet *line = lines[l];

    for (carve::line::PolylineSet::line_iter i = line->lines.begin(); i != line->lines.end(); ++i) {
      is_wireframe[list_num] = false;
      glNewList(dlist + list_num++, GL_COMPILE);
      glBegin((*i)->isClosed() ? GL_LINE_LOOP : GL_LINE_STRIP);
      glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
      for (carve::line::polyline_vertex_iter j = (*i)->vbegin(); j != (*i)->vend(); ++j) {
        carve::geom3d::Vector v = (*j)->v;
        glVertex3f(g_scale * (v.x + g_translation.x),
                   g_scale * (v.y + g_translation.y),
                   g_scale * (v.z + g_translation.z));
      }
      glEnd();
      glEndList();
      is_wireframe[list_num] = true;
      glNewList(dlist + list_num++, GL_COMPILE);
      glPointSize(2.0);
      glColor4f(1.0f, 1.0f, 0.0f, 1.0f);
      glBegin(GL_POINTS);
      for (carve::line::polyline_vertex_iter j = (*i)->vbegin(); j != (*i)->vend(); ++j) {
        carve::geom3d::Vector v = (*j)->v;
        glVertex3f(g_scale * (v.x + g_translation.x),
                   g_scale * (v.y + g_translation.y),
                   g_scale * (v.z + g_translation.z));
      }
      glEnd();
      glEndList();
    }
  }

  for (size_t l = 0; l < points.size(); ++l) {
    carve::point::PointSet *point = points[l];

    is_wireframe[list_num] = false;
    glNewList(dlist + list_num++, GL_COMPILE);
    glPointSize(2.0);
    glBegin(GL_POINTS);
    for (size_t i = 0; i < point->vertices.size(); ++i) {
      carve::geom3d::Vector v = point->vertices[i].v;
      glColor4f(0.0f, 1.0f, 1.0f, 1.0f);
      glVertex3f(g_scale * (v.x + g_translation.x),
                 g_scale * (v.y + g_translation.y),
                 g_scale * (v.z + g_translation.z));
    }
    glEnd();
    glEndList();
  }

  return dlist;
}

struct TestScene : public Scene {
  GLuint draw_list_base;
  std::vector<bool> draw_flags;
  std::vector<bool> is_wireframe;

  virtual bool key(unsigned char k, int x, int y) {
    const char *t;
    static const char *l = "1234567890!@#$%^&*()";
    if (k == '\\') {
      for (unsigned i = 1; i < draw_flags.size(); i += 2) {
        draw_flags[i] = !draw_flags[i];
      }
    } else if (k == 'n') {
      bool n = true;
      for (unsigned i = 0; i < draw_flags.size(); ++i)
        if (is_wireframe[i] && draw_flags[i]) n = false;
      for (unsigned i = 0; i < draw_flags.size(); ++i)
        if (is_wireframe[i]) draw_flags[i] = n;
    } else if (k == 'm') {
      bool n = true;
      for (unsigned i = 0; i < draw_flags.size(); ++i)
        if (!is_wireframe[i] && draw_flags[i]) n = false;
      for (unsigned i = 0; i < draw_flags.size(); ++i)
        if (!is_wireframe[i]) draw_flags[i] = n;
    } else {
      t = strchr(l, k);
      if (t != NULL) {
        CARVE_ASSERT(t >= l);
        unsigned layer = t - l;
        if (layer < draw_flags.size()) {
          draw_flags[layer] = !draw_flags[layer];
        }
      }
    }
    return true;
  }

  virtual GLvoid draw() {
    for (unsigned i = 0; i < draw_flags.size(); ++i) {
      if (draw_flags[i]) glCallList(draw_list_base + i);
    }
  }

  TestScene(int argc, char **argv, int n_dlist) : Scene(argc, argv) {
    draw_list_base = 0;
  }

  virtual ~TestScene() {
    glDeleteLists(draw_list_base, draw_flags.size());
  }
};

int main(int argc, char **argv) {
  TestScene *scene = new TestScene(argc, argv, std::min(1, argc - 1));
  options.parse(argc, argv);

  size_t count = 0;

  carve::input::Input inputs;
  std::vector<carve::poly::Polyhedron *> polys;
  std::vector<carve::line::PolylineSet *> lines;
  std::vector<carve::point::PointSet *> points;

  if (options.files.size() == 0) {
    if (options.obj) {
      readOBJ(std::cin, inputs);
    } else if (options.vtk) {
      readVTK(std::cin, inputs);
    } else {
      readPLY(std::cin, inputs);
    }

  } else {
    for (size_t idx = 0; idx < options.files.size(); ++idx) {
      std::string &s(options.files[idx]);
      std::string::size_type i = s.rfind(".");

      if (i != std::string::npos) {
        std::string ext = s.substr(i, s.size() - i);
        if (!strcasecmp(ext.c_str(), ".obj")) {
          readOBJ(s, inputs);
        } else if (!strcasecmp(ext.c_str(), ".vtk")) {
          readVTK(s, inputs);
        } else {
          readPLY(s, inputs);
        }
      } else {
        readPLY(s, inputs);
      }
    }
  }

  for (std::list<carve::input::Data *>::const_iterator i = inputs.input.begin(); i != inputs.input.end(); ++i) {
    carve::poly::Polyhedron *p;
    carve::point::PointSet *ps;
    carve::line::PolylineSet *l;

    if ((p = carve::input::Input::create<carve::poly::Polyhedron>(*i)) != NULL)  {
      polys.push_back(p);
      std::cerr << "loaded polyhedron "
                << polys.back() << " has " << polys.back()->manifold_is_closed.size()
                << " manifolds (" << std::count(polys.back()->manifold_is_closed.begin(),
                                                polys.back()->manifold_is_closed.end(),
                                                true) << " closed)" << std::endl; 
    } else if ((l = carve::input::Input::create<carve::line::PolylineSet>(*i)) != NULL)  {
      lines.push_back(l);
      std::cerr << "loaded polyline set "
                << lines.back() << std::endl; 
    } else if ((ps = carve::input::Input::create<carve::point::PointSet>(*i)) != NULL)  {
      points.push_back(ps);
      std::cerr << "loaded point set "
                << points.back() << std::endl; 
    }
  }

  scene->draw_list_base = genSceneDisplayList(polys, lines, points, &count, scene->is_wireframe);
  scene->draw_flags.assign(count, true);

  scene->run();

  delete scene;

  return 0;
}
