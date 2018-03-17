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

#include <carve/geom2d.hpp>
#include <carve/poly.hpp>
#include <carve/input.hpp>
#include <carve/triangulator.hpp>
#include <carve/matrix.hpp>

#include <fstream>
#include <sstream>

#include "write_ply.hpp"

#include <iostream>

template<unsigned ndim>
carve::geom::vector<ndim> lerp(
    double t,
    const carve::geom::vector<ndim> &p1,
    const carve::geom::vector<ndim> &p2) {
  return (1-t)*p1 + t*p2;
}

template<unsigned ndim>
class cubic_bezier {

public:
  typedef carve::geom::vector<ndim> vec_t;
  typedef cubic_bezier<ndim> cubic_bezier_t;

  vec_t p[4];

  cubic_bezier() {
  }

  cubic_bezier(const vec_t &p1, const vec_t &p2, const vec_t &p3, const vec_t &p4) {
    p[0] = p1; p[1] = p2; p[2] = p3; p[3] = p4;
  }

  template<typename iter_t>
  cubic_bezier(iter_t begin, iter_t end) {
    if (std::distance(begin, end) != 4) throw std::runtime_error("failed");
    std::copy(begin, end, p);
  }

  vec_t eval(double t) const {
    double u = 1-t;
    return u*u*u*p[0] + 3*t*u*u*p[1] + 3*t*t*u*p[2] + t*t*t*p[3];
  }

  double flatness() const {
    carve::geom::ray<ndim> ray = carve::geom::rayThrough(p[0], p[3]);
    return sqrt(std::max(carve::geom::distance2(ray, p[1]), carve::geom::distance2(ray, p[2])));
  }

  void split(double t, cubic_bezier_t &a, cubic_bezier_t &b) const {
    a.p[0] = p[0]; b.p[3] = p[3];
    a.p[1] = lerp(t, p[0], p[1]);
    vec_t m = lerp(t, p[1], p[2]);
    b.p[2] = lerp(t, p[2], p[3]);
    a.p[2] = lerp(t, a.p[1], m);
    b.p[1] = lerp(t, m, b.p[2]);
    a.p[3] = b.p[0] = lerp(t, a.p[2], b.p[1]);
  }

  void approximateSimple(std::vector<vec_t> &out, double max_flatness) const {
    double f = flatness();
    if (f < max_flatness) {
      out.push_back(p[3]);
    } else {
      cubic_bezier a, b;
      split(0.5, a, b);
      a.approximateSimple(out, max_flatness);
      b.approximateSimple(out, max_flatness);
    }
  }

  void approximate(std::vector<vec_t> &out, double max_flatness, unsigned n_tests = 128) const {
    double f = flatness();
    if (f < max_flatness) {
      out.push_back(p[3]);
    } else {
      double best_t = .5;
      double best_f = f;
      cubic_bezier a, b;
      for (size_t i = 1; i < n_tests; ++i) {
        double t = double(i) / n_tests;
        split(t, a, b);
        double f = a.flatness() + b.flatness();
        if (f < best_f) {
          best_t = t;
          best_f = f;
        }
      }
      split(best_t, a, b);
      a.approximate(out, max_flatness, n_tests);
      b.approximate(out, max_flatness, n_tests);
    }
  }
};

template<unsigned ndim>
cubic_bezier<ndim> make_bezier(
    const carve::geom::vector<ndim> &p1,
    const carve::geom::vector<ndim> &p2,
    const carve::geom::vector<ndim> &p3,
    const carve::geom::vector<ndim> &p4) {
  return cubic_bezier<ndim>(p1, p2, p3, p4);
}

void consume(std::istream &in, char ch) {
  while (in.good()) {
    int c;
    c = in.peek();
    if (in.eof()) return;
    if (std::isspace(c)) { in.ignore(); continue; }
    if (c == ch) { in.ignore(); }
    return;
  }
}

void readVals(std::istream &in, std::vector<double> &vals) {
  while (in.good()) {
    int c;

    while (std::isspace(in.peek())) in.ignore();

    c = in.peek();
    if (in.eof()) break;

    if (c != '-' && c != '+' && c != '.' && !std::isdigit(c)) {
      break;
    }

    double v;
    in >> v;
    vals.push_back(v);

    while (std::isspace(in.peek())) in.ignore();
    c = in.peek();

    if (c == ',') {
      in.ignore();
      continue;
    } else if (c == '-' || c == '+' || c == '.' || std::isdigit(c)) {
      continue;
    } else {
      std::cerr << "remain c=" << (char)c << std::endl;
      break;
    }
  }
}

void add(std::vector<carve::geom2d::P2> &points, carve::geom2d::P2 p) {
  if (!points.size() || points.back() != p) {
    points.push_back(p);
  }
}

void add(std::vector<carve::geom2d::P2> &points, const cubic_bezier<2> &bezier) {
  bezier.approximate(points, .05);
}

