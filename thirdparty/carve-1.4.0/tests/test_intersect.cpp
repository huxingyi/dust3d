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
#include <carve/timing.hpp>
#include <carve/convex_hull.hpp>

#include "geom_draw.hpp"
#include "geometry.hpp"
#include "read_ply.hpp"
#include "write_ply.hpp"

#include "rgb.hpp"

#include "scene.hpp"

#include "opts.hpp"

#include <gloop/gloopgl.hpp>
#include <gloop/gloopglu.hpp>
#include <gloop/gloopglut.hpp>

#include <fstream>
#include <string>
#include <utility>
#include <set>

#include <time.h>
#include <assert.h>

#ifdef WIN32
#undef min
#undef max
#endif



struct Options : public opt::Parser {
  bool edge_classifier;
  bool rescale;
  bool output;
  std::string output_file;
  bool ascii;

  std::vector<std::string> args;
  
  virtual void optval(const std::string &o, const std::string &v) {
    if (o == "--binary"  || o == "-b") { ascii = false; return; }
    if (o == "--ascii"   || o == "-a") { ascii = true; return; }
    if (o == "--rescale" || o == "-r") { rescale = true; return; }
    if (o == "--output"  || o == "-o") { output = true; output_file = v; return; }
    if (o == "--edge"    || o == "-e") { edge_classifier = true; return; }
  }

  virtual void arg(const std::string &a) {
    args.push_back(a);
  }

  Options() {
    ascii = true;
    rescale = false;
    output = false;
    edge_classifier = false;

    option("binary",  'b', false, "binary output");
    option("ascii",   'a', false, "ascii output (default)");
    option("rescale", 'r', false, "rescale prior to CSG operations");
    option("output",  'o', true,  "output result in .ply format");
    option("edge",    'e', false, "use edge classifier");
  }
};


static Options options;


carve::poly::Polyhedron *g_result = NULL;

std::vector<carve::geom3d::LineSegment> rays;

#if 1
std::string data_path = "/Users/sargeant/projects/PERSONAL/CARVE/data/";
#else
std::string data_path = "../data/";
#endif

struct TestScene : public Scene {
  GLuint draw_list_base;
  std::vector<Option*> draw_flags;

  virtual bool key(unsigned char k, int x, int y) {
    const char *t;
    static const char *l = "1234567890!@#$%^&*()";
    t = strchr(l, k);
    if (t != NULL) {
      int layer = t - l;
      if (layer < draw_flags.size()) {
        draw_flags[layer]->setChecked(!draw_flags[layer]->isChecked());
      }
    }
    return true;
  }

  virtual GLvoid draw() {
    for (int i = 0; i < draw_flags.size(); ++i) {
      if (draw_flags[i]->isChecked()) {
        glCallList(draw_list_base + i);
      }
    }

    if (rays.size()) {
      glBegin(GL_LINES);
      for (int i = 0; i < rays.size(); ++i) {
        carve::geom3d::Vector a = rays[i].v1, b = rays[i].v2;
        
        glVertex3f(a.x, a.y, a.z);
        glVertex3f(b.x, b.y, b.z);
      }
      glEnd();
    }
  }

  virtual void click(int button, int state, int x, int y) {
    if ((glutGetModifiers() & GLUT_ACTIVE_CTRL) != 0 && state == GLUT_DOWN) {

      carve::geom3d::Ray r = getRay(x, y);

      r.v = r.v / g_scale;
      r.v = r.v - g_translation;
      carve::geom3d::Vector from = r.v;
      carve::geom3d::Vector to = r.v - r.D * 1000;

      //rays.push_back(LineSegment(g_scale * (from + g_translation), g_scale * (to + g_translation)));
      std::vector<const carve::poly::Face<3> *> faces;

      g_result->findFacesNear(carve::geom3d::LineSegment(from, to), faces);

      // see if any of the faces intersect our ray
      for (int i = 0; i < faces.size();++i) {
        const carve::poly::Face<3> *f = faces[i];
        carve::geom3d::Vector pos;
        if (f->lineSegmentIntersection(carve::geom3d::LineSegment(from, to), pos) > 0) {
          pos = g_scale * (pos + g_translation);
          carve::geom3d::Vector fromWorld = g_scale * (from + g_translation);
          zoomTo(pos, 0.7 * (fromWorld - pos).length());
        }
      }
    }
  }

