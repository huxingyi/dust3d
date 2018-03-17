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

#include "scene.hpp"
#include "rgb.hpp"
#include "geom_draw.hpp"
#include "read_ply.hpp"

#if defined(__APPLE__)
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#endif

#include <fstream>
#include <string>
#include <utility>
#include <set>

#include <time.h>

struct TestScene : public Scene {
  GLuint draw_list_base;
  std::vector<bool> draw_flags;

  virtual bool key(unsigned char k, int x, int y) {
    const char *t;
    static const char *l = "1234567890!@#$%^&*()";
    t = strchr(l, k);
    if (t != NULL) {
      int layer = t - l;
      if (layer < draw_flags.size()) {
        draw_flags[layer] = !draw_flags[layer];
      }
    }
    return true;
  }

  virtual GLvoid draw() {
    for (int i = 0; i < draw_flags.size(); ++i) {
      if (draw_flags[i]) glCallList(draw_list_base + i);
    }
  }

  TestScene(int argc, char **argv, int n_dlist) : Scene(argc, argv) {
    draw_list_base = glGenLists(n_dlist);

    draw_flags.resize(n_dlist, false);
  }

  virtual ~TestScene() {
    glDeleteLists(draw_list_base, draw_flags.size());
  }
};

// XXX: for the moment, we're only handling simple closed surfaces.

struct EdgePlane {
  carve::geom3d::Vector base, dir, norm;
};

typedef std::unordered_map<const carve::poly::Edge *, EdgePlane, carve::poly::hash_edge_ptr> EPMap;

void makeEdgePlanes(const carve::poly::Polyhedron *poly, EPMap &edge_planes) {
#if defined(UNORDERED_COLLECTIONS_SUPPORT_RESIZE)
  edge_planes.resize(poly->edges.size());
#endif

  for (size_t i = 0; i < poly->edges.size(); ++i) {
    EdgePlane &ep(edge_planes[&poly->edges[i]]);

    CARVE_ASSERT(poly->edges[i].faces.size() == 2);

    const carve::poly::Face *f1 = poly->edges[i].faces[0];
    const carve::poly::Face *f2 = poly->edges[i].faces[1];

    CARVE_ASSERT(f1 && f2);

    ep.base = poly->edges[i].v2->v - poly->edges[i].v1->v;
    ep.base.normalize();
    carve::geom3d::Vector d_1 = f1->plane_eqn.N + f2->plane_eqn.N;
    carve::geom3d::Vector d_2 = cross(f1->plane_eqn.N, ep.base) - cross(f2->plane_eqn.N, ep.base);
    ep.dir = d_2.length2() > d_1.length2() ? d_2 : d_1;
    ep.dir.normalize();
    ep.norm = cross(ep.dir, ep.base);

          if (true) {
            carve::geom3d::Vector v1 = (poly->edges[i].v1->v + poly->edges[i].v2->v) / 2.0;
            carve::geom3d::Vector v2 = v1 + 0.075 * ep.dir;
            glBegin(GL_LINES);
            glColor4f(1.0, 1.0, 1.0, 1.0);
            glVertex3f(v1.x, v1.y, v1.z);
            glColor4f(1.0, 1.0, 1.0, 0.4);
            glVertex3f(v2.x, v2.y, v2.z);
            glEnd();
          }
  }
}

void makeVertexPaths(const carve::poly::Polyhedron *poly, const EPMap &edge_planes) {
  for (size_t i = 0; i < poly->poly_vertices.size(); ++i) {
    const carve::poly::Vertex *v = &poly->poly_vertices[i];
    const std::list<const carve::poly::Edge *> &edges = v->v_edges;

    carve::geom3d::Vector sum = carve::geom::VECTOR(0.0, 0.0, 0.0);

    for (std::list<const carve::poly::Edge *>::const_iterator i = edges.begin(), e = edges.end(); i != e; ++i) {
      const carve::poly::Edge *e1 = (*i);
      const EdgePlane &ep1(edge_planes.find(e1)->second);
      carve::geom3d::Vector e1n = e1->v2 == v ? ep1.norm : -ep1.norm;

      std::list<const carve::poly::Edge *>::const_iterator j = i;
      for (++j; j != e; ++j) {
        const carve::poly::Edge *e2 = (*j);
        const EdgePlane &ep2(edge_planes.find(e2)->second);
        carve::geom3d::Vector e2n = e2->v1 == v ? ep2.norm : -ep2.norm;

        carve::geom3d::Vector vertex_dir = cross(e2n, e1n);

        if (vertex_dir.isZero()) {
          vertex_dir = (ep1.dir + ep2.dir).normalized();
        } else {
          vertex_dir.normalize();
        }
        sum += vertex_dir;
      }
    }
    sum.normalize();

  }
}

void doOffset(const carve::poly::Polyhedron *poly, const double OFFSET) {
  EPMap edge_planes;

  makeEdgePlanes(poly, edge_planes);
  makeVertexPaths(poly, edge_planes);

  const carve::poly::Face *f = poly->faces.front();
}

int main(int argc, char **argv) {
  carve::poly::Polyhedron *input = readPLY(argv[1]);
  double offset = strtod(argv[2], NULL);

  TestScene *scene = new TestScene(argc, argv, 3);

  glNewList(scene->draw_list_base, GL_COMPILE);
  doOffset(input, offset);
  glEndList();

  glNewList(scene->draw_list_base + 1, GL_COMPILE);
  drawPolyhedron(input, .6, .6, .6, 1.0, false);
  glEndList();

  glNewList(scene->draw_list_base + 2, GL_COMPILE);
  drawPolyhedronWireframe(input);
  glEndList();

  scene->draw_flags[0] = true;
  scene->draw_flags[1] = true;
  scene->draw_flags[2] = true;

  scene->run();

  delete scene;

  return 0;
}
