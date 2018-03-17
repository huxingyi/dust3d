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
#include <carve/csg_triangulator.hpp>
#include <carve/poly.hpp>
#include <carve/geom3d.hpp>

// #include <surpac_tri_tri_intersection.hpp>

#include "opts.hpp"
#include "read_ply.hpp"
#include "write_ply.hpp"


#include <fstream>
#include <algorithm>
#include <string>
#include <utility>
#include <set>
#include <iostream>
#include <iomanip>

struct Options : public opt::Parser {
  bool ascii;
  bool obj;
  bool vtk;

  std::string file;

  virtual void optval(const std::string &o, const std::string &v) {
    if (o == "--binary"       || o == "-b") { ascii = false; return; }
    if (o == "--obj"          || o == "-O") { obj = true; return; }
    if (o == "--vtk"          || o == "-V") { vtk = true; return; }
    if (o == "--ascii"        || o == "-a") { ascii = true; return; }
  }

  virtual std::string usageStr() {
    return std::string ("Usage: ") + progname + std::string(" [options] expression");
  };

  virtual void arg(const std::string &a) {
    if (file == "") {
      file = a;
    }
  }

  virtual void help(std::ostream &out) {
    this->opt::Parser::help(out);
  }

  Options() {
    ascii = true;
    obj = false;
    vtk = false;
    file = "";

    option("binary",       'b', false, "Produce binary output.");
    option("ascii",        'a', false, "ASCII output (default).");
    option("obj",          'O', false, "Output in .obj format.");
    option("vtk",          'V', false, "Output in .vtk format.");
  }
};



static Options options;

static bool endswith(const std::string &a, const std::string &b) {
  if (a.size() < b.size()) return false;

  for (unsigned i = a.size(), j = b.size(); j; ) {
    if (tolower(a[--i]) != tolower(b[--j])) return false;
  }
  return true;
}

typedef carve::geom::vector<3> vec3;
typedef carve::geom::vector<2> vec2;

inline void add_to_bbox(vec3 &lo, vec3 &hi, const vec3 &p) {
  lo.x = std::min(lo.x, p.x); lo.y = std::min(lo.y, p.y); lo.z = std::min(lo.z, p.z);
  hi.x = std::max(hi.x, p.x); hi.y = std::max(hi.y, p.y); hi.z = std::max(hi.z, p.z);
}

template<typename iter_t>
void bbox(iter_t begin, iter_t end, vec3 &lo, vec3 &hi) {
  lo = hi = *begin++;
  for (; begin != end; ++begin) {
    add_to_bbox(lo, hi, *begin);
  }
}

vec3 extent(const vec3 tri[3]) {
  vec3 lo, hi;
  bbox(tri, tri+3, lo, hi);
  return hi - lo;
}

vec3 extent(const vec3 tri[3], const vec3 &p1) {
  vec3 lo, hi;
  bbox(tri, tri+3, lo, hi);
  add_to_bbox(lo, hi, p1);
  return hi - lo;
}

vec3 extent(const vec3 tri[3], const vec3 &p1, const vec3 &p2) {
  vec3 lo, hi;
  bbox(tri, tri+3, lo, hi);
  add_to_bbox(lo, hi, p1);
  add_to_bbox(lo, hi, p2);
  return hi - lo;
}

vec3 extent(const vec3 tri_a[3], const vec3 tri_b[3]) {
  vec3 lo, hi;
  bbox(tri_a, tri_a+3, lo, hi);
  add_to_bbox(lo, hi, tri_b[0]);
  add_to_bbox(lo, hi, tri_b[1]);
  add_to_bbox(lo, hi, tri_b[2]);
  return hi - lo;
}

bool shares_edge(const vec3 tri_a[3], const vec3 tri_b[3], size_t &ia, size_t &ib) {
  for (size_t a = 0; a < 3; ++a) {
    for (size_t b = 0; b < 3; ++b) {
      if (tri_a[a] == tri_b[(b+1)%3] && tri_a[(a+1)%3] == tri_b[b]) {
        ia = a; ib = b; return true;
      }
    }
  }
  return false;
}

std::string SIGN(double x) {
  if (x == 0.0) return "0";
  if (x < 0.0) return "-";
  return "+";
}

inline int fsign(double d) {
  if (d < 0.0) return -1;
  if (d == 0.0) return 0;
  return +1;
}