  TestScene(int argc, char **argv, int n_dlist) : Scene(argc, argv) {
    draw_list_base = glGenLists(n_dlist);
  }

  virtual ~TestScene() {
    glDeleteLists(draw_list_base, draw_flags.size());
  }

  virtual void _init() {
  }
};

bool odd(int x, int y, int z) {
  return ((x + y + z) & 1) == 1;
}

bool even(int x, int y, int z) {
  return ((x + y + z) & 1) == 0;
}

class Input {
public:
  Input() {
    poly = NULL;
    op = carve::csg::CSG::UNION;
    ownsPoly = true;
  }

  // Our copy constructor actually transfers ownership.
  Input(const Input &i) {
    poly = i.poly;
    op = i.op;
    i.ownsPoly = false;
    ownsPoly = true;
  }

  Input(carve::poly::Polyhedron *p, carve::csg::CSG::OP o, bool becomeOwner = true) {
    poly = p;
    op = o;
    ownsPoly = becomeOwner;
  }

  ~Input() {
    if (ownsPoly) {
      delete poly;
    }
  }

  carve::poly::Polyhedron *poly;
  carve::csg::CSG::OP op;
  mutable bool ownsPoly;

private:
};

void getInputsFromTest(int test, std::list<Input> &inputs) {
  carve::csg::CSG::OP op = carve::csg::CSG::INTERSECTION;
  carve::poly::Polyhedron *a = NULL;
  carve::poly::Polyhedron *b = NULL;
  carve::poly::Polyhedron *c = NULL;

  switch (test) {
  case 0:
    a = makeCube(carve::math::Matrix::SCALE(2.0, 2.0, 2.0));
    b = makeCube(carve::math::Matrix::SCALE(2.0, 2.0, 2.0) *
                 carve::math::Matrix::ROT(1.0, 1.0, 1.0, 1.0) *
                 carve::math::Matrix::TRANS(1.0, 1.0, 1.0));
    break;
  case 1:
    a = makeCube(carve::math::Matrix::SCALE(2.0, 2.0, 2.0));
    b = makeCube(carve::math::Matrix::TRANS(1.0, 0.0, 0.0));
    break;

  case 2:
    a = makeTorus(20, 20, 2.0, 1.0, carve::math::Matrix::ROT(0.5, 1.0, 1.0, 1.0));
    b = makeTorus(20, 20, 2.0, 1.0, carve::math::Matrix::TRANS(0.0, 0.0, 0.0));
    op = carve::csg::CSG::A_MINUS_B;
    break;

  case 4:
    a = makeDoubleCube(carve::math::Matrix::SCALE(2.0, 2.0, 2.0));
    b = makeTorus(20, 20, 2.0, 1.0, carve::math::Matrix::ROT(M_PI / 2.0, 0.1, 0.0, 0.0));
    break;

  case 5:
    a = makeCube(carve::math::Matrix::TRANS(0.0, 0.0, -0.5));
    b = makeCube(carve::math::Matrix::TRANS(0.0, 0.0, +0.5));
    break;

  case 6:
    a = makeCube();
    b = makeCube(carve::math::Matrix::ROT(M_PI/4.0, 0.0, 0.0, +1.0));
    break;

  case 7:
    a = makeCube();
    b = makeCube(carve::math::Matrix::ROT(M_PI/4.0, 0.0, 0.0, +1.0) * carve::math::Matrix::TRANS(0.0, 0.0, 1.0));
    break;

  case 8:
    a = makeCube();
    b = makeCube(carve::math::Matrix::ROT(M_PI/4.0, 0.0, 0.0, +1.0) * carve::math::Matrix::SCALE(sqrt(2.0)/2.0, sqrt(2.0)/2.0, 0.1) * carve::math::Matrix::TRANS(0.0, 0.0, 0.1));
    break;

  case 9:
    a = makeCube();
    b = makeSubdividedCube(3, 3, 3, NULL, carve::math::Matrix::TRANS(0.0, 0.0, 0.5) * carve::math::Matrix::SCALE(0.5, 0.5, 0.5));
    break;

  case 12:
    a = makeCube();
    b = makeCube(carve::math::Matrix::TRANS(.5, .0, 1.0) *
                 carve::math::Matrix::SCALE(sqrt(2.0)/4.0, sqrt(2.0)/4.0, 0.1) *
                 carve::math::Matrix::ROT(M_PI/4.0, 0.0, 0.0, +1.0));
    break;

  case 13:
    a = makeSubdividedCube(3, 3, 3, NULL);
    b = makeSubdividedCube(3, 3, 3, NULL,
                           carve::math::Matrix::TRANS(0.0, 0.0, 0.5) *
                           carve::math::Matrix::SCALE(0.5, 0.5, 0.5));
    break;

  case 14:
    a = makeSubdividedCube(3, 3, 3, odd);
    b = makeSubdividedCube(3, 3, 3, even);
    op = carve::csg::CSG::UNION;
    break;

  case 15:
    a = makeSubdividedCube(3, 3, 1, odd);
    b = makeSubdividedCube(3, 3, 1);
    op = carve::csg::CSG::UNION;
    break;

  case 16:
    a = readPLY(data_path + "cylinderx.ply");
    b = readPLY(data_path + "cylindery.ply");
    op = carve::csg::CSG::UNION;
    break;

  case 17:
    a = readPLY(data_path + "coneup.ply");
    b = readPLY(data_path + "conedown.ply");
    op = carve::csg::CSG::UNION;
    break;

  case 18:
    a = readPLY(data_path + "coneup.ply");
    b = readPLY(data_path + "conedown.ply");
    op = carve::csg::CSG::A_MINUS_B;
    break;

  case 19:
    a = readPLY(data_path + "sphere.ply");
    b = readPLY(data_path + "sphere.ply");
    op = carve::csg::CSG::UNION;
    break;

  case 20:
    a = readPLY(data_path + "sphere.ply");
    b = readPLY(data_path + "sphere.ply");
    op = carve::csg::CSG::A_MINUS_B;
    break;

  case 21:
    a = readPLY(data_path + "sphere.ply");
    b = readPLY(data_path + "sphere.ply", carve::math::Matrix::TRANS(0.01, 0.01, 0.01));
    op = carve::csg::CSG::A_MINUS_B;
    break;

  case 22:
    a = readPLY(data_path + "cylinderx.ply");
    b = readPLY(data_path + "cylindery.ply");
    op = carve::csg::CSG::UNION;
    break;

  case 23:
    a = readPLY(data_path + "cylinderx.ply");
    b = readPLY(data_path + "cylindery.ply");
    c = readPLY(data_path + "cylinderz.ply");
    op = carve::csg::CSG::UNION;
    break;

  case 24:
    a = makeCube();
    b = makeCube(carve::math::Matrix::ROT(1e-5, 1.0, 1.0, 0.0));
    op = carve::csg::CSG::UNION;
    break;

  case 25:
    a = makeCube();
    b = makeCube(carve::math::Matrix::ROT(1e-1, 1.0, 1.0, 0.0));
    op = carve::csg::CSG::UNION;
    break;

  case 26:
    a = makeCube();
    b = makeCube(carve::math::Matrix::ROT(1e-5, 1.0, 1.0, 0.0));
    op = carve::csg::CSG::B_MINUS_A;
    break;

  case 27:
    a = makeCube();
    b = makeCube(carve::math::Matrix::ROT(1e-2, 1.0, 1.0, 0.0));
    op = carve::csg::CSG::B_MINUS_A;
    break;

  case 28:
    a = makeCube();
    b = makeCube(carve::math::Matrix::ROT(1e-6, 1.0, 0.0, 0.0));
    c = makeCube(carve::math::Matrix::ROT(2e-6, 1.0, 0.0, 0.0));
    op = carve::csg::CSG::UNION;
    break;

  case 29:
    for (int i = 0; i < 30; i++) {
      inputs.push_back(Input(makeCube(carve::math::Matrix::ROT(i * M_TWOPI / 30, .4, .3, .7)), carve::csg::CSG::UNION));
    }
    break;

  case 30:
    inputs.push_back(Input(readPLY(data_path + "sphere.ply"), carve::csg::CSG::UNION));
    inputs.push_back(Input(readPLY(data_path + "sphere.ply", carve::math::Matrix::SCALE(0.9, 0.9, 0.9)), carve::csg::CSG::A_MINUS_B));
    inputs.push_back(Input(makeCube(carve::math::Matrix::TRANS(5.5, 0.0, 0.0) * carve::math::Matrix::SCALE(5.0, 5.0, 5.0)), carve::csg::CSG::A_MINUS_B));
    break;

  case 31:
    inputs.push_back(Input(makeCube(), carve::csg::CSG::UNION));
    inputs.push_back(Input(makeCube(carve::math::Matrix::SCALE(0.9, 0.9, 0.9)), carve::csg::CSG::A_MINUS_B));
    inputs.push_back(Input(makeCube(carve::math::Matrix::TRANS(5.5, 0.0, 0.0) * carve::math::Matrix::SCALE(5.0, 5.0, 5.0)), carve::csg::CSG::A_MINUS_B));
    break;

  case 32:
    inputs.push_back(Input(readPLY(data_path + "ico.ply"), carve::csg::CSG::UNION));
    inputs.push_back(Input(readPLY(data_path + "ico.ply", carve::math::Matrix::SCALE(0.9, 0.9, 0.9)), carve::csg::CSG::A_MINUS_B));
    inputs.push_back(Input(makeCube(carve::math::Matrix::TRANS(5.5, 0.0, 0.0) * carve::math::Matrix::SCALE(5.0, 5.0, 5.0)), carve::csg::CSG::A_MINUS_B));
    break;

  case 33:
    inputs.push_back(Input(readPLY(data_path + "ico2.ply"), carve::csg::CSG::UNION));
    inputs.push_back(Input(readPLY(data_path + "ico2.ply", carve::math::Matrix::SCALE(0.9, 0.9, 0.9)), carve::csg::CSG::A_MINUS_B));
    inputs.push_back(Input(makeCube(carve::math::Matrix::TRANS(5.5, 0.0, 0.0) * carve::math::Matrix::SCALE(5.0, 5.0, 5.0)), carve::csg::CSG::A_MINUS_B));
    break;

  case 34:
    inputs.push_back(Input(readPLY(data_path + "cow2.ply"), carve::csg::CSG::UNION));
    inputs.push_back(Input(readPLY(data_path + "cow2.ply", carve::math::Matrix::TRANS(0.5, 0.5, 0.5)), carve::csg::CSG::UNION));
    break;

  case 35:
    inputs.push_back(Input(readPLY(data_path + "201addon.ply"), carve::csg::CSG::UNION));
    inputs.push_back(Input(readPLY(data_path + "addontun.ply"), carve::csg::CSG::UNION));
    break;

  case 36:
    inputs.push_back(Input(readPLY(data_path + "../Bloc/block1.ply"), carve::csg::CSG::INTERSECTION));
    inputs.push_back(Input(readPLY(data_path + "../Bloc/debug1.ply"), carve::csg::CSG::INTERSECTION));
    inputs.push_back(Input(readPLY(data_path + "../Bloc/debug2.ply"), carve::csg::CSG::INTERSECTION));
    break;

  case 37:
    a = readPLY("../data/sphere_one_point_moved.ply");
    b = readPLY("../data/sphere.ply");
    op = carve::csg::CSG::A_MINUS_B;
    break;
  }
    
  

  if (a != NULL) {
    inputs.push_back(Input(a, carve::csg::CSG::UNION));
  }
  if (b != NULL) {
    inputs.push_back(Input(b, op));
  }
  if (c != NULL) {
    inputs.push_back(Input(c, op));
  }

//   glPointSize(4.0);
//   glEnable(GL_DEPTH_TEST);
//   glBegin(GL_POINTS);
//   for (int i = 0; i < 1000; i++) {
//     double x = 4.0 * random() / (double)RAND_MAX - 2;
//     double y = 4.0 * random() / (double)RAND_MAX - 2;
//     double z = 4.0 * random() / (double)RAND_MAX - 2;
//     switch (b->containsVertex(Vector(x, y, z))) {
//     case POINT_IN:  glColor4f(1,1,1,1); break;
//     case POINT_ON:  glColor4f(1,0,0,1); break;
//     case POINT_OUT: glColor4f(0,0,0,1); break;
//     }
//     glVertex3f(x,y,z);
//   }
//   glEnd();
}

