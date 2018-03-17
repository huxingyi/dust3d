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

#include "scene.hpp"

#include <carve/matrix.hpp>
#include <carve/geom3d.hpp>

#include <GL/glui.h>

static int lastx = 0, lasty = 0;
static unsigned buttons;
static int lastbutton = 0;

static Scene *g_scene = NULL;
static int g_mainWindow = NULL;
static GLUI *g_rightPanel = NULL;
static double near_plane = 0.2;
static double far_plane = 200;
carve::math::Matrix g_projection, g_modelview;

void Scene::updateDisplay() {
  if (CAM_ELEVATION < -90.0) CAM_ELEVATION = -90;
  if (CAM_ELEVATION > 90.0) CAM_ELEVATION = 90;
  if (CAM_DIST < 0.05) {
    CAM_DIST = 0.05;
  }
  glutPostWindowRedisplay(g_mainWindow);
  glutPostRedisplay();
}

carve::geom3d::Vector rotateWithVector(const carve::geom3d::Vector &x, carve::geom3d::Vector u, float ang) {
  carve::geom3d::Vector h,v,uxx;

  u.normalize();


  uxx = cross(u, x) * (float)sin(ang);

  h = u * (dot(x,u));

  v = (x - h) * (float)cos(ang);

  return (h + v) + uxx;
}

// Our world is Z up.
const carve::geom3d::Vector WORLD_RIGHT = carve::geom::VECTOR(1,0,0);
const carve::geom3d::Vector WORLD_UP = carve::geom::VECTOR(0,0,1);
const carve::geom3d::Vector WORLD_IN = carve::geom::VECTOR(0,1,0);

void getCameraVectors(float rot, float elev, carve::geom3d::Vector &right, carve::geom3d::Vector &up, carve::geom3d::Vector &in) {
  right = WORLD_RIGHT;
  up = WORLD_UP;
  in = WORLD_IN;

  right = rotateWithVector(right, WORLD_RIGHT, carve::math::radians(elev));
  up = rotateWithVector(up, WORLD_RIGHT, carve::math::radians(elev));
  in = rotateWithVector(in, WORLD_RIGHT, carve::math::radians(elev));
  right = rotateWithVector(right, WORLD_UP, carve::math::radians(rot));
  up = rotateWithVector(up, WORLD_UP, carve::math::radians(rot));
  in = rotateWithVector(in, WORLD_UP, carve::math::radians(rot));
}

GLvoid Scene::_drag(int x, int y) {
  int dx = x - lastx;
  int dy = y - lasty;

  if (buttons == 0x04) {
    CAM_DIST *= 1.0 + 0.01 * (y - lasty);
    CAM_DIST_REAL *= 1.0 + 0.01 * (y - lasty);
    updateDisplay();
  } else if (buttons == 0x01) {
    CAM_ELEVATION += 0.5 * (y-lasty);
    CAM_ROT -= 0.5 * (x - lastx);
    updateDisplay();
  } else if (buttons == 0x02) {
    carve::geom3d::Vector right, up, in;
    getCameraVectors(CAM_ROT, CAM_ELEVATION, right, up, in);

    right.scaleBy(2 * dx * CAM_DIST / WIDTH);
    up.scaleBy(2 * dy * CAM_DIST / HEIGHT);
    CAM_LOOK += right;
    CAM_LOOK += up;
    CAM_LOOK_REAL += right;
    CAM_LOOK_REAL += up;
    updateDisplay();
  }


  lastx = x;
  lasty = y;
}

GLvoid Scene::_click(int button, int state, int x, int y) {
  unsigned mask = 1 << button;
  if (state) {
    buttons &= ~mask;
  } else {
    buttons |= mask;
  }

  lastx = x;
  lasty = y;
  click(button, state, x, y);
}

