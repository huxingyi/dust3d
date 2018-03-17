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

#include "write_ply.hpp"

#include <gloop/model/ply_format.hpp>
#include <gloop/model/obj_format.hpp>
#include <gloop/model/vtk_format.hpp>

#include <stdlib.h>
#include <stdio.h>
#include <iomanip>

#ifndef WIN32
#  include <stdint.h>
#endif

namespace {
  struct vertex_base : public gloop::stream::null_writer {
    virtual const carve::geom3d::Vector &curr() const =0;
  };


  template<typename container_t>
  struct vertex : public vertex_base {
    const container_t &cnt;
    int i;
    vertex(const container_t &_cnt) : cnt(_cnt), i(-1) { }
    virtual void next() { ++i; }
    virtual int length() { return cnt.size(); }
    virtual const carve::geom3d::Vector &curr() const {  return cnt[i].v; }
  };
  typedef vertex<std::vector<carve::point::Vertex> > pointset_vertex;
  typedef vertex<std::vector<carve::poly::Vertex<3> > > poly_vertex;
  typedef vertex<std::vector<carve::line::Vertex> > line_vertex;


  template<int idx>
  struct vertex_component : public gloop::stream::writer<double> {
    vertex_base &r;
    vertex_component(vertex_base &_r) : r(_r) { }
    virtual double value() { return r.curr().v[idx]; }
  };
  
  struct face : public gloop::stream::null_writer {
    const carve::poly::Polyhedron *poly;
    int i;
    face(const carve::poly::Polyhedron *_poly) : poly(_poly), i(-1) { }
    virtual void next() { ++i; }
    virtual int length() { return poly->faces.size(); }
    const carve::poly::Face<3> *curr() const { return &poly->faces[i]; }
  };


  struct face_idx : public gloop::stream::writer<size_t> {
    face &r;
    const carve::poly::Face<3> *f;
    size_t i;
    gloop::stream::Type data_type;
    int max_length;

    face_idx(face &_r, gloop::stream::Type _data_type, int _max_length) :
        r(_r), f(NULL), i(0), data_type(_data_type), max_length(_max_length) {
    }
    virtual void begin() { f = r.curr(); i = 0; }
    virtual int length() { return f->nVertices(); }

    virtual bool isList() { return true; }
    virtual gloop::stream::Type dataType() { return data_type; }
    virtual int maxLength() { return max_length; }

    virtual size_t value() { return static_cast<const carve::poly::Polyhedron *>(f->owner)->vertexToIndex_fast(f->vertex(i++)); }
  };


  struct lineset : public gloop::stream::null_writer {
    const carve::line::PolylineSet *polyline;
    carve::line::PolylineSet::const_line_iter c;
    carve::line::PolylineSet::const_line_iter n;
    lineset(const carve::line::PolylineSet *_polyline) : polyline(_polyline) { n = polyline->lines.begin(); }
    virtual void next() { c = n; ++n; }
    virtual int length() { return polyline->lines.size(); }
    const carve::line::Polyline *curr() const { return *c; }
  };


  struct line_closed : public gloop::stream::writer<bool> {
    lineset &ls;
    line_closed(lineset &_ls) : ls(_ls) { }
    virtual gloop::stream::Type dataType() { return gloop::stream::U8; }
    virtual bool value() { return ls.curr()->isClosed(); }
  };


  struct line_vi : public gloop::stream::writer<size_t> {
    lineset &ls;
    const carve::line::Polyline *l;
    size_t i;
    gloop::stream::Type data_type;
    int max_length;

    line_vi(lineset &_ls, gloop::stream::Type _data_type, int _max_length) :
        ls(_ls), l(NULL), i(0), data_type(_data_type), max_length(_max_length) {
    }
    virtual bool isList() { return true; }
    virtual gloop::stream::Type dataType() { return data_type; }
    virtual int maxLength() { return max_length; }
    virtual void begin() { l = ls.curr(); i = 0; }
    virtual int length() { return l->vertexCount(); }
    virtual size_t value() { return ls.polyline->vertexToIndex_fast(l->vertex(i++)); }
  };


  void setup(gloop::stream::model_writer &file, const carve::poly::Polyhedron *poly) {
    size_t face_max = 0;
    for (size_t i = 0; i < poly->faces.size(); ++i) face_max = std::max(face_max, poly->faces[i].nVertices());

    file.newBlock("polyhedron");
    poly_vertex *vi = new poly_vertex(poly->vertices);
    file.addWriter("polyhedron.vertex", vi);
    file.addWriter("polyhedron.vertex.x", new vertex_component<0>(*vi));
    file.addWriter("polyhedron.vertex.y", new vertex_component<1>(*vi));
    file.addWriter("polyhedron.vertex.z", new vertex_component<2>(*vi));

    face *fi = new face(poly);
    file.addWriter("polyhedron.face", fi);
    file.addWriter("polyhedron.face.vertex_indices",
                   new face_idx(*fi,
                                gloop::stream::smallest_type(poly->vertices.size()),
                                face_max));

  }