static bool endswith(const std::string &a, const std::string &b) {
  if (a.size() < b.size()) return false;

  for (unsigned i = a.size(), j = b.size(); j; ) {
    if (tolower(a[--i]) != tolower(b[--j])) return false;
  }
  return true;
}

void testCSG(GLuint &dlist, std::list<Input>::const_iterator begin, std::list<Input>::const_iterator end, carve::poly::Polyhedron *&finalResult, TestScene *scene) {
  // Can't do anything with the terminating iterator
  if (begin == end) {
    return;
  }

  bool result_is_temp = true;

  // If this is the first time around, we use the first input as our first intermediate result.
  if (finalResult == NULL) {
    finalResult = begin->poly;
    result_is_temp = false;
    ++begin;
  }

  OptionGroup *group = scene->createOptionGroup("CSG debug");

  while (begin != end) {
    // Okay, we have a polyhedron in result that will be our first operand, and also our output,
    // and we have a list of operations and second operands in our iterator list.

    carve::poly::Polyhedron *a = finalResult;
    carve::poly::Polyhedron *b = begin->poly;
    carve::csg::CSG::OP op = begin->op;

    glNewList(dlist++, GL_COMPILE);
    scene->draw_flags.push_back(group->createOption("Debug data visible", false));
    if (a && b) {
      carve::poly::Polyhedron *result = NULL;
      try {
        result = carve::csg::CSG().compute(a, b, op, NULL, options.edge_classifier ? carve::csg::CSG::CLASSIFY_EDGE : carve::csg::CSG::CLASSIFY_NORMAL);

        std::cerr << "a->octree.root->is_leaf = " << a->octree.root->is_leaf << std::endl;
        std::cerr << "b->octree.root->is_leaf = " << b->octree.root->is_leaf << std::endl;
        std::cerr << "result = " << result << std::endl
                  << "       n(manifolds) = " << result->manifold_is_closed.size() << std::endl
                  << "       n(open manifolds) = " << std::count(result->manifold_is_closed.begin(),
                                                                 result->manifold_is_closed.end(),
                                                                 false) << std::endl;
        writePLY(std::cout, result, true);

        // Place the result of this CSG into our final result, and get rid of our last one
        std::swap(result, finalResult);
        if (result_is_temp) delete result;
        result_is_temp = true;

      } catch (carve::exception e) {
        std::cerr << "FAIL- " << e.str();
        if (result_is_temp && finalResult) delete finalResult;
        finalResult = NULL;
      }
    }
    ++begin;
    glEndList();
  }
}

