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

#include "geom_draw.hpp"

#include <carve/debug_hooks.hpp>

#include <gloop/gloopgl.hpp>
#include <gloop/gloopglu.hpp>
#include <gloop/gloopglut.hpp>

#include <fstream>
#include <string>

#include <time.h>

#if defined(__GNUC__)
#define __stdcall
#endif

#if defined(GLU_TESS_CALLBACK_VARARGS)
  typedef GLvoid (_stdcall *GLUTessCallback)(...);
#else
  typedef void (__stdcall *GLUTessCallback)();
#endif

carve::geom3d::Vector g_translation;
double g_scale = 1.0;

static inline void glVertex(double x, double y, double z) {
  glVertex3f(g_scale * (x + g_translation.x),
             g_scale * (y + g_translation.y),
             g_scale * (z + g_translation.z));
}

static inline void glVertex(const carve::geom3d::Vector *v) {
  glVertex3f(g_scale * (v->x + g_translation.x),
             g_scale * (v->y + g_translation.y),
             g_scale * (v->z + g_translation.z));
}

static inline void glVertex(const carve::geom3d::Vector &v) {
  glVertex3f(g_scale * (v.x + g_translation.x),
             g_scale * (v.y + g_translation.y),
             g_scale * (v.z + g_translation.z));
}

static inline void glVertex(const carve::poly::Vertex<3> *v) {
  glVertex(v->v);
}

static inline void glVertex(const carve::poly::Vertex<3> &v) {
  glVertex(v.v);
}

class DebugHooks : public carve::csg::IntersectDebugHooks {
public:
  virtual void drawIntersections(const carve::csg::VertexIntersections &vint);

  virtual void drawOctree(const carve::csg::Octree &o);

  virtual void drawPoint(const carve::geom3d::Vector *v,
      float r, float g, float b, float a,
      float rad);

  virtual void drawEdge(const carve::geom3d::Vector *v1, const carve::geom3d::Vector *v2,
      float rA, float gA, float bA, float aA,
      float rB, float gB, float bB, float aB,
      float thickness = 1.0);

  virtual void drawFaceLoopWireframe(const std::vector<const carve::geom3d::Vector *> &face_loop,
      const carve::geom3d::Vector &normal,
      float r, float g, float b, float a,
      bool inset = true);

  virtual void drawFaceLoop(const std::vector<const carve::geom3d::Vector *> &face_loop,
      const carve::geom3d::Vector &normal,
      float r, float g, float b, float a,
      bool offset = true,
      bool lit = true);

  virtual void drawFaceLoop2(const std::vector<const carve::geom3d::Vector *> &face_loop,
      const carve::geom3d::Vector &normal,
      float rF, float gF, float bF, float aF,
      float rB, float gB, float bB, float aB,
      bool offset = true,
      bool lit = true);
};

void DebugHooks::drawPoint(const carve::geom3d::Vector *v,
                           float r, float g, float b, float a,
                           float rad) {
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_LIGHTING);
  glEnable(GL_POINT_SMOOTH);
  glPointSize(rad);
  glBegin(GL_POINTS);
  glColor4f(r, g, b, a);
  glVertex(v);
  glEnd();
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_LIGHTING);
}

void DebugHooks::drawEdge(const carve::geom3d::Vector *v1, const carve::geom3d::Vector *v2,
                          float rA, float gA, float bA, float aA,
                          float rB, float gB, float bB, float aB,
                          float thickness) {
  glDisable(GL_LIGHTING);
  glDisable(GL_DEPTH_TEST);
  glLineWidth(thickness);
  glEnable(GL_LINE_SMOOTH);

  glBegin(GL_LINES);

  glColor4f(rA, gA, bA, aA);
  glVertex(v1);

  glColor4f(rB, gB, bB, aB);
  glVertex(v2);
 
  glEnd();

  glDisable(GL_LINE_SMOOTH);
  glLineWidth(1.0);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_LIGHTING);
}