inline void count(int s[3], int c[3]) {
  std::fill(c, c+3, 0);
  c[s[0]+1]++; c[s[1]+1]++; c[s[2]+1]++;
}

vec3 normal(const vec3 tri[3]) {
  return carve::geom::cross(tri[1]-tri[0], tri[2]-tri[0]);
}

// assumes that p is coplanar with tri - compute by projecting away smallest axis
int triangle_point_intersection_2d(const vec3 tri[3], const vec3 &p) {
  if (p == tri[0] || p == tri[1] || p == tri[2]) {
    // shared vertex.
    return 1;
  }

  int a0 = carve::geom::largestAxis(normal(tri));
  int a1 = (a0+1) % 3;
  int a2 = (a0+2) % 3;

  vec2 ptri[3];
  vec2 pp;

  ptri[0] = carve::geom::select(tri[0], a1, a2);
  ptri[1] = carve::geom::select(tri[1], a1, a2);
  ptri[2] = carve::geom::select(tri[2], a1, a2);
  pp      = carve::geom::select(p, a1, a2);

  // what about vertex-edge intersections? currently count them as intersections.
  return carve::geom2d::pointIntersectsTriangle(pp, ptri) ? 3 : 0;
}

// assumes that p1 and p2 are coplanar with tri - compute by projecting away smallest axis
int triangle_line_intersection_2d(const vec3 tri[3], const vec3 &p1, const vec3 &p2) {
  int a0 = carve::geom::largestAxis(normal(tri));
  int a1 = (a0+1) % 3;
  int a2 = (a0+2) % 3;

  vec2 ptri[3];
  vec2 pp1;
  vec2 pp2;

  ptri[0] = carve::geom::select(tri[0], a1, a2);
  ptri[1] = carve::geom::select(tri[1], a1, a2);
  ptri[2] = carve::geom::select(tri[2], a1, a2);
  if (!carve::geom2d::isAnticlockwise(ptri)) std::swap(ptri[0], ptri[2]);

  pp1     = carve::geom::select(p1, a1, a2);
  pp2     = carve::geom::select(p2, a1, a2);

  int ia = std::find(ptri, ptri+3, pp1) - ptri;
  int ib = std::find(ptri, ptri+3, pp2) - ptri;

  if (ia == 3 && ib == 3) {
    // no shared vertices
    return carve::geom2d::lineIntersectsTriangle(pp1, pp2, ptri) ? 3 : 0;
  } else if (ib == 3) {
    // shared vertex (pp1)
    if (carve::geom2d::orient2d(ptri[(ia+2)%3], ptri[ia], pp2) < 0.0) return 1;
    if (carve::geom2d::orient2d(ptri[ia], ptri[(ia+1)%3], pp2) < 0.0) return 1;
    return 3;
  } else if (ia == 3) {
    // shared vertex (pp2)
    if (carve::geom2d::orient2d(ptri[(ib+2)%3], ptri[ib], pp1) < 0.0) return 1;
    if (carve::geom2d::orient2d(ptri[ib], ptri[(ib+1)%3], pp1) < 0.0) return 1;
    return 3;
  } else {
    // shared edge
    return 2;
  }
}

bool shared_vertex_intersection_test(const vec2 tri_a[3], int va, const vec2 tri_b[3], int vb) {
  // if two triangles share a vertex, then they additionally intersect if the two wedges formed by the two triangles at the shared vertex overlap.
  int van = (va+1)%3, vap = (va+2)%3; 
  int vbn = (vb+1)%3, vbp = (vb+2)%3; 

  if (carve::geom2d::orient2d(tri_a[vap], tri_a[va],  tri_b[vbp]) >= 0 &&
      carve::geom2d::orient2d(tri_a[va],  tri_a[van], tri_b[vbp]) >= 0) return true;
  if (carve::geom2d::orient2d(tri_a[vap], tri_a[va],  tri_b[vbn]) >= 0 &&
      carve::geom2d::orient2d(tri_a[va],  tri_a[van], tri_b[vbn]) >= 0) return true;

  if (carve::geom2d::orient2d(tri_b[vbp], tri_b[vb],  tri_a[vap]) >= 0 &&
      carve::geom2d::orient2d(tri_b[vb],  tri_b[vbn], tri_a[vap]) >= 0) return true;
  if (carve::geom2d::orient2d(tri_b[vbp], tri_b[vb],  tri_a[van]) >= 0 &&
      carve::geom2d::orient2d(tri_b[vb],  tri_b[vbn], tri_a[van]) >= 0) return true;

  return false;
}

