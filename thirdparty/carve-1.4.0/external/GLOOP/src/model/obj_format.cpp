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
#include <gloop/model/obj_format.hpp>
#include <gloop/stringfuncs.hpp>
#include <gloop/exceptions.hpp>

#include <iostream>
#include <iomanip>



namespace gloop {
  namespace obj {

    bool ObjReader::readDoubles(std::istream &in,
                                stream::reader_base *e_rd,
                                stream::reader_base **v_rd,
                                size_t min_count,
                                size_t rd_count,
                                const double *defaults) {
      size_t i = 0;
      if (e_rd) e_rd->next();
      while (in.good() && i < rd_count) {
        double d;
        in >> d;
        if (v_rd[i]) {
          v_rd[i]->begin();
          v_rd[i]->length(1);
          v_rd[i]->next();
          v_rd[i]->_val(d);
          v_rd[i]->end();
        }
        ++i;
      }
      if (i < min_count) return false;
      while (i < rd_count) {
        if (v_rd[i]) {
          v_rd[i]->begin();
          v_rd[i]->length(1);
          v_rd[i]->next();
          v_rd[i]->_val(defaults[i]);
          v_rd[i]->end();
        }
        ++i;
      }
      return true;
    }

    bool ObjReader::readIntLists(std::istream &in,
                                 stream::reader_base *e_rd,
                                 stream::reader_base **v_rd,
                                 size_t rd_count) {
      if (e_rd) e_rd->next();

      std::string s;
      std::getline(in, s);
      std::vector<std::string> blocks;
      str::split(std::back_inserter(blocks), s);

      if (!blocks.size()) return false;

      std::vector<std::string> r1, r2;
      str::split(std::back_inserter(r1), blocks[0], '/');
      if (r1.size() > rd_count) return false;

      for (size_t i = 1; i < blocks.size(); ++i) {
        r2.clear();
        str::split(std::back_inserter(r2), blocks[i], '/');
        if (r1.size() != r2.size()) return false;
        for (size_t j = 0; j < r1.size(); ++j) {
          if ((r1[j]=="") != (r2[j]=="")) return false;
        }
      }

      for (size_t i = 0; i < std::min(rd_count, r1.size()); ++i) {
        if (v_rd[i] && r1[i].size()) {
          v_rd[i]->begin();
          v_rd[i]->length(blocks.size());
        }
      }

      for (size_t i = 0; i < blocks.size(); ++i) {
        r1.clear();
        str::split(std::back_inserter(r1), blocks[i], '/');
        for (size_t j = 0; j < std::min(rd_count, r1.size()); ++j) {
          if (v_rd[j]) {
            size_t v = strtoul(r1[j].c_str(), NULL, 10);
            v_rd[j]->next();
            v_rd[j]->_val((uint32_t)v - 1);
          }
        }
      }

      for (size_t j = 0; j < std::min(rd_count, r1.size()); ++j) {
        if (v_rd[j]) {
          v_rd[j]->end();
        }
      }

      return true;
    }