void DebugHooks::drawIntersections(const carve::csg::VertexIntersections &vint) {
  glEnable(GL_POINT_SMOOTH);
  glDisable(GL_DEPTH_TEST);
  for (carve::csg::VertexIntersections::const_iterator
         i = vint.begin(), e = vint.end();
       i != e;
       ++i) {
    float sz = 4.0 + (*i).second.size() * 3.0;
    for (carve::csg::VertexIntersections::mapped_type::const_iterator
           j = (*i).second.begin(), je = (*i).second.end(); j != je; ++j) {
      glPointSize(sz);
      sz -= 3.0;
      switch ((*j).first.obtype | (*j).second.obtype) {
      case 0: glColor4f(0,0,0,1); break;
      case 1: glColor4f(0,0,1,1); break; // VERTEX - VERTEX
      case 2: glColor4f(0,1,1,1); break; // EDGE - EDGE
      case 3: glColor4f(1,0,0,1); break; // EDGE - VERTEX
      case 4: glColor4f(0,0,0,1); break;
      case 5: glColor4f(1,1,0,1); break; // FACE - VERTEX
      case 6: glColor4f(0,1,0,1); break; // FACE - EDGE
      case 7: glColor4f(0,0,0,1); break;
      }
      glBegin(GL_POINTS);
      glVertex((*i).first);
      glEnd();
    }
  }
  glEnable(GL_DEPTH_TEST);
}

void drawCube(const carve::geom3d::Vector &a, const carve::geom3d::Vector &b) {
  glBegin(GL_QUADS);
    glNormal3f(0,0,-1);
    glVertex(a.x, a.y, a.z);
    glVertex(b.x, a.y, a.z);
    glVertex(b.x, b.y, a.z);
    glVertex(a.x, b.y, a.z);
    
    glNormal3f(0,0,1);
    glVertex(a.x, b.y, b.z);
    glVertex(b.x, b.y, b.z);
    glVertex(b.x, a.y, b.z);
    glVertex(a.x, a.y, b.z);

    glNormal3f(-1,0,0);
    glVertex(a.x, a.y, a.z);
    glVertex(a.x, b.y, a.z);
    glVertex(a.x, b.y, b.z);
    glVertex(a.x, a.y, b.z);

    glNormal3f(1,0,0);
    glVertex(b.x, a.y, b.z);
    glVertex(b.x, b.y, b.z);
    glVertex(b.x, b.y, a.z);
    glVertex(b.x, a.y, a.z);

    glNormal3f(0,-1,0);
    glVertex(a.x, a.y, a.z);
    glVertex(b.x, a.y, a.z);
    glVertex(b.x, a.y, b.z);
    glVertex(a.x, a.y, b.z);

    glNormal3f(0,1,0);

    glVertex(a.x, b.y, b.z);
    glVertex(b.x, b.y, b.z);
    glVertex(b.x, b.y, a.z);
    glVertex(a.x, b.y, a.z);

  glEnd();
}

static void drawCell(int level, carve::csg::Octree::Node *node) {
  // we only want to draw leaf nodes
  if (!node->hasChildren() && node->hasGeometry()) {
    glColor3f(1,0,0);
    drawCube(node->min, node->max);
  }
}

void drawOctree(const carve::csg::Octree &o) {
  glDisable(GL_LIGHTING);
  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  glDisable(GL_CULL_FACE);

  o.iterateNodes(&drawCell);

  glEnable(GL_LIGHTING);
}

void DebugHooks::drawOctree(const carve::csg::Octree &o) {
  ::drawOctree(o);
}

static void __stdcall _faceBegin(GLenum type, void *data) {
  carve::poly::Face<3> *face = static_cast<carve::poly::Face<3> *>(data);
  glBegin(type);
  glNormal3f(face->plane_eqn.N.x, face->plane_eqn.N.y, face->plane_eqn.N.z);
}

static void __stdcall _faceVertex(void *vertex_data, void *data) {
  std::pair<carve::geom3d::Vector, cRGBA> &vd(*static_cast<std::pair<carve::geom3d::Vector, cRGBA> *>(vertex_data));
  glColor4f(vd.second.r, vd.second.g, vd.second.b, vd.second.a);
  glVertex3f(vd.first.x, vd.first.y, vd.first.z);
}

static void __stdcall _faceEnd(void *data) {
  glEnd();
}