void testConvexHull() {
#define N 100

  std::vector<carve::geom2d::P2> points;
#if defined(__APPLE__)
  srandomdev();
#else
  srandom(time(NULL));
#endif
  points.reserve(100);
  glPointSize(5.0);
  glEnable(GL_POINT_SMOOTH);

  glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

  glBegin(GL_POINTS);
  for (int i = 0; i < 100; i++) {
    double a = random() / double(RAND_MAX) * M_TWOPI;
    double d = random() / double(RAND_MAX) * 10.0;

    points.push_back(carve::geom::VECTOR(cos(a) * d, sin(a) * d));

    glVertex3f((GLfloat)points.back().x, (GLfloat)points.back().y, 0.1f);
  }
  glEnd();

  std::vector<int> result = carve::geom::convexHull(points);

  glBegin(GL_LINE_LOOP);
  for (unsigned i = 0; i < result.size(); i++) {
    glVertex3f((GLfloat)points[result[i]].x, (GLfloat)points[result[i]].y, 0.1f);
  }
  glEnd();

  glBegin(GL_LINES);

  glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
  glVertex3f( 0.0f, 0.0f, 0.1f);
  glVertex3f(20.0f, 0.0f, 0.1f);

  glColor4f(0.0f, 1.0f, 0.0f, 1.0f);
  glVertex3f(0.0f,  0.0f, 0.1f);
  glVertex3f(0.0f, 20.0f, 0.1f);

  glEnd();
}