void parsePath(std::istream &in, std::vector<std::vector<carve::geom2d::P2> > &paths) {
  carve::geom2d::P2 curr, curr_ctrl;

  std::string pathname;
  // while (in.good() && !std::isspace(in.peek())) pathname.push_back(in.get());
  in >> pathname;
  std::cerr << "parsing: [" << pathname << "]" << std::endl;

  std::vector<double> vals;
  
  std::vector<carve::geom2d::P2> points;
  while (in.good()) {
    char c;

    in >> c;
    if (in.eof()) break;
    if (std::isspace(c)) continue;
    std::cerr << "[" << c << "]";

    vals.clear();
    switch (c) {
      case 'M': {
        readVals(in, vals);
        points.clear();
        for (unsigned i = 0; i < vals.size(); i += 2) {
          curr = carve::geom::VECTOR(vals[i], vals[i+1]);
          add(points, curr);
        }
        curr_ctrl = curr;
        break;
      }
      case 'm': {
        readVals(in, vals);
        points.clear();
        curr = carve::geom::VECTOR(0.0, 0.0);
        for (unsigned i = 0; i < vals.size(); i += 2) {
          curr += carve::geom::VECTOR(vals[i], vals[i+1]);
          add(points, curr);
        }
        curr_ctrl = curr;
        break;
      }
      case 'L': {
        readVals(in, vals);
        for (unsigned i = 0; i < vals.size(); i += 2) {
          curr = carve::geom::VECTOR(vals[i], vals[i+1]);
          add(points, curr);
        }
        curr_ctrl = curr;
        break;
      }
      case 'l': {
        readVals(in, vals);
        for (unsigned i = 0; i < vals.size(); i += 2) {
          curr += carve::geom::VECTOR(vals[i], vals[i+1]);
          add(points, curr);
        }
        curr_ctrl = curr;
        break;
      }
      case 'H': {
        readVals(in, vals);
        for (unsigned i = 0; i < vals.size(); ++i) {
          curr.x = vals[i];
          add(points, curr);
        }
        curr_ctrl = curr;
        break;
      }
      case 'h': {
        readVals(in, vals);
        for (unsigned i = 0; i < vals.size(); ++i) {
          curr.x += vals[i];
          add(points, curr);
        }
        curr_ctrl = curr;
        break;
      }
      case 'V': {
        readVals(in, vals);
        for (unsigned i = 0; i < vals.size(); ++i) {
          curr.y = vals[i];
          add(points, curr);
        }
        curr_ctrl = curr;
        break;
      }
      case 'v': {
        readVals(in, vals);
        for (unsigned i = 0; i < vals.size(); ++i) {
          curr.y += vals[i];
          add(points, curr);
        }
        curr_ctrl = curr;
        break;
      }
      case 'S': {
        readVals(in, vals);
        for (unsigned i = 0; i < vals.size(); i += 4) {
          carve::geom2d::P2 c1 = curr - (curr_ctrl - curr);
          carve::geom2d::P2 c2 = carve::geom::VECTOR(vals[i+0], vals[i+1]);
          carve::geom2d::P2 p2 = carve::geom::VECTOR(vals[i+2], vals[i+3]);
          add(points, make_bezier(curr, c1, c2, p2));
          curr_ctrl = c2;
          curr = p2;
        }
        break;
      }
      case 's': {
        readVals(in, vals);
        for (unsigned i = 0; i < vals.size(); i += 4) {
          carve::geom2d::P2 c1 = curr - (curr_ctrl - curr);
          carve::geom2d::P2 c2 = curr + carve::geom::VECTOR(vals[i+0], vals[i+1]);
          carve::geom2d::P2 p2 = curr + carve::geom::VECTOR(vals[i+2], vals[i+3]);
          add(points, make_bezier(curr, c1, c2, p2));
          curr_ctrl = c2;
          curr = p2;
        }
        break;
      }
      case 'C': {
        readVals(in, vals);
        for (unsigned i = 0; i < vals.size(); i += 6) {
          carve::geom2d::P2 c1 = carve::geom::VECTOR(vals[i+0], vals[i+1]);
          carve::geom2d::P2 c2 = carve::geom::VECTOR(vals[i+2], vals[i+3]);
          carve::geom2d::P2 p2 = carve::geom::VECTOR(vals[i+4], vals[i+5]);
          add(points, make_bezier(curr, c1, c2, p2));
          curr_ctrl = c2;
          curr = p2;
        }
        break;
      }
      case 'c': {
        readVals(in, vals);
        for (unsigned i = 0; i < vals.size(); i += 6) {
          carve::geom2d::P2 c1 = curr + carve::geom::VECTOR(vals[i+0], vals[i+1]);
          carve::geom2d::P2 c2 = curr + carve::geom::VECTOR(vals[i+2], vals[i+3]);
          carve::geom2d::P2 p2 = curr + carve::geom::VECTOR(vals[i+4], vals[i+5]);
          add(points, make_bezier(curr, c1, c2, p2));
          curr_ctrl = c2;
          curr = p2;
        }
        break;
      }
      case 'Z':
      case 'z': {

        std::cerr << "path coords: " << std::endl;
        for (size_t i = 0; i < points.size(); ++i) {
          std::cerr << " " << i << ": " << points[i].x << "," << points[i].y;
        }
        if (points.back() == points.front()) points.pop_back();
        std::cerr << std::endl;
        paths.push_back(points);                     
        curr = curr_ctrl = carve::geom::VECTOR(0.0, 0.0);
        break;
      }
      default: {
        std::cerr << "unhandled path op: [" << c << "]" << std::endl;
        throw std::runtime_error("failed");
      }
    }
  }
}