void drawColourFace(carve::poly::Face<3> *face, const std::vector<cRGBA> &vc, bool offset) {
  if (offset) {
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(0.5, 0.5);
  }

  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

  GLUtesselator *tess = gluNewTess();

  gluTessCallback(tess, GLU_TESS_BEGIN_DATA, (GLUTessCallback)_faceBegin);
  gluTessCallback(tess, GLU_TESS_VERTEX_DATA, (GLUTessCallback)_faceVertex);
  gluTessCallback(tess,  GLU_TESS_END_DATA, (GLUTessCallback)_faceEnd);

  gluTessBeginPolygon(tess, (void *)face);
  gluTessBeginContour(tess);

  std::vector<std::pair<carve::geom3d::Vector, cRGBA> > v;
  v.resize(face->nVertices());
  for (size_t i = 0, l = face->nVertices(); i != l; ++i) {
    v[i] = std::make_pair(g_scale * (face->vertex(i)->v + g_translation), vc[i]);
    gluTessVertex(tess, (GLdouble *)v[i].first.v, (GLvoid *)&v[i]);
  }

  gluTessEndContour(tess);
  gluTessEndPolygon(tess);

  gluDeleteTess(tess);

  if (offset) {
    glDisable(GL_POLYGON_OFFSET_FILL);
  }
}

void drawFace(carve::poly::Face<3> *face, cRGBA fc, bool offset) {
  if (offset) {
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(0.5, 0.5);
  }

  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

  GLUtesselator *tess = gluNewTess();

  gluTessCallback(tess, GLU_TESS_BEGIN_DATA, (GLUTessCallback)_faceBegin);
  gluTessCallback(tess, GLU_TESS_VERTEX_DATA, (GLUTessCallback)_faceVertex);
  gluTessCallback(tess,  GLU_TESS_END_DATA, (GLUTessCallback)_faceEnd);

  gluTessBeginPolygon(tess, (void *)face);
  gluTessBeginContour(tess);

  std::vector<std::pair<carve::geom3d::Vector, cRGBA> > v;
  v.resize(face->nVertices());
  for (size_t i = 0, l = face->nVertices(); i != l; ++i) {
    v[i] = std::make_pair(g_scale * (face->vertex(i)->v + g_translation), fc);
    gluTessVertex(tess, (GLdouble *)v[i].first.v, (GLvoid *)&v[i]);
  }

  gluTessEndContour(tess);
  gluTessEndPolygon(tess);

  gluDeleteTess(tess);

  if (offset) {
    glDisable(GL_POLYGON_OFFSET_FILL);
  }
}

 
void drawFaceWireframe(carve::poly::Face<3> *face, bool normal, float r, float g, float b) {
  glDisable(GL_LIGHTING);
  glDepthMask(GL_FALSE);
  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  glDisable(GL_CULL_FACE);

  glColor4f(r, g, b ,1.0);
  glBegin(GL_POLYGON);
  glColor4f(r, g, b, 0.1f);
  for (size_t i = 0, l = face->nVertices(); i != l; ++i) {
    glVertex(face->vertex(i));
  }
  glEnd();

  glDisable(GL_DEPTH_TEST);

  glColor4f(r, g, b ,0.01);
  glBegin(GL_POLYGON);
  glColor4f(r, g, b, 0.01f);
  for (size_t i = 0, l = face->nVertices(); i != l; ++i) {
    glVertex(face->vertex(i));
  }
  glEnd();

  glLineWidth(3.0f);
  glColor4f(1.0, 0.0, 0.0, 1.0);
  glBegin(GL_LINES);
  for (size_t i = 0, l = face->nEdges(); i != l; ++i) {
    if (static_cast<const carve::poly::Polyhedron *>(face->owner)->connectedFace(face, face->edge(i)) == NULL) {
      glVertex(face->edge(i)->v1);
      glVertex(face->edge(i)->v2);
    }
  }
  glEnd();
  glLineWidth(1.0f);

  glEnable(GL_DEPTH_TEST);

  if (normal) {
    glBegin(GL_LINES);
    glColor4f(1.0, 1.0, 0.0, 1.0);
    glVertex(face->centroid());
    glColor4f(1.0, 1.0, 0.0, 0.0);
    glVertex(face->centroid() + 1 / g_scale * face->plane_eqn.N);
    glEnd();
  }

  glDepthMask(GL_TRUE);
  glEnable(GL_LIGHTING);
}