void genSceneDisplayList(const std::list<Input> &inputs, TestScene *scene) {
  std::list<Input>::const_iterator i = inputs.begin();

  carve::geom3d::Vector min, max;

  if (i != inputs.end()) {
    carve::geom3d::AABB aabb = i->poly->aabb;
    min = aabb.min();
    max = aabb.max();

    for (;i != inputs.end(); ++i) {
      aabb = i->poly->aabb;

      assign_op(min, min, aabb.min(), carve::util::min_functor());
      assign_op(max, max, aabb.max(), carve::util::max_functor());
    }
  } else {
    min.fill(-1.0);
    max.fill(+1.0);
  }

  std::cerr << "X: " << min.x << " - " << max.x << std::endl;
  std::cerr << "Y: " << min.y << " - " << max.y << std::endl;
  std::cerr << "Z: " << min.z << " - " << max.z << std::endl;

  double scale_fac = 40.0 / std::max(max.x - min.x, max.y - min.y);

  g_translation = -carve::geom::VECTOR((min.x + max.x) / 2.0,
                                           (min.y + max.y) / 2.0,
                                           (min.z + max.z) / 2.0);
  g_scale = scale_fac;
  std::cerr << "scale fac: " << scale_fac << std::endl;

  g_result = NULL;

  GLuint currentList = scene->draw_list_base;

  testCSG(currentList, inputs.begin(), inputs.end(), g_result, scene);

  {
    OptionGroup *group;
    group = scene->createOptionGroup("Result");

    scene->draw_flags.push_back(group->createOption("Result visible", true));
    glNewList(currentList++, GL_COMPILE);
    if (g_result) {
      glCullFace(GL_BACK);
      drawPolyhedron(g_result, 0.3f, 0.5f, 0.8f, 1.0f, true);
      glCullFace(GL_FRONT);
      drawPolyhedron(g_result, 0.8f, 0.5f, 0.3f, 1.0f, true);
      glCullFace(GL_BACK);
    }
    glEndList();

    scene->draw_flags.push_back(group->createOption("Result wireframe", true));
    glNewList(currentList++, GL_COMPILE);
    if (g_result) {
      drawPolyhedronWireframe(g_result);
    }
    glEndList();
  }

  {
    OptionGroup *group = scene->createOptionGroup("Inputs");
    char buf[1024];
    int count = 0;
    float H = 0.0, S = 1.0, V = 1.0;

    for (std::list<Input>::const_iterator it = inputs.begin(); it != inputs.end(); ++it) {
      H = fmod((H + .37), 1.0);
      S = 0.5 + fmod((S - 0.37), 0.5);
      cRGB colour = HSV2RGB(H, S, V);

      count++;
      sprintf(buf, "Input %d wireframe", count);
      scene->draw_flags.push_back(group->createOption(buf, false));
      glNewList(currentList++, GL_COMPILE);
      if (it->poly) drawPolyhedronWireframe(it->poly);
      glEndList();

      sprintf(buf, "Input %d solid", count);
      scene->draw_flags.push_back(group->createOption(buf, false));
      glNewList(currentList++, GL_COMPILE);
      if (it->poly) drawPolyhedron(it->poly, colour.r, colour.g, colour.b, true);
      glEndList();

      if (it->poly->octree.root->is_leaf) {
        sprintf(buf, "Input %d octree (unsplit)", count);
      } else {
        sprintf(buf, "Input %d octree", count);
      }
      scene->draw_flags.push_back(group->createOption(buf, false));
      glNewList(currentList++, GL_COMPILE);
      if (it->poly) { drawOctree(it->poly->octree); }
      glEndList();
    }
  }
}

