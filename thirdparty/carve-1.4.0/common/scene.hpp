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


#pragma once

#include <carve/carve.hpp>

#include <carve/geom3d.hpp>

#include <gloop/gloopgl.hpp>
#include <gloop/gloopglu.hpp>
#include <gloop/gloopglut.hpp>

class Option {
public:
  bool isChecked();
  void setChecked(bool value);
};

class OptionGroup {
public:
  Option* createOption(const char *caption, bool initialValue);
};

class Scene {
  static GLvoid s_drag(int x, int y);
  static GLvoid s_click(int button, int state, int x, int y);
  static GLvoid s_key(unsigned char k, int x, int y);
  static GLvoid s_draw();
  static GLvoid s_resize(int w, int h);

  carve::geom3d::Vector CAM_LOOK;
  double CAM_ROT;
  double CAM_ELEVATION;
  double CAM_DIST;
  carve::geom3d::Vector CAM_LOOK_REAL;
  double CAM_DIST_REAL;


  int WIDTH;
  int HEIGHT;
  
  bool initialised;
  bool disp_grid;
  bool disp_axes;

  GLvoid _drag(int x, int y);
  GLvoid _click(int button, int state, int x, int y);
  GLvoid _key(unsigned char k, int x, int y);
  GLvoid _draw();
  GLvoid _resize(int w, int h);
  virtual void _init();

  void updateDisplay();
  
protected:

  virtual bool key(unsigned char k, int x, int y);
  virtual GLvoid draw();
  virtual void click(int button, int state, int x, int y);
  

public:
  Scene(int argc, char **argv);
  virtual ~Scene();
  virtual void run();
  void init();

  carve::geom3d::Ray getRay(int x, int y);
  void zoomTo(carve::geom3d::Vector pos, double dist);
  OptionGroup* createOptionGroup(const char *title);

};
