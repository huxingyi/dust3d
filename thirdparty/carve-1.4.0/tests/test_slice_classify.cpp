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
#include "geom_draw.hpp"
#include "geometry.hpp"

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

#define POINTS 60

int main(int argc, char **argv) {
  carve::poly::Polyhedron *a = makeCube(carve::math::Matrix::ROT(1.0, 1.0, 1.0, 1.0));
  
  std::vector<carve::poly::Vertex<3> > shape;

  for (int i = 0; i < POINTS; ++i) {
    double r = 2.0 + .4 * sin(i * 3 * M_TWOPI / POINTS) + .8 * sin(i * 5 * M_TWOPI / POINTS);
    shape.push_back(carve::poly::Vertex<3>(carve::geom::VECTOR(r * cos(i * M_TWOPI / POINTS), r * sin(i * M_TWOPI / POINTS), 0.0)));
  }
  std::vector<const carve::poly::Vertex<3> *> face_verts;
  for (int i = 0; i < POINTS; ++i) {
    face_verts.push_back(&shape[i]);
  }
  std::vector<carve::poly::Face<3> > faces;
  faces.push_back(carve::poly::Face<3>(face_verts));

  carve::poly::Polyhedron *b = new carve::poly::Polyhedron(faces);

  std::list<std::pair<carve::csg::FaceClass, carve::poly::Polyhedron *> > b_sliced;

  carve::csg::CSG().sliceAndClassify(a, b, b_sliced);

  TestScene *scene = new TestScene(argc, argv, 6);

  glNewList(scene->draw_list_base + 0, GL_COMPILE);
  drawPolyhedron(a, .4, .6, .8, 1.0, false);
  glEndList();

  glNewList(scene->draw_list_base + 1, GL_COMPILE);
  drawPolyhedron(b, .8, .6, .4, 1.0, false);
  glEndList();

  glNewList(scene->draw_list_base + 2, GL_COMPILE);
  {
    int n = 0;
    for (std::list<std::pair<carve::csg::FaceClass, carve::poly::Polyhedron *> >::iterator i = b_sliced.begin(); i != b_sliced.end(); ++i) {
      float r, g, b;
      switch ((*i).first) {
      case carve::csg::FACE_IN:             r = 0.0; g = 0.0; b = 1.0; break;
      case carve::csg::FACE_OUT:            r = 1.0; g = 0.0; b = 0.0; break;
      case carve::csg::FACE_ON_ORIENT_OUT:  r = 1.0; g = 1.0; b = 0.0; break;
      case carve::csg::FACE_ON_ORIENT_IN:   r = 0.0; g = 1.0; b = 1.0; break;
      }
      drawPolyhedron((*i).second, r, g, b, 1.0, true);
      ++n;
    }
  }
  glEndList();

  glNewList(scene->draw_list_base + 3, GL_COMPILE);
  {
    int n = 0;
    for (std::list<std::pair<carve::csg::FaceClass, carve::poly::Polyhedron *> >::iterator i = b_sliced.begin(); i != b_sliced.end(); ++i) {
      float r, g, b;
      switch ((*i).first) {
      case carve::csg::FACE_IN:             r = 0.3; g = 0.3; b = 0.7; break;
      case carve::csg::FACE_OUT:            r = 0.7; g = 0.3; b = 0.3; break;
      case carve::csg::FACE_ON_ORIENT_OUT:  r = 0.7; g = 0.7; b = 0.3; break;
      case carve::csg::FACE_ON_ORIENT_IN:   r = 0.3; g = 0.7; b = 0.7; break;
      }
      drawPolyhedronWireframe((*i).second);
      ++n;
    }
  }
  glEndList();

  scene->run();

  delete scene;

  return 0;
}
