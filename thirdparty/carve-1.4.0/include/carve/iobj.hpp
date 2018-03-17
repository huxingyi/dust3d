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

namespace carve {
  namespace csg {
    struct IObj {
      enum {
        OBTYPE_NONE   = 0,
        OBTYPE_VERTEX = 1,
        OBTYPE_EDGE   = 2,
        OBTYPE_FACE   = 4
      } obtype;

      union {
        const carve::poly::Polyhedron::vertex_t *vertex;
        const carve::poly::Polyhedron::edge_t *edge;
        const carve::poly::Polyhedron::face_t *face;
        intptr_t val;
      };

      IObj() : obtype(OBTYPE_NONE), val(0) { }
      IObj(const carve::poly::Polyhedron::vertex_t *v) : obtype(OBTYPE_VERTEX), vertex(v) { }
      IObj(const carve::poly::Polyhedron::edge_t *e) : obtype(OBTYPE_EDGE), edge(e) { }
      IObj(const carve::poly::Polyhedron::face_t *f) : obtype(OBTYPE_FACE), face(f) { }
    };



    struct IObj_hash {
      inline size_t operator()(const IObj &i) const {
        return (size_t)i.val;
      }
      inline size_t operator()(const std::pair<const IObj, const IObj> &i) const {
        return (size_t)i.first.val ^ (size_t)i.second.val;
      }
    };



    typedef std::unordered_set<std::pair<const IObj, const IObj>, IObj_hash> IObjPairSet;

    typedef std::unordered_map<IObj, const carve::poly::Polyhedron::vertex_t *, IObj_hash> IObjVMap;
    typedef std::map<IObj, const carve::poly::Polyhedron::vertex_t *> IObjVMapSmall;

    class VertexIntersections :
      public std::unordered_map<const carve::poly::Polyhedron::vertex_t *, IObjPairSet, carve::poly::hash_vertex_ptr> {
    };



    static inline bool operator==(const carve::csg::IObj &a, const carve::csg::IObj &b) {
      return a.obtype == b.obtype && a.val == b.val;
    }

    static inline bool operator!=(const carve::csg::IObj &a, const carve::csg::IObj &b) {
      return a.obtype != b.obtype || a.val != b.val;
    }

    static inline bool operator<(const carve::csg::IObj &a, const carve::csg::IObj &b) {
      return a.obtype < b.obtype || (a.obtype == b.obtype && a.val < b.val);
    }

    static inline bool operator<=(const carve::csg::IObj &a, const carve::csg::IObj &b) {
      return a.obtype < b.obtype || (a.obtype == b.obtype && a.val <= b.val);
    }

    static inline bool operator>(const carve::csg::IObj &a, const carve::csg::IObj &b) {
      return a.obtype > b.obtype || (a.obtype == b.obtype && a.val > b.val);
    }

    static inline bool operator>=(const carve::csg::IObj &a, const carve::csg::IObj &b) {
      return a.obtype > b.obtype || (a.obtype == b.obtype && a.val >= b.val);
    }

    static inline std::ostream &operator<<(std::ostream &o, const carve::csg::IObj &a) {
      switch (a.obtype) {
        case carve::csg::IObj::OBTYPE_NONE:   o << "NONE{}"; break;
        case carve::csg::IObj::OBTYPE_VERTEX: o << "VERT{" << a.vertex << "}"; break;
        case carve::csg::IObj::OBTYPE_EDGE:   o << "EDGE{" << a.edge << "}"; break;
        case carve::csg::IObj::OBTYPE_FACE:   o << "FACE{" << a.face << "}"; break;
      }
      return o;
    }

  }
}