GLvoid Scene::_key(unsigned char k, int x, int y) {
  double rate = 1.0;
  if (isupper(k)) { rate = 0.1; k = tolower(k); }

  switch (k) {
  case 'q':
    exit(0);
    break;
  case 'g':
    disp_grid = !disp_grid;
    break;
  case 'h':
    disp_axes = !disp_axes;
    break;
  case 'w':
    if (CAM_ELEVATION > -85.0) CAM_ELEVATION -= 5.0 * rate;
    break;
  case 's':
    if (CAM_ELEVATION < 85.0) CAM_ELEVATION += 5.0 * rate;
    break;
  case 'a':
    CAM_ROT += 5.0 * rate;
    break;
  case 'd':
    CAM_ROT -= 5.0 * rate;
    break;
  case 'r':
    CAM_DIST += 2.0 * rate;
    break;
  case 'f':
    CAM_DIST -= 2.0 * rate;
    break;
  case 'i':
    CAM_LOOK.x -= cos(carve::math::radians(CAM_ROT)) * 2.0 * rate;
    CAM_LOOK.z -= sin(carve::math::radians(CAM_ROT)) * 2.0 * rate;
    break;
  case 'j':
    CAM_LOOK.x -= sin(carve::math::radians(CAM_ROT)) * 2.0 * rate;
    CAM_LOOK.z += cos(carve::math::radians(CAM_ROT)) * 2.0 * rate;
    break;
  case 'k':
    CAM_LOOK.x += cos(carve::math::radians(CAM_ROT)) * 2.0 * rate;
    CAM_LOOK.z += sin(carve::math::radians(CAM_ROT)) * 2.0 * rate;
    break;
  case 'l':
    CAM_LOOK.x += sin(carve::math::radians(CAM_ROT)) * 2.0 * rate;
    CAM_LOOK.z -= cos(carve::math::radians(CAM_ROT)) * 2.0 * rate;
    break;
  case 'z':
    near_plane *= 1.1;
    break;
  case 'x':
    near_plane /= 1.1;
    break;
  case 'c':
    far_plane *= 1.1;
    break;
  case 'v':
    far_plane /= 1.1;
    break;
  default: {
    if (!key(k, x, y)) goto skip_redisplay;
    break;
  }
  }
  updateDisplay();
 skip_redisplay:;
}


