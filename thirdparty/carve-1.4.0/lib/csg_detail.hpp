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

#include <carve/polyhedron_base.hpp>

namespace carve {
  namespace csg {
    namespace detail {

    typedef std::unordered_set<
      const carve::poly::Geometry<3>::vertex_t *,
      carve::poly::hash_vertex_ptr> VSet;
    typedef std::unordered_set<
      const carve::poly::Geometry<3>::face_t *,
      carve::poly::hash_face_ptr> FSet;

    typedef std::set<const carve::poly::Geometry<3>::vertex_t *> VSetSmall;
    typedef std::set<csg::V2> V2SetSmall;
    typedef std::set<const carve::poly::Geometry<3>::face_t *> FSetSmall;

    typedef std::unordered_map<
      const carve::poly::Geometry<3>::vertex_t *,
      VSetSmall,
      carve::poly::hash_vertex_ptr> VVSMap;
    typedef std::unordered_map<
      const carve::poly::Geometry<3>::edge_t *,
      VSetSmall,
      carve::poly::hash_edge_ptr> EVSMap;
    typedef std::unordered_map<
      const carve::poly::Geometry<3>::face_t *,
      VSetSmall,
      carve::poly::hash_face_ptr> FVSMap;

    typedef std::unordered_map<
      const carve::poly::Geometry<3>::vertex_t *,
      FSetSmall,
      carve::poly::hash_vertex_ptr> VFSMap;

    typedef std::unordered_map<
      const carve::poly::Geometry<3>::face_t *,
      V2SetSmall,
      carve::poly::hash_face_ptr> FV2SMap;

    typedef std::unordered_map<
      const carve::poly::Geometry<3>::edge_t *,
      std::vector<const carve::poly::Geometry<3>::vertex_t *>,
      carve::poly::hash_edge_ptr> EVVMap;



     class LoopEdges : public std::unordered_map<V2, std::list<FaceLoop *>, carve::poly::hash_vertex_ptr> {
        typedef std::unordered_map<V2, std::list<FaceLoop *>, carve::poly::hash_vertex_ptr> super;

      public:
        void addFaceLoop(FaceLoop *fl);
        void sortFaceLoopLists();
        void removeFaceLoop(FaceLoop *fl);
      };

    }
  }
}



static inline std::ostream &operator<<(std::ostream &o, const carve::csg::detail::FSet &s) {
  const char *sep="";
  for (carve::csg::detail::FSet::const_iterator i = s.begin(); i != s.end(); ++i) {
    o << sep << *i; sep=",";
  }
  return o;
}