static bool isInteger(const char *str) {
  int count = 0;
  while (*str) {
    if (!isdigit(*str)) {
      return false;
    }
    ++str;
    ++count;
  }
  return count > 0;
}


int main(int argc, char **argv) {
  installDebugHooks();

  int test = 0;
  int inputFilenamesStartAt = -1;

  options.parse(argc, argv);
  
  if (options.args.size() == 0) {
    test = 0;
  } else if (options.args.size() == 1 && isInteger(options.args[0].c_str())) {
    test = atoi(options.args[0].c_str());
  } else {
    std::cerr << "invalid test: " << options.args[0] << std::endl;
    exit(1);
  }

  {
    std::list<Input> inputs;

    static carve::TimingName MAIN_BLOCK("Application");
    static carve::TimingName PARSE_BLOCK("Parse");

    carve::Timing::start(MAIN_BLOCK);

    carve::Timing::start(PARSE_BLOCK);

    getInputsFromTest(test, inputs);

    carve::Timing::stop();

    //                                            inputs             result    debug
    TestScene *scene = new TestScene(argc, argv, (3 * inputs.size()) + 2 + (inputs.size() - 1));
    scene->init();
    genSceneDisplayList(inputs, scene);

    carve::Timing::stop();

    carve::Timing::printTimings();

    scene->run();

    delete scene;
  }

  return 0;
}