GLvoid Scene::_draw() {
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  double w = WIDTH;
  double h = HEIGHT;

  if (h == 0) h = 1;
  glViewport(0, 0, WIDTH, HEIGHT);
  if (w > h) {
    double r = w / h;
    glFrustum(-0.2 * r, 0.2 * r, -0.2, 0.2, near_plane, far_plane);
  } else {
    double r = h / w;
    glFrustum(-0.2, 0.2, -0.2 * r, 0.2 * r, near_plane, far_plane);
  }

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  CAM_LOOK_REAL = (CAM_LOOK_REAL * 4 + CAM_LOOK) / 5;
  CAM_DIST_REAL = (CAM_DIST_REAL * 4 + CAM_DIST) / 5;

  // if they are far apart post another redisplay
  if ((CAM_LOOK_REAL - CAM_LOOK).length() > 0.001) {
    glutPostRedisplay();
  } else {
    CAM_LOOK_REAL = CAM_LOOK;
  }
  if (fabs(CAM_DIST_REAL - CAM_DIST) > 0.001) {
    glutPostRedisplay();
  } else {
    CAM_DIST_REAL = CAM_DIST;
  }

  carve::geom3d::Vector right, up, in;
  getCameraVectors(CAM_ROT, CAM_ELEVATION, right, up, in);
  in = CAM_DIST_REAL * in;
  gluLookAt(CAM_LOOK_REAL.x + in.x, CAM_LOOK_REAL.y + in.y, CAM_LOOK_REAL.z + in.z,
            CAM_LOOK_REAL.x, CAM_LOOK_REAL.y, CAM_LOOK_REAL.z,
            up.x, up.y, up.z);

  glGetDoublev(GL_MODELVIEW_MATRIX, g_modelview.v);
  glGetDoublev(GL_PROJECTION_MATRIX, g_projection.v);


  glShadeModel(GL_SMOOTH);
  glClearDepth(1.0);
  glClearColor(0.05f, 0.075f, 0.2f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  {
    const float amb[] = {0.05f, 0.05f, 0.05f, 1.0f};
    const float diff[] = {1.0f, 0.8f, 0.5f, 1.0f};
    const float spec[] = {0.4f, 0.4f, 0.4f, 1.0f};
    const float pos[] = {-40.0f, 40.0f, 80.0f, 0.0f};

    glLightfv(GL_LIGHT0, GL_AMBIENT, amb);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diff);
    glLightfv(GL_LIGHT0, GL_SPECULAR, spec);
    glLightfv(GL_LIGHT0, GL_POSITION, pos);
  }

  {
    const float amb[] = {0.05f, 0.05f, 0.05f, 1.0f};
    const float diff[] = {0.8f, 0.8f, 1.0f, 1.0f};
    const float spec[] = {0.4f, 0.4f, 0.4f, 1.0f};
    const float pos[] = {+50.0f, -10.0f, 10.0f, 0.0f};

    glLightfv(GL_LIGHT1, GL_AMBIENT, amb);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, diff);
    glLightfv(GL_LIGHT1, GL_SPECULAR, spec);
    glLightfv(GL_LIGHT1, GL_POSITION, pos);
  }

  glCullFace(GL_BACK);

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_LIGHT0);
  glEnable(GL_LIGHT1);
  glEnable(GL_LIGHTING);
  glEnable(GL_NORMALIZE);
  glEnable(GL_COLOR_MATERIAL);

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glDisable(GL_LIGHTING);
  glDisable(GL_DEPTH);

  if (disp_grid) {
    glBegin(GL_LINE_LOOP);
    glColor4f(1,1,1,0.5);
    glVertex3f(-30,-30,0);
    glVertex3f(+30,-30,0);
    glVertex3f(+30,+30,0);
    glVertex3f(-30,+30,0);
    glEnd();

    glBegin(GL_LINES);
    glColor4f(1.0f, 1.0f, 1.0f, 0.3f);
    for (int i = 0; i < 30; i++) {
      glVertex3f(i * 2.0f - 30.0f, -30.0f, 0.0f);
      glVertex3f(i * 2.0f - 30.0f, +30.0f, 0.0f);
      glVertex3f(-30.0f, i * 2.0f - 30.0f, 0.0f);
      glVertex3f(+30.0f, i * 2.0f - 30.0f, 0.0f);
    }
    glEnd();
  }

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_LIGHTING);
  glEnable(GL_COLOR_MATERIAL);
  glDepthFunc(GL_LESS);
  glDepthMask(GL_TRUE);

  draw();

  glDisable(GL_COLOR_MATERIAL);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_LIGHTING);

  if (disp_axes) {
    glBegin(GL_LINES);
    glColor4f(1,0,0,1); glVertex3f(0,0,0); glVertex3f(10,0,0);
    glColor4f(0,1,0,1); glVertex3f(0,0,0); glVertex3f(0,10,0);
    glColor4f(0,0,1,1); glVertex3f(0,0,0); glVertex3f(0,0,10);
    glEnd();
  }

  glPopMatrix();

  glutSwapBuffers();
}

bool Scene::key(unsigned char k, int x, int y) {
  return false;
}

GLvoid Scene::draw() {
}

void Scene::click(int button, int state, int x, int y) {
}

GLvoid Scene::_resize(int w, int h) {
  int tx, ty, tw, th;
  GLUI_Master.get_viewport_area(&tx, &ty, &tw, &th);
  WIDTH = tw;
  HEIGHT = th;
}

void control_cb(int control) {
  glutPostRedisplay();
  glutPostWindowRedisplay(g_mainWindow);
}

