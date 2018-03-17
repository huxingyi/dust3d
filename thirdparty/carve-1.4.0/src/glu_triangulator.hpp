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

#if defined(__APPLE__)
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else

#ifdef WIN32
#include <windows.h>
#undef rad1
#undef rad2
#endif

#include <GL/gl.h>
#include <GL/glu.h>

#endif

#include <carve/csg.hpp>

class GLUTriangulator : public carve::csg::CSG::Hook {
  GLUtesselator *tess;
  GLenum curr_type;

  std::vector<const carve::poly::Vertex<3> *> vertices;
  std::vector<carve::poly::Face<3> *> new_faces;
  const carve::poly::Face<3> *orig_face;

public:
  GLUTriangulator();
  virtual ~GLUTriangulator();
  virtual void processOutputFace(std::vector<carve::poly::Face<3> *> &faces,
                                 const carve::poly::Face<3> *orig,
                                 bool flipped);

  void faceBegin(GLenum type);
  void faceVertex(const carve::poly::Vertex<3> *vertex);
  void faceEnd();
};