void drawFaceWireframe(carve::poly::Face<3> *face, bool normal) {
  drawFaceWireframe(face, normal, 0,0,0);
}

void drawFaceNormal(carve::poly::Face<3> *face, float r, float g, float b) {
  glDisable(GL_LIGHTING);
  glDepthMask(GL_FALSE);

  glLineWidth(1.0f);

  glEnable(GL_DEPTH_TEST);

  glBegin(GL_LINES);
  glColor4f(1.0, 1.0, 0.0, 1.0);
  glVertex(face->centroid());
  glColor4f(1.0, 1.0, 0.0, 0.0);
  glVertex(face->centroid() + 1 / g_scale * face->plane_eqn.N);
  glEnd();

  glDepthMask(GL_TRUE);
  glEnable(GL_LIGHTING);
}

void drawFaceNormal(carve::poly::Face<3> *face) {
  drawFaceNormal(face, 0,0,0);
}

void DebugHooks::drawFaceLoopWireframe(const std::vector<const carve::geom3d::Vector *> &face_loop,
                                       const carve::geom3d::Vector &normal,
                                       float r, float g, float b, float a,
                                       bool inset) {
  glDisable(GL_DEPTH_TEST);

  const size_t S = face_loop.size();

  double INSET = 0.005;

  if (inset) {
    glColor4f(r, g, b, a / 2.0);
    glBegin(GL_LINE_LOOP);

    for (size_t i = 0; i < S; ++i) {
      size_t i_pre = (i + S - 1) % S;
      size_t i_post = (i + 1) % S;

      carve::geom3d::Vector v1 = (*face_loop[i] - *face_loop[i_pre]).normalized();
      carve::geom3d::Vector v2 = (*face_loop[i] - *face_loop[i_post]).normalized();

      carve::geom3d::Vector n1 = cross(normal, v1);
      carve::geom3d::Vector n2 = cross(v2, normal);

      carve::geom3d::Vector v = *face_loop[i];

      carve::geom3d::Vector p1 = v + INSET * n1;
      carve::geom3d::Vector p2 = v + INSET * n2;

      carve::geom3d::Vector i1 , i2;
      double mu1, mu2;

      carve::geom3d::Vector p;

      if (carve::geom3d::rayRayIntersection(carve::geom3d::Ray(v1, p1), carve::geom3d::Ray(v2, p2), i1, i2, mu1, mu2)) {
        p = (i1 + i2) / 2;
      } else {
        p = (p1 + p2) / 2;
      }

      glVertex(&p);
    }

    glEnd();
  }

  glColor4f(r, g, b, a);

  glBegin(GL_LINE_LOOP);

  for (unsigned i = 0; i < S; ++i) {
    glVertex(face_loop[i]);
  }

  glEnd();

  glColor4f(r, g, b, a);
  glPointSize(3.0);
  glBegin(GL_POINTS);

  for (unsigned i = 0; i < S; ++i) {
    carve::geom3d::Vector p = *face_loop[i];
    glVertex(face_loop[i]);
  }

  glEnd();

  glEnable(GL_DEPTH_TEST);
}

void DebugHooks::drawFaceLoop(const std::vector<const carve::geom3d::Vector *> &face_loop,
                              const carve::geom3d::Vector &normal,
                              float r, float g, float b, float a,
                              bool offset, 
                              bool lit) {
  if (lit) glEnable(GL_LIGHTING); else glDisable(GL_LIGHTING);
  if (offset) {
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(0.5, 0.5);
  }

  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

  GLUtesselator *tess = gluNewTess();

  gluTessCallback(tess, GLU_TESS_BEGIN, (GLUTessCallback)glBegin);
  gluTessCallback(tess, GLU_TESS_VERTEX, (GLUTessCallback)glVertex3dv);
  gluTessCallback(tess,  GLU_TESS_END, (GLUTessCallback)glEnd);

  glNormal3f(normal.x, normal.y, normal.z);
  glColor4f(r, g, b, a);

  gluTessBeginPolygon(tess, (void *)NULL);
  gluTessBeginContour(tess);

  std::vector<carve::geom3d::Vector> v;
  v.resize(face_loop.size());
  for (size_t i = 0, l = face_loop.size(); i != l; ++i) {
    v[i] = g_scale * (*face_loop[i] + g_translation);
    gluTessVertex(tess, (GLdouble *)v[i].v, (GLvoid *)v[i].v);
  }

  gluTessEndContour(tess);
  gluTessEndPolygon(tess);

  gluDeleteTess(tess);

  if (offset) {
    glDisable(GL_POLYGON_OFFSET_FILL);
  }
  glEnable(GL_LIGHTING);
}

