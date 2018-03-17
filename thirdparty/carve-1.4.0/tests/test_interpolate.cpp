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
#include <carve/interpolator.hpp>

#include "scene.hpp"
#include "rgb.hpp"

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

#define POINTS 80

double rad(int p) {
  return .6 + .2 * (sin(p * 6 * M_TWOPI / POINTS) + sin(p * 5 * M_TWOPI / POINTS));
}

double H(int p) { return .5 + .5 * cos(p * M_TWOPI * 2 / double(POINTS)); }
double S(int p) { return .8 + .2 * cos(p * M_TWOPI * 8 / double(POINTS)); }
double V(int p) { return .8 + .2 * sin(.3 + p * M_TWOPI * 5 / double(POINTS)) * sin(p * M_TWOPI * 9 / double(POINTS)); }

int main(int argc, char **argv) {
  TestScene *scene = new TestScene(argc, argv, 2);

  std::vector<carve::geom2d::P2> poly;
  std::vector<cRGBA> colour;
  for (int i = 0; i < POINTS; ++i) {
    double r = rad(i);
    poly.push_back(carve::geom::VECTOR(cos(i * M_TWOPI / POINTS) * r, sin(i * M_TWOPI / POINTS) * r));
    colour.push_back(HSV2RGB(H(i), S(i), V(i)));
  }

  glNewList(scene->draw_list_base, GL_COMPILE);
  glDisable(GL_LIGHTING);
  glBegin(GL_TRIANGLES);

  for (int x = -100; x < +100; x++) {
    double X = x / 100.0;
    double X2 = (x + 1) / 100.0;
    for (int y = -100; y < +100; y++) {
      double Y = y / 100.0;
      double Y2 = (y + 1) / 100.0;
      cRGBA c1 = carve::interpolate::interp(poly, colour, X, Y, colour_clamp_t());
      cRGBA c2 = carve::interpolate::interp(poly, colour, X2, Y, colour_clamp_t());
      cRGBA c3 = carve::interpolate::interp(poly, colour, X2, Y2, colour_clamp_t());
      cRGBA c4 = carve::interpolate::interp(poly, colour, X, Y2, colour_clamp_t());

      glColor4f(c1.r, c1.g, c1.b, c1.a);
      glVertex3f(X * 20, Y * 20, 1.0);
      glColor4f(c2.r, c2.g, c2.b, c2.a);
      glVertex3f(X2 * 20, Y * 20, 1.0);
      glColor4f(c3.r, c3.g, c3.b, c3.a);
      glVertex3f(X2 * 20, Y2 * 20, 1.0);
      
      glColor4f(c1.r, c1.g, c1.b, c1.a);
      glVertex3f(X * 20, Y * 20, 1.0);
      glColor4f(c3.r, c3.g, c3.b, c3.a);
      glVertex3f(X2 * 20, Y2 * 20, 1.0);
      glColor4f(c4.r, c4.g, c4.b, c4.a);
      glVertex3f(X * 20, Y2 * 20, 1.0);
    }
  }

  glEnd();
  glEnable(GL_LIGHTING);

  glEndList();

  glNewList(scene->draw_list_base + 1, GL_COMPILE);
  glDisable(GL_LIGHTING);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glBegin(GL_TRIANGLES);
  for (int i = 0; i < POINTS; ++i) {
    int i2 = (i + 1) % POINTS;
    glColor4f(0, 0, 0, 0);
    glVertex3f(0.0, 0.0, 2.0);
    glColor4f(colour[i].r, colour[i].g, colour[i].b, 1.0);
    glVertex3f(poly[i].x * 20, poly[i].y * 20, 2.0);
    glColor4f(colour[i2].r, colour[i2].g, colour[i2].b, 1.0);
    glVertex3f(poly[i2].x * 20, poly[i2].y * 20, 2.0);
  }
  glEnd();
  glDisable(GL_BLEND);
  glEnable(GL_LIGHTING);
  glEndList();

  scene->draw_flags[0] = true;
  scene->draw_flags[1] = true;

  scene->run();

  delete scene;

  return 0;
}