bool shared_edge_intersection_test(const vec2 &l1, const vec2 &l2, const vec2 &p1, const vec2 &p2) {
  // if two triangles share an edge, then they additionally intersect if the unshared vertices lie on the same side as the edge.
  return fsign(carve::geom2d::orient2d(l1, l2, p1)) * fsign(carve::geom2d::orient2d(l1, l2, p2)) != -1;
}

// assumes that tri_a is coplanar with tri_b - compute by projecting away smallest axis
int triangle_triangle_intersection_2d(const vec3 tri_a[3], const vec3 tri_b[3]) {
  int a0 = carve::geom::largestAxis(normal(tri_a));
  int a1 = (a0+1) % 3;
  int a2 = (a0+2) % 3;

  vec2 ptri_a[3], ptri_b[3];

  ptri_a[0] = carve::geom::select(tri_a[0], a1, a2);
  ptri_a[1] = carve::geom::select(tri_a[1], a1, a2);
  ptri_a[2] = carve::geom::select(tri_a[2], a1, a2);
  if (!carve::geom2d::isAnticlockwise(ptri_a)) std::swap(ptri_a[0], ptri_a[2]);

  ptri_b[0] = carve::geom::select(tri_b[0], a1, a2);
  ptri_b[1] = carve::geom::select(tri_b[1], a1, a2);
  ptri_b[2] = carve::geom::select(tri_b[2], a1, a2);
  if (!carve::geom2d::isAnticlockwise(ptri_b)) std::swap(ptri_b[0], ptri_b[2]);

  int ia = std::find(ptri_b, ptri_b+3, ptri_a[0]) - ptri_b;
  int ib = std::find(ptri_b, ptri_b+3, ptri_a[1]) - ptri_b;
  int ic = std::find(ptri_b, ptri_b+3, ptri_a[2]) - ptri_b;

  if (ia == 3 && ib == 3 && ic == 3) {
    // no shared vertices
    return carve::geom2d::triangleIntersectsTriangle(ptri_b, ptri_a) ? 3 : 0;
  } else if (ib == 3 && ic == 3) {
    // shared vertex ia.
    return shared_vertex_intersection_test(ptri_a, 0, ptri_b, ia) ? 3 : 1;
  } else if (ia == 3 && ic == 3) {
    // shared vertex ib.
    return shared_vertex_intersection_test(ptri_a, 1, ptri_b, ib) ? 3 : 1;
  } else if (ia == 3 && ib == 3) {
    // shared vertex ic.
    return shared_vertex_intersection_test(ptri_a, 2, ptri_b, ic) ? 3 : 1;
  } else if (ic == 3) {
    // shared edge ia-ib.
    return shared_edge_intersection_test(ptri_a[0], ptri_a[1], ptri_a[2], ptri_b[3-ia-ib]) ? 3 : 2;
  } else if (ia == 3) {
    // shared edge ib-ic.
    return shared_edge_intersection_test(ptri_a[1], ptri_a[2], ptri_a[0], ptri_b[3-ib-ic]) ? 3 : 2;
  } else if (ib == 3) {
    // shared edge ic-ia.
    return shared_edge_intersection_test(ptri_a[2], ptri_a[0], ptri_a[1], ptri_b[3-ic-ia]) ? 3 : 2;
  } else {
    // duplicated triangle.
    return 3;
  }
}

bool line_segment_triangle_test(const vec3 tri[3], const vec3 &a, const vec3 &b) {
  // XXX this test is correct with exact predicates, but fails for nearly-coplanar inputs with inexact predicates. a 2d projection test would be useful in this case.
  int o[3];
  int c[3];

  o[0] = fsign(carve::geom3d::orient3d(tri[0], tri[1], a, b));
  o[1] = fsign(carve::geom3d::orient3d(tri[1], tri[2], a, b));
  o[2] = fsign(carve::geom3d::orient3d(tri[2], tri[0], a, b));
  count(o, c);

  // count(0) == 0 -> face intersection?  requires same result for other three tests
  // count(0) == 1 -> edge intersection?  requires same result for other two tests
  // count(0) == 2 -> vertex intersection implies count(-1) == 0 or count(+1) == 0
  return (c[0] == 0 || c[2] == 0);
}