  void setup(gloop::stream::model_writer &file, const carve::line::PolylineSet *lines) {
    size_t line_max = 0;
    for (std::list<carve::line::Polyline *>::const_iterator
           i = lines->lines.begin(),
           e = lines->lines.end();
         i != e;
         ++i) {
      line_max = std::max(line_max, (*i)->vertexCount());
    }

    file.newBlock("polyline");
    line_vertex *vi = new line_vertex(lines->vertices);
    file.addWriter("polyline.vertex", vi);
    file.addWriter("polyline.vertex.x", new vertex_component<0>(*vi));
    file.addWriter("polyline.vertex.y", new vertex_component<1>(*vi));
    file.addWriter("polyline.vertex.z", new vertex_component<2>(*vi));

    lineset *pi = new lineset(lines);
    file.addWriter("polyline.polyline", pi);
    file.addWriter("polyline.polyline.closed", new line_closed(*pi));
    file.addWriter("polyline.polyline.vertex_indices",
                   new line_vi(*pi,
                               gloop::stream::smallest_type(lines->vertices.size()),
                               line_max));
  }

  void setup(gloop::stream::model_writer &file, const carve::point::PointSet *points) {
    file.newBlock("pointset");
    pointset_vertex *vi = new pointset_vertex(points->vertices);
    file.addWriter("pointset.vertex", vi);
    file.addWriter("pointset.vertex.x", new vertex_component<0>(*vi));
    file.addWriter("pointset.vertex.y", new vertex_component<1>(*vi));
    file.addWriter("pointset.vertex.z", new vertex_component<2>(*vi));
  }

}

void writePLY(std::ostream &out, const carve::poly::Polyhedron *poly, bool ascii) {
  gloop::ply::PlyWriter file(!ascii, false);
  if (ascii) out << std::setprecision(30);
  setup(file, poly);
  file.write(out);
}

void writePLY(std::string &out_file, const carve::poly::Polyhedron *poly, bool ascii) {
  std::ofstream out(out_file.c_str(), std::ios_base::binary);
  if (!out.is_open()) { std::cerr << "File '" <<  out_file << "' could not be opened." << std::endl; return; }
  writePLY(out, poly, ascii);
}

void writePLY(std::ostream &out, const carve::line::PolylineSet *lines, bool ascii) {
  gloop::ply::PlyWriter file(!ascii, false);

  if (ascii) out << std::setprecision(30);
  setup(file, lines);
  file.write(out);
}

void writePLY(std::string &out_file, const carve::line::PolylineSet *lines, bool ascii) {
  std::ofstream out(out_file.c_str(), std::ios_base::binary);
  if (!out.is_open()) { std::cerr << "File '" <<  out_file << "' could not be opened." << std::endl; return; }
  writePLY(out, lines, ascii);
}

void writePLY(std::ostream &out, const carve::point::PointSet *points, bool ascii) {
  gloop::ply::PlyWriter file(!ascii, false);

  if (ascii) out << std::setprecision(30);
  setup(file, points);
  file.write(out);
}

void writePLY(std::string &out_file, const carve::point::PointSet *points, bool ascii) {
  std::ofstream out(out_file.c_str(), std::ios_base::binary);
  if (!out.is_open()) { std::cerr << "File '" <<  out_file << "' could not be opened." << std::endl; return; }
  writePLY(out, points, ascii);
}



void writeOBJ(std::ostream &out, const carve::poly::Polyhedron *poly) {
  gloop::obj::ObjWriter file;
  setup(file, poly);
  file.write(out);
}

void writeOBJ(std::string &out_file, const carve::poly::Polyhedron *poly) {
  std::ofstream out(out_file.c_str(), std::ios_base::binary);
  if (!out.is_open()) { std::cerr << "File '" <<  out_file << "' could not be opened." << std::endl; return; }
  writeOBJ(out, poly);
}

void writeOBJ(std::ostream &out, const carve::line::PolylineSet *lines) {
  gloop::obj::ObjWriter file;
  setup(file, lines);
  file.write(out);
}

void writeOBJ(std::string &out_file, const carve::line::PolylineSet *lines, bool ascii) {
  std::ofstream out(out_file.c_str(), std::ios_base::binary);
  if (!out.is_open()) { std::cerr << "File '" <<  out_file << "' could not be opened." << std::endl; return; }
  writeOBJ(out, lines);
}



void writeVTK(std::ostream &out, const carve::poly::Polyhedron *poly) {
  gloop::vtk::VtkWriter file(gloop::vtk::VtkWriter::ASCII);
  setup(file, poly);
  file.write(out);
}

void writeVTK(std::string &out_file, const carve::poly::Polyhedron *poly) {
  std::ofstream out(out_file.c_str(), std::ios_base::binary);
  if (!out.is_open()) { std::cerr << "File '" <<  out_file << "' could not be opened." << std::endl; return; }
  writeVTK(out, poly);
}

void writeVTK(std::ostream &out, const carve::line::PolylineSet *lines) {
  gloop::vtk::VtkWriter file(gloop::vtk::VtkWriter::ASCII);
  setup(file, lines);
  file.write(out);
}

void writeVTK(std::string &out_file, const carve::line::PolylineSet *lines, bool ascii) {
  std::ofstream out(out_file.c_str(), std::ios_base::binary);
  if (!out.is_open()) { std::cerr << "File '" <<  out_file << "' could not be opened." << std::endl; return; }
  writeVTK(out, lines);
}