    bool ObjReader::read(std::istream &in) {
      stream::reader_base *v_rd[5];
      stream::reader_base *vp_rd[4];
      stream::reader_base *vn_rd[4];
      stream::reader_base *vt_rd[4];

      stream::reader_base *p_rd[2];
      stream::reader_base *l_rd[3];
      stream::reader_base *f_rd[4];

      stream::reader_base *rd = findReader("polyhedron");
      if (rd) {
        rd->begin();
      }

      v_rd[0]  = findReader("polyhedron", "vertex");
      v_rd[1]  = findReader("polyhedron", "vertex", "x");
      v_rd[2]  = findReader("polyhedron", "vertex", "y");
      v_rd[3]  = findReader("polyhedron", "vertex", "z");
      v_rd[4]  = findReader("polyhedron", "vertex", "w");

      vp_rd[0] = findReader("polyhedron", "parameter");
      vp_rd[1] = findReader("polyhedron", "parameter", "u");
      vp_rd[2] = findReader("polyhedron", "parameter", "v");
      vp_rd[3] = findReader("polyhedron", "parameter", "w");

      vn_rd[0] = findReader("polyhedron", "normal");
      vn_rd[1] = findReader("polyhedron", "normal", "x");
      vn_rd[2] = findReader("polyhedron", "normal", "y");
      vn_rd[3] = findReader("polyhedron", "normal", "z");

      vt_rd[0] = findReader("polyhedron", "texture");
      vt_rd[1] = findReader("polyhedron", "texture", "u");
      vt_rd[2] = findReader("polyhedron", "texture", "v");
      vt_rd[3] = findReader("polyhedron", "texture", "w");

      p_rd[0]  = findReader("polyhedron", "point");
      p_rd[1]  = findReader("polyhedron", "point", "vertex_indices");

      l_rd[0]  = findReader("polyhedron", "line");
      l_rd[1]  = findReader("polyhedron", "line", "vertex_indices");
      l_rd[2]  = findReader("polyhedron", "line", "texture_indices");

      f_rd[0]  = findReader("polyhedron", "face");
      f_rd[1]  = findReader("polyhedron", "face", "vertex_indices");
      f_rd[2]  = findReader("polyhedron", "face", "texture_indices");
      f_rd[3]  = findReader("polyhedron", "face", "normal_indices");

      while (in.good()) {
        std::string s = "";
        while (1) {
          std::string t;
          std::getline(in, t);
          if (t.size() && t[0] == '#') continue;
          t.erase(t.begin() + t.find_last_not_of("\r\n") + 1, t.end());
          s += t;
          if (s[s.size() - 1] != '\\') break;
          s.erase(s.end() - 1);
        }
        if (str::strip(s) == "") continue;
        std::istringstream inl(s);
        std::string cmd;
        inl >> cmd;
        bool result;
        // doesn't call element.begin/end/fail/length
        if (cmd == "v") {
          static const double defaults[] = { 0.0, 0.0, 0.0, 1.0 };
          result = readDoubles(inl, v_rd[0] , v_rd+1, 3, 4, defaults);
        } else if (cmd == "vp") {
          static const double defaults[] = { 0.0, 0.0, 1.0 };
          result = readDoubles(inl, vp_rd[0], vp_rd+1, 2, 3, defaults);
        } else if (cmd == "vn") {
          static const double defaults[] = { 0.0, 0.0, 0.0 };
          result = readDoubles(inl, vn_rd[0], vn_rd+1, 3, 3, defaults);
        } else if (cmd == "vt") {
          static const double defaults[] = { 0.0, 0.0, 0.0 };
          result = readDoubles(inl, vt_rd[0], vt_rd+1, 1, 3, defaults);
        } else if (cmd == "p") {
          result = readIntLists(inl, p_rd[0], p_rd+1, 1);
        } else if (cmd == "l") {
          result = readIntLists(inl, p_rd[0], l_rd+1, 2);
        } else if (cmd == "f") {
          result = readIntLists(inl, p_rd[0], f_rd+1, 3);
        } else {
          std::cerr << "warning: unhandled OBJ element [" << cmd << "]" << std::endl;
          result = true;
        }
        if (result == false) {
          if (rd) rd->fail();
          return false;
        }
      }

      if (rd) rd->end();
      return true;
    }

  }
}


namespace {

  struct prop {
    virtual bool setup(const gloop::stream::named_element_t *) const =0;
    virtual std::string next() const =0;
    virtual ~prop() {}
  };

  struct dbl : public prop {
    bool has_default;
    std::string def;
    mutable gloop::stream::named_prop_t *prop;
    std::string propname;

    dbl(const char *pn, const char *defval = NULL) : has_default(false), def(), prop(NULL), propname(pn) {
      if (defval) {
        def = defval;
        has_default = true;
      }
    }
    virtual ~dbl() {}
    virtual bool setup(const gloop::stream::named_element_t *elem) const {
      prop = elem->findProp(propname);
      if (prop == NULL && !has_default) return false;
      if (prop != NULL && prop->wt == NULL) return false;
      return true;
    }
    virtual std::string next() const {
      double v;
      std::ostringstream s;
      if (prop == NULL) {
        return def;
      }
      prop->wt->begin();
      prop->wt->next();
      prop->wt->_val(v);
      prop->wt->end();
      s<< std::setprecision(30);
      s << v;
      return s.str();
    }
  };

  struct face : public prop {
    mutable gloop::stream::named_prop_t *v_prop;
    mutable gloop::stream::named_prop_t *vt_prop;
    mutable gloop::stream::named_prop_t *vn_prop;
    int n;
    size_t v_base, vt_base, vn_base;

    face(size_t _v_base, size_t _vt_base, size_t _vn_base) :
      v_prop(NULL), vt_prop(NULL), vn_prop(NULL),
      n(0),
      v_base(_v_base), vt_base(_vt_base), vn_base(_vn_base) {
    }
    virtual ~face() {}