template<typename t>
t max3(const t &a, const t &b, const t &c) {
  return std::max(std::max(a,b),c);
}

// 0 = not intersecting
// 1 = shared vertex
// 2 = shared edge
// 3 = intersecting
int a_intersects_b(const vec3 tri_a[3], double da[3], const vec3 tri_b[3]) {
  // called with non-coplanar triangles the sign of elements of da determine which side of tri_b each vertex of tri_a is on.

  int s[3]; s[0] = fsign(da[0]); s[1] = fsign(da[1]); s[2] = fsign(da[2]);
  int c[3];
  count(s, c);

  switch (c[1]) {
    case 0: {
      // no coplanar points
      if (s[0] != s[1] && line_segment_triangle_test(tri_b, tri_a[0], tri_a[1])) return 3;
      if (s[1] != s[2] && line_segment_triangle_test(tri_b, tri_a[1], tri_a[2])) return 3;
      if (s[2] != s[0] && line_segment_triangle_test(tri_b, tri_a[2], tri_a[0])) return 3;
      return 0;
    }
    case 1: {
      // 1 coplanar point.
      size_t zi = std::find(s, s+3, 0) - s;
      if (c[0] == 1 && c[2] == 1) {
        // test for intersection of crossing edge.
        if (line_segment_triangle_test(tri_b, tri_a[(zi+1)%3], tri_a[(zi+2)%3])) return 3;
      }
      // test point against triangle.
      return triangle_point_intersection_2d(tri_b, tri_a[zi]);
    }
    case 2: {
      // 2 coplanar points.
      // test line segment against triangle.
      size_t zi = std::find(s, s+3, 0) - s;
      size_t zi2 = std::find(s+zi+1, s+3, 0) - s;
      return triangle_line_intersection_2d(tri_b, tri_a[zi], tri_a[zi2]);
    }
    case 3: {
      // 3 coplanar points.
      // coplanar triangle.
      return triangle_triangle_intersection_2d(tri_b, tri_a);
    }
  }

  return false; // not reached.
}

bool bounding_box_overlap(const vec3 tri_a[3], const vec3 tri_b[3]) {
  vec3 a_lo, a_hi;
  vec3 b_lo, b_hi;

  bbox(tri_a, tri_a+3, a_lo, a_hi);
  bbox(tri_b, tri_b+3, b_lo, b_hi);

  // check bounding boxes.
  for (size_t i = 0; i < 3; ++i) {
    if (a_hi.v[i] < b_lo.v[i] || b_hi.v[i] < a_lo.v[i]) {
      return false;
    }
  }
  return true;
}

// 0 = not intersecting
// 1 = shared vertex
// 2 = shared edge
// 3 = intersecting
int tripair_intersection(const vec3 tri_a[3], const vec3 tri_b[3]) {
  if (!bounding_box_overlap(tri_a, tri_b)) {
    return 0;
  }

  {
    size_t ia, ib;
    if (shares_edge(tri_a, tri_b, ia, ib)) {
      if (carve::geom3d::orient3d(tri_a[0], tri_a[1], tri_a[2], tri_b[(ib+2)%3]) != 0.0) {
        return 2;
      }

      // triangles are coplanar, and share an edge. 2d test to determine edge sharing vs. intersecting.
      // this case also catches identical triangles (possibly with reversed winding) and returns intersecting if they are not degenerate, and shared edge if they are.
      int a0 = carve::geom::largestAxis(normal(tri_a));
      int a1 = (a0+1) % 3;
      int a2 = (a0+2) % 3;

      CARVE_ASSERT(tri_a[ia] == tri_b[(ib+1)%3]);
      CARVE_ASSERT(tri_a[(ia+1)%3] == tri_b[ib]);

      vec2 ea = carve::geom::select(tri_a[ia], a1, a2);
      vec2 eb = carve::geom::select(tri_a[(ia+1)%3], a1, a2);

      int sa = fsign(carve::geom2d::orient2d(ea, eb, carve::geom::select(tri_a[(ia+2)%3], a1, a2)));
      int sb = fsign(carve::geom2d::orient2d(ea, eb, carve::geom::select(tri_b[(ib+2)%3], a1, a2)));

      return (sa * sb == -1) ? 2 : 3;
    }
  }

  // triangles share at most one vertex.
  double da[3], db[3];
  double lo, hi;

  for (size_t i = 0; i < 3; ++i) {
    db[i] = carve::geom3d::orient3d(tri_a[0], tri_a[1], tri_a[2], tri_b[i]);
  }
  // test for b on one side of the plane containing a
  lo = *std::min_element(db, db+3);
  hi = *std::max_element(db, db+3);
  if (lo > 0 || hi < 0) {
    return 0;
  }

  for (size_t i = 0; i < 3; ++i) {
    da[i] = carve::geom3d::orient3d(tri_b[0], tri_b[1], tri_b[2], tri_a[i]);
  }
  // test for a on one side of the plane containing b
  lo = *std::min_element(da, da+3);
  hi = *std::max_element(da, da+3);
  if (lo > 0 || hi < 0) {
    return 0;
  }

  int t1 = a_intersects_b(tri_a, da, tri_b);
  int t2 = a_intersects_b(tri_b, db, tri_a);

  return std::max(t1, t2);
}

