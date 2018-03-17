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
#include <carve/math.hpp>

#include "scene.hpp"
#include "rgb.hpp"
#include "geom_draw.hpp"

#if defined(__APPLE__)
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#endif

#include <iostream>
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

int main(int argc, char **argv) {
#define N_NORMALS 4
  carve::geom3d::Vector normals[N_NORMALS];

  normals[0] = carve::geom::VECTOR(cos(.1),sin(.1),sin(.1)).normalized();
  normals[1] = carve::geom::VECTOR(cos(.8),sin(.8),sin(.8)).normalized();
  normals[2] = carve::geom::VECTOR(cos(-1.6),sin(-1.6),sin(-1.6)).normalized();
  normals[3] = carve::geom::VECTOR(cos(-2.5),sin(-2.5),sin(-2.5)).normalized();

  TestScene *scene = new TestScene(argc, argv, 1);

  double d[100][100];
  carve::geom3d::Vector p[100][100];

  double max_d = 0.0;
  double min_d = 10000.0;

  for (int ring = 0; ring < 100; ++ring) {
    double ra = M_PI * ring / 99.0;
    double z = cos(ra);
    double rr = sin(ra);
    for (int slice = 0; slice < 100; ++slice) {
      double rb = M_PI * 2.0 * slice / 100.0;
      double x = sin(rb) * rr;
      double y = cos(rb) * rr;
      p[ring][slice] = carve::geom::VECTOR(x,y,z);
      double dist = 0.0;
      for (int i = 0; i < N_NORMALS; ++i) {
        dist += pow(dot(normals[i], p[ring][slice]), 2.0);
      }
      std::cerr << x << " " << y << " " << z << "  " << dist << std::endl;
      d[ring][slice] = dist;
      max_d = std::max(dist, max_d);
      min_d = std::min(dist, min_d);
    }
  }

  glNewList(scene->draw_list_base, GL_COMPILE);

  glBegin(GL_TRIANGLES);
  for (int ring = 0; ring < 99; ++ring) {
    for (int slice = 0; slice < 100; ++slice) {
#define VERT(x, y) do { double k = (d[x][y] - min_d) / (max_d - min_d); k = pow(k, 0.1); glColor4f(k, k, k, 1.0); glVertex3dv(p[x][y].v); } while(0)
      VERT(ring, slice);
      VERT(ring + 1, (slice + 1) % 100);
      VERT(ring + 1, slice);

      VERT(ring, slice);
      VERT(ring, (slice + 1) % 100);
      VERT(ring + 1, (slice + 1) % 100);
    }
  }
  glEnd();

  carve::math::Matrix3 m;
  for (int i = 0; i < 3; ++i) {
    for (int j = i; j < 3; ++j) {
      double d = 0.0;
      for (int k = 0; k < N_NORMALS; ++k) {
        d += normals[k].v[i] * normals[k].v[j];
      }
      m.m[i][j] = m.m[j][i] = d;
    }
  }

  double l1, l2, l3;
  carve::geom3d::Vector e1, e2, e3;
  carve::math::eigSolveSymmetric(m, l1, e1, l2, e2, l3, e3);
  e1 = e1 * 2.0;
  e2 = e2 * 2.0;
  e3 = e3 * 2.0;

  glLineWidth(3.0);
  glBegin(GL_LINES);
  if (fabs(l1) < fabs(l2) && fabs(l1) < fabs(l3)) glColor4f(1,1,1,1); else glColor4f(0,0,0,1);
  glVertex3f(0,0,0);
  glVertex3dv(e1.v);
  if (fabs(l2) < fabs(l1) && fabs(l2) < fabs(l3)) glColor4f(1,1,1,1); else glColor4f(0,0,0,1);
  glVertex3f(0,0,0);
  glVertex3dv(e2.v);
  if (fabs(l3) < fabs(l1) && fabs(l3) < fabs(l2)) glColor4f(1,1,1,1); else glColor4f(0,0,0,1);
  glVertex3f(0,0,0);
  glVertex3dv(e3.v);
  glEnd();
  glLineWidth(1.0);

  glEnable(GL_BLEND);
  glDisable(GL_LIGHTING);
  glDisable(GL_CULL_FACE);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glBegin(GL_LINES);
  for (int i = 0; i < N_NORMALS; i++) {
    carve::geom3d::Vector N = normals[i];
    N = N * 2.0;
    glColor4f(.5 + (i & 1 ? .5 : .0), .5 + (i & 2 ? .5 : .0), .5 + (i & 4 ? .5 : .0), 1);
    glVertex3f(0,0,0);
    glVertex3dv(N.v);
  }
  glEnd();

  glBegin(GL_QUADS);
  for (int i = 0; i < N_NORMALS; i++) {
    carve::geom3d::Vector N = normals[i];
    carve::geom3d::Vector V1 = carve::geom::VECTOR(N.y, N.z, N.x);
    carve::geom3d::Vector V2 = cross(N, V1).normalized();
    V1 = cross(N, V2);
    carve::geom3d::Vector V;

    glColor4f(.5 + (i & 1 ? .5 : .0), .5 + (i & 2 ? .5 : .0), .5 + (i & 4 ? .5 : .0), .2);
    V = + 3 * V1 + 3 * V2 ; glVertex3dv(V.v);
    V = + 3 * V1 - 3 * V2 ; glVertex3dv(V.v);
    V = - 3 * V1 - 3 * V2 ; glVertex3dv(V.v);
    V = - 3 * V1 + 3 * V2 ; glVertex3dv(V.v);
  }
  glEnd();

  glDisable(GL_BLEND);
  glEnable(GL_LIGHTING);
  glEnable(GL_CULL_FACE);

  glEndList();

  scene->draw_flags[0] = true;

  scene->run();

  delete scene;

  return 0;
}
