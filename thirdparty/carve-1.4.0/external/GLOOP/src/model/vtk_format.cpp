// Copyright (c) 2006, Tobias Sargeant
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
// Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the
// distribution.  The names of its contributors may be used to endorse
// or promote products derived from this software without specific
// prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.

#include <gloop/model/stream.hpp>
#include <gloop/model/vtk_format.hpp>
#include <gloop/stringfuncs.hpp>
#include <gloop/exceptions.hpp>
#include <gloop/vector.hpp>

#include <iterator>
#include <iostream>
#include <iomanip>
#include <stdio.h>

namespace {

  bool asUnsigned(const std::string &s, size_t &val) {
    char *c;
    val = strtoul(s.c_str(), &c, 10);
    if (c == s.c_str()) {
      return false;
    }
    return true;
  }

  bool readIndexBlock(std::istream &in, size_t cnt, std::vector<uint32_t> &blk, size_t &num) {
    blk.clear();
    blk.reserve(cnt);
    num = 0;

    while (blk.size() < cnt) {
      std::string s;
      std::getline(in, s);
      std::istringstream l(s);
      uint32_t n;

      l >> n;
      if (l.fail() || blk.size() + n + 1 > cnt) return false;

      blk.push_back(n);

      for (size_t i = 0; i < n; ++i) {
        uint32_t v;
        l >> v;
        blk.push_back(v);
      }

      if (l.fail()) return false;
      ++num;
    }
    return true;
  }

}

namespace gloop {
  namespace vtk {

    void VtkReader::emitPoints(const std::vector<point_t> &point_data, const std::string &base) {
      stream::reader_base *e_rd = findReader(base, "vertex");
      stream::reader_base *x_rd = findReader(base, "vertex", "x");
      stream::reader_base *y_rd = findReader(base, "vertex", "y");
      stream::reader_base *z_rd = findReader(base, "vertex", "z");
      if (e_rd) {
        e_rd->begin();
        e_rd->length(point_data.size());
      }
      // std::cerr << "emitting " << point_data.size() << " points" << std::endl;
      for (size_t i = 0; i < point_data.size(); ++i) {
        if (e_rd) e_rd->next();
        stream::send_scalar(x_rd, point_data[i].x);
        stream::send_scalar(y_rd, point_data[i].y);
        stream::send_scalar(z_rd, point_data[i].z);
      }
      if (e_rd) e_rd->end();
    }