    virtual bool setup(const gloop::stream::named_element_t *elem) const {
      v_prop = elem->findProp("vertex_indices");
      if (v_prop == NULL || v_prop->wt == NULL) return false;
      vt_prop = elem->findProp("texture_indices");
      if (vt_prop != NULL && vt_prop->wt == NULL) return false;
      vn_prop = elem->findProp("normal_indices");
      if (vn_prop != NULL && vn_prop->wt == NULL) return false;
      return true;
    }
    virtual std::string next() const {
      std::ostringstream s;

      v_prop->wt->begin();
      if (vt_prop) vt_prop->wt->begin();
      if (vn_prop) vn_prop->wt->begin();


      int n = v_prop->wt->length();
      int n_t = vt_prop ? vt_prop->wt->length() : -1;
      int n_n = vn_prop ? vn_prop->wt->length() : -1;

      if (n_t == -1) n_t = n;
      if (n_n == -1) n_n = n;
      if (n_n != n || n_t != n) {
        std::cerr << "warning: face vertex/normal/texture indices are of different length" << std::endl;
        n = std::min(std::min(n, n_n), n_t);
      }

      for (int i = 0; i < n; ++i) {
        if (i) s << " ";
        uint32_t idx;
        v_prop->wt->next();
        v_prop->wt->_val(idx);
        idx += v_base;
        s << idx;
        if (vt_prop || vn_prop) s << "/";
        if (vt_prop) {
          vt_prop->wt->next();
          vt_prop->wt->_val(idx);
          idx += vt_base;
          s << idx;
        }
        if (vn_prop) {
          vn_prop->wt->next();
          vn_prop->wt->_val(idx);
          idx += vn_base;
          s << "/" << idx;
        }
      }

      v_prop->wt->end();
      if (vt_prop) vt_prop->wt->end();
      if (vn_prop) vn_prop->wt->end();

      return s.str();
    }
  };

  struct null : public prop {
    virtual ~null() {}

    virtual bool setup(const gloop::stream::named_element_t *elem) const {
      return true;
    }
    virtual std::string next() const {
      return "";
    }
  };

  int writeElems(std::ostream &out,
                 const gloop::stream::block_t &block,
                 const char *objtype,
                 const char *elem_name,
                 const prop &p1,
                 const prop &p2 = null(),
                 const prop &p3 = null(),
                 const prop &p4 = null()) {
    gloop::stream::named_element_t *elem = block.findElem(elem_name);
    if (elem == NULL) return 0;
    if (elem->wt == NULL) return -1;
    size_t n = elem->wt->length();
    elem->wt->begin();
    if (!p1.setup(elem)) { elem->wt->fail(); return -1; }
    if (!p2.setup(elem)) { elem->wt->fail(); return -1; }
    if (!p3.setup(elem)) { elem->wt->fail(); return -1; }
    if (!p4.setup(elem)) { elem->wt->fail(); return -1; }
    for (size_t i = 0; i < n; ++i) {
      std::string s;
      elem->wt->next();
      out << objtype;
      s = p1.next(); if (s != "") out << " " << s;
      s = p2.next(); if (s != "") out << " " << s;
      s = p3.next(); if (s != "") out << " " << s;
      s = p4.next(); if (s != "") out << " " << s;
      out << std::endl;
    }
    elem->wt->end();
    return (int)n;
  }

}

namespace gloop {
  namespace obj {

    bool ObjWriter::write(std::ostream &out) {
      size_t v_base = 1;
      size_t vt_base = 1;
      size_t vn_base = 1;

      int v_count;
      int vt_count;
      int vn_count;
      int f_count;

      for (std::list<std::pair<std::string, stream::block_t> >::iterator i = blocks.begin(); i != blocks.end(); ++i) {
        stream::block_t &block = (*i).second;

        if (block.wt != NULL) block.wt->begin();
        out << "g " << (*i).first << std::endl;

        if (block.name == "polyhedron") {
          v_count =  writeElems(out, block,  "v",    "vertex", dbl("x"), dbl("y"), dbl("z"), dbl("w", ""));
          if (v_count == -1) goto fail;
          vn_count = writeElems(out, block, "vn",    "normal", dbl("x"), dbl("y"), dbl("z"));
          if (vn_count == -1) goto fail;
          vt_count = writeElems(out, block, "vt",   "texture", dbl("u"), dbl("v"), dbl("w", ""));
          if (vt_count == -1) goto fail;

          f_count = writeElems(out, block,  "f",      "face", face(v_base, vt_base, vn_base));
          if (f_count == -1) goto fail;

          v_base += v_count;
          vn_base += vn_count;
          vt_base += vt_count;

        } else if (block.name == "polyline") {
          v_count =  writeElems(out, block,  "v",    "vertex", dbl("x"), dbl("y"), dbl("z"), dbl("w", ""));
          if (v_count == -1) goto fail;
          vt_count = writeElems(out, block, "vt",   "texture", dbl("u"), dbl("v"), dbl("w", ""));
          if (vt_count == -1) goto fail;

          v_base += v_count;
          vt_base += vt_count;

        } else {
          std::cerr << "unhandled block type: " << block.name << std::endl;

        }

        if (block.wt != NULL) block.wt->end();
        continue;

      fail:
        if (block.wt != NULL) block.wt->fail();
        return false;
      }
      return true;
    }

  }
}