#define WIREFRAME_ENABLED_ID    200

int wireframe = 0;

static std::map<OptionGroup*, GLUI_Rollout *> groupToRollouts;
static std::map<Option*, GLUI_Checkbox *> optionToCheckboxes;

OptionGroup* Scene::createOptionGroup(const char *caption) {
  // make sure our UI has been initialised.
  init();

  // Create a rollout GUI item.
  OptionGroup *group = new OptionGroup();
  GLUI_Rollout *rollout = new GLUI_Rollout(g_rightPanel, caption);
  groupToRollouts[group] = rollout;
  return group;
}

Option* OptionGroup::createOption(const char *caption, bool initialValue) {
  GLUI_Rollout *rollout = groupToRollouts[this];
  if (rollout == NULL) {
    return NULL;
  }

  GLUI_Checkbox *cb = new GLUI_Checkbox(rollout, caption, NULL, 1, control_cb);
  cb->set_int_val(initialValue);

  Option *option = new Option();

  optionToCheckboxes[option] = cb;

  return option;

}

bool Option::isChecked() {
  GLUI_Checkbox *cb = optionToCheckboxes[this];
  if (cb != NULL) {
    return cb->get_int_val();
  } else {
    return false;
  }
}

void Option::setChecked(bool value) {
  GLUI_Checkbox *cb = optionToCheckboxes[this];
  if (cb != NULL) {
    return cb->set_int_val(value);
  }
}

void Scene::init() {
  if (!initialised) {
    initialised = true;

    GLUI_Master.set_glutDisplayFunc(s_draw);
    GLUI_Master.set_glutReshapeFunc(s_resize);
    GLUI_Master.set_glutKeyboardFunc(s_key);
    GLUI_Master.set_glutSpecialFunc(NULL);
    GLUI_Master.set_glutMouseFunc(s_click);
    GLUI_Master.set_glutMotionFunc(s_drag);

    g_rightPanel = GLUI_Master.create_glui_subwindow(g_mainWindow, GLUI_SUBWINDOW_RIGHT);

    this->_init();
  }
}

carve::geom3d::Ray Scene::getRay(int x, int y) {
  carve::geom3d::Vector from, to;

  GLint view[4];
  glGetIntegerv(GL_VIEWPORT, view);
  gluUnProject(x, view[3] - y, 0, g_modelview.v, g_projection.v, view, &from.x, &from.y, &from.z);
  gluUnProject(x, view[3] - y, 50, g_modelview.v, g_projection.v, view, &to.x, &to.y, &to.z);


  return carve::geom3d::Ray((to - from).normalized(), from);
}

void Scene::zoomTo(carve::geom3d::Vector pos, double dist) {
  CAM_LOOK = pos;
  CAM_DIST = dist;
  updateDisplay();
}
void Scene::_init() {

}

GLvoid Scene::s_draw() { g_scene->_draw(); }
GLvoid Scene::s_resize(int w, int h) { g_scene->_resize(w, h); }
GLvoid Scene::s_drag(int x, int y) { g_scene->_drag(x, y); }
GLvoid Scene::s_click(int button, int state, int x, int y) { g_scene->_click(button, state, x, y); }
GLvoid Scene::s_key(unsigned char k, int x, int y) { g_scene->_key(k, x, y); }

Scene::Scene(int argc, char **argv) {
  CAM_ROT = 0.0;
  CAM_ELEVATION = 45.0;
  CAM_DIST = 70.0;
  CAM_DIST_REAL = 10000;

  WIDTH = 1024;
  HEIGHT = 768;

  disp_grid = true;
  disp_axes = true;

  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);

  glutInitWindowSize(WIDTH, HEIGHT);
  g_mainWindow = glutCreateWindow("Main");

  initialised = false;

  g_scene = this;
}

Scene::~Scene() {
  g_scene = NULL;
}

void Scene::run() {
  init();
  glutMainLoop();
}