    bool VtkReader::readASCII(std::istream &in) {
      std::string s;
      std::vector<std::string> v;
      std::vector<point_t> point_data;

      bool polyhedron_points_done = false;
      bool polyline_points_done = false;

      std::string dataset_type;

      while (in.good()) {
        std::getline(in, s);
        if (str::strip(s) == "") continue;

        if (str::startswith(s, "DATASET")) {
          dataset_type = str::split(s)[1];

          if (dataset_type == "STRUCTURED_POINTS") {
          } else if (dataset_type == "STRUCTURED_GRID") {
          } else if (dataset_type == "RECTILINEAR_GRID") {
          } else if (dataset_type == "UNSTRUCTURED_GRID") {
          } else if (dataset_type == "POLYDATA") {
          } else {
            std::cerr << "unhandled dataset type: [" << str::strip(s) << "]" << std::endl;
            return false;
          }

          point_data.clear();

        } else if (str::startswith(s, "POINTS")) {
          v = str::split(s);
          if (v.size() != 3) goto syntax_error;

          size_t n_points;
          if (!asUnsigned(v[1], n_points)) goto syntax_error;
          std::string data_type = v[2];
          point_data.resize(n_points);
          for (size_t i = 0; i < n_points; ++i) {
            std::getline(in, s);

            std::istringstream l(s);
            l >> point_data[i].x >> point_data[i].y >> point_data[i].z;
            if (l.fail()) goto syntax_error;
          }

          if (polyhedron_points_done) {
            stream::reader_base *p_rd = findReader("polyhedron");
            if (p_rd != NULL) p_rd->end();
            polyhedron_points_done = false;
          }
          if (polyline_points_done) {
            stream::reader_base *p_rd = findReader("polyline");
            if (p_rd != NULL) p_rd->end();
            polyline_points_done = false;
          }

        } else if (str::startswith(s, "VERTICES") ||
                   str::startswith(s, "LINES") || 
                   str::startswith(s, "POLYGONS") || 
                   str::startswith(s, "TRIANGLE_STRIPS")) {
          std::vector<uint32_t> blk;
          v = str::split(s);
          if (v.size() != 3) goto syntax_error;

          size_t n_vertices;
          if (!asUnsigned(v[1], n_vertices)) goto syntax_error;
          size_t n_vals, num;
          if (!asUnsigned(v[2], n_vals)) goto syntax_error;
          if (!readIndexBlock(in, n_vals, blk, num)) goto block_syntax_error;

          if (str::startswith(s, "VERTICES")) {
            // vertices aren't handled

          } else if (str::startswith(s, "LINES")) {
            if (!polyline_points_done) {
              stream::reader_base *p_rd = findReader("polyline");
              if (p_rd != NULL) p_rd->begin();
              emitPoints(point_data, "polyline");
              polyline_points_done = true;
            }
            stream::reader_base *e_rd = findReader("polyline", "polyline");
            stream::reader_base *v_rd = findReader("polyline", "vertex_indices");
            stream::reader_base *c_rd = findReader("polyline", "closed");

            if (e_rd) {
              e_rd->begin();
              e_rd->length(num);
            }

            size_t x = 0;
            for (size_t i = 0; i < num; ++i) {
              if (e_rd) {
                e_rd->next();
              }
              uint32_t n = blk[x++];
              uint32_t closed = blk[x] == blk[x + n - 1] ? 1 : 0;
              // std::copy(blk.begin() + x, blk.begin() + x + n - closed, std::ostream_iterator<uint32_t>(std::cerr, " ")); std::cerr << std::endl;
              stream::send_sequence(v_rd, n, blk.begin() + x, blk.begin() + x + n - closed);
              x += n;
              stream::send_scalar(c_rd, closed);
            }

            if (e_rd) {
              e_rd->end();
            }

          } else if (str::startswith(s, "POLYGONS") || str::startswith(s, "TRIANGLE_STRIPS")) {
            if (!polyhedron_points_done) {
              stream::reader_base *p_rd = findReader("polyhedron");
              if (p_rd != NULL) p_rd->begin();
              // std::cerr << "p_rd=" << p_rd << std::endl;
              emitPoints(point_data, "polyhedron");
              polyhedron_points_done = true;
            }

            std::string t;
            if (str::startswith(s, "POLYGONS")) {
              t = "face";
            } else {
              t = "tristrips";
            }
            stream::reader_base *e_rd = findReader("polyhedron", t);
            stream::reader_base *v_rd = findReader("polyhedron", t, "vertex_indices");
            // std::cerr << "e_rd=" << e_rd << std::endl;
            // std::cerr << "v_rd=" << v_rd << std::endl;

            if (e_rd) {
              e_rd->begin();
              e_rd->length(num);
            }

            size_t x = 0;
            for (size_t i = 0; i < num; ++i) {
              if (e_rd) {
                e_rd->next();
              }
              size_t n = blk[x++];
              // std::copy(blk.begin() + x, blk.begin() + x + n, std::ostream_iterator<uint32_t>(std::cerr, " ")); std::cerr << std::endl;
              stream::send_sequence(v_rd, n, blk.begin() + x, blk.begin() + x + n);
              x += n;
            }

            if (e_rd) {
              e_rd->end();
            }

          }

        } else {
          std::cerr << "unhandled line: [" << str::strip(s) << "]" << std::endl;
        }

      }
      if (polyhedron_points_done) {
        stream::reader_base *p_rd = findReader("polyhedron");
        if (p_rd != NULL) p_rd->end();
        polyhedron_points_done = false;
      }
      if (polyline_points_done) {
        stream::reader_base *p_rd = findReader("polyline");
        if (p_rd != NULL) p_rd->end();
        polyline_points_done = false;
      }

      return true;
    syntax_error:
      std::cerr << "syntax error: [" << str::strip(s) << "]" << std::endl;
      goto error;
    block_syntax_error:
      std::cerr << "syntax error in block following: [" << str::strip(s) << "]" << std::endl;
      goto error;
    error:

      if (polyhedron_points_done) {
        stream::reader_base *p_rd = findReader("polyhedron");
        if (p_rd != NULL) p_rd->fail();
        polyhedron_points_done = false;
      }
      if (polyline_points_done) {
        stream::reader_base *p_rd = findReader("polyline");
        if (p_rd != NULL) p_rd->fail();
        polyline_points_done = false;
      }
      return false;
    }

    bool VtkReader::readBinary(std::istream &in) {
      std::string s;
      std::getline(in, s);
	  return false;
    }

    bool VtkReader::readPlain(std::istream &in) {
      std::string s;
      std::getline(in, s); // data description.
      std::getline(in, s); // ASCII | BINARY
      if (str::startswith(s, "ASCII")) {
        return readASCII(in);
      } else if (str::startswith(s, "BINARY")) {
        return readBinary(in);
      } else {
        return false;
      }
    }

    bool VtkReader::readXML(std::istream &in) {
	  return false;
    }

    bool VtkReader::read(std::istream &in) {
      std::string s;
      std::getline(in, s);
      int ver_maj, ver_min;
      if (sscanf(s.c_str(), "# vtk DataFile Version %d.%d", &ver_maj, &ver_min) == 2) {
        return readPlain(in);
      } else if (s.substr(0, 5) == "<?xml") {
        return readXML(in);
        // try xml format.
      } else {
        return false;
        // not a vtk file
      }
    }

  }
}


namespace gloop {
  namespace vtk {