void DebugHooks::drawFaceLoop2(const std::vector<const carve::geom3d::Vector *> &face_loop,
                               const carve::geom3d::Vector &normal,
                               float rF, float gF, float bF, float aF,
                               float rB, float gB, float bB, float aB,
                               bool offset,
                               bool lit) {
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  drawFaceLoop(face_loop, normal, rF, gF, bF, aF, offset, lit);
  glCullFace(GL_FRONT);
  drawFaceLoop(face_loop, normal, rB, gB, bB, aB, offset, lit);
  glCullFace(GL_BACK);
}

void drawPolyhedron(carve::poly::Polyhedron *poly, float r, float g, float b, float a, bool offset, int group) {
  if (offset) {
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(0.5, 0.5);
  }
  glColor4f(r, g, b, a);

  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glBegin(GL_TRIANGLES);
  for (size_t i = 0, l = poly->faces.size(); i != l; ++i) {
    carve::poly::Face<3> &f = poly->faces[i];
    if (group == -1 || f.manifold_id == group) {
      if (f.nVertices() == 3) {
        glNormal3dv(f.plane_eqn.N.v);
        glVertex(f.vertex(0));
        glVertex(f.vertex(1));
        glVertex(f.vertex(2));
      }
    }
  }
  glEnd();

  for (size_t i = 0, l = poly->faces.size(); i != l; ++i) {
    carve::poly::Face<3> &f = poly->faces[i];
    if (group == -1 || f.manifold_id == group) {
      if (f.nVertices() != 3) {
        drawFace(&poly->faces[i], cRGBA(r, g, b, a), false);
      }
    }
  }

  if (offset) {
    glDisable(GL_POLYGON_OFFSET_FILL);
  }
}

void drawEdges(carve::poly::Polyhedron *poly, double alpha, int group) {
  glBegin(GL_LINES);
  for (size_t i = 0, l = poly->edges.size(); i != l; ++i) {
    if (group == -1 || poly->edgeOnManifold(&poly->edges[i], group)) {
      const std::vector<const carve::poly::Polyhedron::face_t *> &ef = poly->connectivity.edge_to_face[i];
      if (std::find(ef.begin(), ef.end(), (carve::poly::Polyhedron::face_t *)NULL) != ef.end()) {
        glColor4f(1.0, 1.0, 0.0, alpha);
      } else if (ef.size() > 2) {
        glColor4f(0.0, 1.0, 1.0, alpha);
      } else {
        glColor4f(1.0, 0.0, 0.0, alpha);
      }
      glVertex(poly->edges[i].v1->v);
      glVertex(poly->edges[i].v2->v);
    }
  }
  glEnd();
}

void drawPolyhedronWireframe(carve::poly::Polyhedron *poly, bool normal, int group) {
  if (normal) {
    for (size_t i = 0, l = poly->faces.size(); i != l; ++i) {
      carve::poly::Face<3> &f = poly->faces[i];
      if (group == -1 || f.manifold_id == group) {
        drawFaceNormal(&f);
      }
    }
  }

  glDisable(GL_LIGHTING);
  glDepthMask(GL_FALSE);
  glDisable(GL_DEPTH_TEST);

  drawEdges(poly, 0.2, group);

  glEnable(GL_DEPTH_TEST);

  drawEdges(poly, 0.8, group);

  glDepthMask(GL_TRUE);
  glEnable(GL_LIGHTING);
}

void installDebugHooks() {
  if (carve::csg::intersect_debugEnabled()) {
    carve::csg::intersect_installDebugHooks(new DebugHooks());
  }
}