void parsePath(std::string &s, std::vector<std::vector<carve::geom2d::P2> > &paths) {
  std::istringstream in(s);
  return parsePath(in, paths);
}

carve::poly::Polyhedron *extrude(const std::vector<std::vector<carve::geom2d::P2> > &paths, carve::geom3d::Vector dir) {
  carve::input::PolyhedronData data;

  for (size_t p = 0; p < paths.size(); ++p) {
    const std::vector<carve::geom2d::P2> &path = paths[p];
    const unsigned N = path.size();
    std::cerr << "N=" << N << std::endl;

    std::vector<unsigned> fwd, rev;
    fwd.reserve(N);
    rev.reserve(N);

    std::map<carve::geom3d::Vector, size_t> vert_idx;
    std::set<std::pair<size_t, size_t> > edges;
        
    for (size_t i = 0; i < path.size(); ++i) {
      carve::geom3d::Vector v;
      std::map<carve::geom3d::Vector, size_t>::iterator j;

      v = carve::geom::VECTOR(path[i].x, -path[i].y, 0.0);
      j = vert_idx.find(v);
      if (j == vert_idx.end()) {
        data.addVertex(v);
        fwd.push_back(vert_idx[v] = data.getVertexCount()-1);
      } else {
        fwd.push_back((*j).second);
      }

      v = carve::geom::VECTOR(path[i].x, -path[i].y, 0.0) + dir;
      j = vert_idx.find(v);
      if (j == vert_idx.end()) {
        data.addVertex(v);
        rev.push_back(vert_idx[v] = data.getVertexCount()-1);
      } else {
        rev.push_back((*j).second);
      }
    }

    data.addFace(fwd.begin(), fwd.end());
    data.addFace(rev.rbegin(), rev.rend());

    for (size_t i = 0; i < path.size()-1; ++i) {
      edges.insert(std::make_pair(fwd[i+1], fwd[i]));
    }
    edges.insert(std::make_pair(fwd[0], fwd[N-1]));

    for (size_t i = 0; i < path.size()-1; ++i) {
      if (edges.find(std::make_pair(fwd[i], fwd[i+1])) == edges.end()) {
        data.addFace(fwd[i+1], fwd[i], rev[i], rev[i+1]);
      }
    }
    if (edges.find(std::make_pair(fwd[N-1], fwd[0])) == edges.end()) {
      data.addFace(fwd[0], fwd[N-1], rev[N-1], rev[0]);
    }
  }

  return new carve::poly::Polyhedron(data.points, data.getFaceCount(), data.faceIndices);
}

int main(int argc, char **argv) {
  typedef std::vector<carve::geom2d::P2> loop_t;

  std::ifstream in(argv[1]);
  unsigned file_num = 0;
  while (in.good()) {
    std::string s;
    std::getline(in, s);
    if (in.eof()) break;
    std::vector<std::vector<carve::geom2d::P2> > paths;
    parsePath(s, paths);

    std::cerr << "paths.size()=" << paths.size() << std::endl;

    if (paths.size() > 1) {
      std::vector<std::vector<std::pair<size_t, size_t> > > result;
      std::vector<std::vector<carve::geom2d::P2> > merged;

      result = carve::triangulate::mergePolygonsAndHoles(paths);

      merged.resize(result.size());
      for (size_t i = 0; i < result.size(); ++i) {
        std::vector<std::pair<size_t, size_t> > &p = result[i];
        merged[i].reserve(p.size());
        for (size_t j = 0; j < p.size(); ++j) {
          merged[i].push_back(paths[p[j].first][p[j].second]);
        }
      }

      paths.swap(merged);
    }

    carve::poly::Polyhedron *p = extrude(paths, carve::geom::VECTOR(0,0,100));
    p->transform(carve::math::Matrix::TRANS(-p->aabb.pos));
    std::ostringstream outf;
    outf << "file_" << file_num++ << ".ply";
    std::ofstream out(outf.str().c_str());
    writePLY(out, p, false);
    delete p;
  }
  return 0;
}