    bool VtkWriter::write(std::ostream &out) {
      out << std::setprecision(30);
      out << "# vtk DataFile Version 3.0" << std::endl;
      out << "Carve output" << std::endl;
      out << "ASCII" << std::endl;

      for (std::list<std::pair<std::string, stream::block_t> >::iterator i = blocks.begin(); i != blocks.end(); ++i) {
        std::string &blockname = (*i).first;
        stream::block_t &block = (*i).second;
        if (block.name == "polyhedron") {
          if (block.wt != NULL) block.wt->begin();

          {
            stream::named_element_t *elem = block.findElem("vertex");
            if (!elem || elem->wt == NULL) continue;
            stream::named_prop_t *px = elem->findProp("x");
            stream::named_prop_t *py = elem->findProp("y");
            stream::named_prop_t *pz = elem->findProp("z");
            if (!px || px->wt == NULL ||
                !py || py->wt == NULL ||
                !pz || pz->wt == NULL) continue;

            elem->wt->begin();
            size_t n_verts = elem->wt->length();
            out << "DATASET POLYDATA" << std::endl;
            out << "POINTS " << n_verts << " double" << std::endl;
            for (size_t i = 0; i < n_verts; ++i) {
              double x, y, z;
              elem->wt->next();
              px->wt->begin(); px->wt->next(); px->wt->_val(x); px->wt->end();
              py->wt->begin(); py->wt->next(); py->wt->_val(y); py->wt->end();
              pz->wt->begin(); pz->wt->next(); pz->wt->_val(z); pz->wt->end();
              out << x << " " << y << " " << z << std::endl;
            }
            elem->wt->end();
          }

          {
            std::vector<uint32_t> vv;
            stream::named_element_t *elem = block.findElem("face");
            if (!elem || elem->wt == NULL) continue;
            stream::named_prop_t *v = elem->findProp("vertex_indices");

            elem->wt->begin();
            size_t n_faces = elem->wt->length();
            for (size_t i = 0; i < n_faces; ++i) {
              uint32_t nv, vi;
              elem->wt->next();
              v->wt->begin();
              nv = v->wt->length();
              vv.push_back(nv);
              vv.reserve(vv.size() + nv);
              for (size_t j = 0; j < nv; ++j) {
                v->wt->next();
                v->wt->_val(vi);
                vv.push_back(vi);
              }
              v->wt->end();
            }
            elem->wt->end();
            out << "POLYGONS " << n_faces << " " << vv.size() << std::endl;
            for (size_t i = 0; i < vv.size(); ) {
              out << vv[i];
              for (size_t j = 0; j < vv[i]; ++j) {
                out << " " << vv[i+1+j];
              }
              out << std::endl;
              i += vv[i]+1;
            }
          }

          if (block.wt != NULL) block.wt->end();

        } else if (block.name == "polyline") {
          if (block.wt != NULL) block.wt->begin();

          {
            stream::named_element_t *elem = block.findElem("vertex");
            if (!elem || elem->wt == NULL) continue;
            stream::named_prop_t *px = elem->findProp("x");
            stream::named_prop_t *py = elem->findProp("y");
            stream::named_prop_t *pz = elem->findProp("z");
            if (!px || px->wt == NULL ||
                !py || py->wt == NULL ||
                !pz || pz->wt == NULL) continue;

            elem->wt->begin();
            size_t n_verts = elem->wt->length();
            out << "DATASET POLYDATA" << std::endl;
            out << "POINTS " << n_verts << " double" << std::endl;
            for (size_t i = 0; i < n_verts; ++i) {
              double x, y, z;
              elem->wt->next();
              px->wt->begin(); px->wt->next(); px->wt->_val(x); px->wt->end();
              py->wt->begin(); py->wt->next(); py->wt->_val(y); py->wt->end();
              pz->wt->begin(); pz->wt->next(); pz->wt->_val(z); pz->wt->end();
              out << x << " " << y << " " << z << std::endl;
            }
            elem->wt->end();
          }

          {
            std::vector<uint32_t> vv;
            stream::named_element_t *elem = block.findElem("polyline");
            if (!elem || elem->wt == NULL) continue;
            stream::named_prop_t *v = elem->findProp("vertex_indices");
            stream::named_prop_t *c = elem->findProp("closed");

            elem->wt->begin();
            size_t n_lines = elem->wt->length();
            for (size_t i = 0; i < n_lines; ++i) {
              uint32_t nv, vi, vi_first;
              uint32_t closed;
              elem->wt->next();

              c->wt->begin();
              c->wt->next();
              c->wt->_val(closed);
              closed = closed ? 1 : 0;
              c->wt->end();

              v->wt->begin();
              nv = v->wt->length();
              vv.push_back(nv + closed);
              vv.reserve(vv.size() + nv +closed);

              v->wt->next();
              v->wt->_val(vi_first);
              vv.push_back(vi_first);
              for (size_t j = 1; j < nv; ++j) {
                v->wt->next();
                v->wt->_val(vi);
                vv.push_back(vi);
              }
              if (closed) vv.push_back(vi_first);
              v->wt->end();
            }
            elem->wt->end();

            out << "LINES " << n_lines << " " << vv.size() << std::endl;
            for (size_t i = 0; i < vv.size(); ) {
              out << vv[i];
              for (size_t j = 0; j < vv[i]; ++j) {
                out << " " << vv[i+1+j];
              }
              out << std::endl;
              i += vv[i]+1;
            }
          }

          if (block.wt != NULL) block.wt->end();

        }
      }
	  return true;
	}
  }
}