int main(int argc, char **argv) {
  options.parse(argc, argv);
  carve::poly::Polyhedron *poly;

  if (options.file == "-") {
    poly = readPLY(std::cin);
  } else if (endswith(options.file, ".ply")) {
    poly = readPLY(options.file);
  } else if (endswith(options.file, ".vtk")) {
    poly = readVTK(options.file);
  } else if (endswith(options.file, ".obj")) {
    poly = readOBJ(options.file);
  }

  if (poly == NULL) {
   //  std::cerr << "failed to load polyhedron" << std::endl;
    exit(1);
  }

 //  std::cerr << "poly aabb = " << poly->aabb << std::endl;

  for (size_t f = 0; f < poly->faces.size(); ++f) {
    std::vector<const carve::poly::Polyhedron::face_t *> near_faces;
    poly->findFacesNear(poly->faces[f].aabb, near_faces);
    const carve::poly::Polyhedron::face_t *fa = &poly->faces[f];
    vec3 tri_a[3]; tri_a[0] = fa->vertex(0)->v; tri_a[1] = fa->vertex(1)->v; tri_a[2] = fa->vertex(2)->v;

    for (size_t f2 = 0; f2 < near_faces.size(); ++f2) {
      const carve::poly::Polyhedron::face_t *fb = near_faces[f2];
      if (fa >= fb) continue;
      if (fa->aabb.intersects(fb->aabb)) {
        if (fa->nVertices() == 3 && fb->nVertices() == 3) {
          vec3 tri_b[3]; tri_b[0] = fb->vertex(0)->v; tri_b[1] = fb->vertex(1)->v; tri_b[2] = fb->vertex(2)->v;

          switch (tripair_intersection(tri_a, tri_b)) {
            case 0:
              break;
            case 1:
              break;
            case 2:
              break;
            case 3: {
              std::cerr << "intersection: " << poly->faceToIndex_fast(fa) << " - " << poly->faceToIndex_fast(fb) << std::endl;
              static int c = 0;
              std::ostringstream fn;
              fn << "intersection-" << c++ << ".ply";
              std::cerr << fn.str().c_str() << std::endl;
              std::ofstream outf(fn.str().c_str());
              outf << "\
ply\n\
format ascii 1.0\n\
element vertex 6\n\
property double x\n\
property double y\n\
property double z\n\
element face 2\n\
property list uchar uchar vertex_indices\n\
end_header\n";
              outf << std::setprecision(30);
              outf << tri_a[0].x << " " << tri_a[0].y << " " << tri_a[0].z << "\n";
              outf << tri_a[1].x << " " << tri_a[1].y << " " << tri_a[1].z << "\n";
              outf << tri_a[2].x << " " << tri_a[2].y << " " << tri_a[2].z << "\n";
              outf << tri_b[0].x << " " << tri_b[0].y << " " << tri_b[0].z << "\n";
              outf << tri_b[1].x << " " << tri_b[1].y << " " << tri_b[1].z << "\n";
              outf << tri_b[2].x << " " << tri_b[2].y << " " << tri_b[2].z << "\n";
              outf << "\
3 0 1 2\n\
3 5 4 3\n";
              break;
            }
          }
        }
      }
    }
  }

  return 0;
}
