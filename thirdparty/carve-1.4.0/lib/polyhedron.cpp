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

#if defined(CARVE_DEBUG)
#define DEBUG_CONTAINS_VERTEX
#endif

#include <carve/djset.hpp>

#include <carve/geom.hpp>
#include <carve/poly.hpp>

#include <carve/octree_impl.hpp>

#include <carve/timing.hpp>

#include <algorithm>

#include BOOST_INCLUDE(random.hpp)

namespace {



  struct FV {
    carve::poly::Polyhedron::face_t *face;
    size_t vertex;
    FV(carve::poly::Polyhedron::face_t *f, size_t v) : face(f), vertex(v) { }
  };



  struct EdgeFaces {
    std::list<FV> fwd, rev;
    carve::poly::Polyhedron::edge_t *edge;
  };



  struct EdgeFaceMap {
    std::unordered_map<std::pair<const carve::poly::Polyhedron::vertex_t *,
                                 const carve::poly::Polyhedron::vertex_t *>,
                       size_t,
                       carve::poly::hash_vertex_ptr> index_map;
    std::vector<EdgeFaces> edge_faces;

    void sizeHint(size_t n_faces, size_t n_vertices) {
#if defined(UNORDERED_COLLECTIONS_SUPPORT_RESIZE)
      index_map.resize(n_faces + n_vertices); // approximately, for a single closed manifold.
#endif
      edge_faces.reserve(n_faces + n_vertices);
    }

    void record(const carve::poly::Polyhedron::vertex_t *v1,
                const carve::poly::Polyhedron::vertex_t *v2,
                carve::poly::Polyhedron::face_t *f,
                size_t i) {
      if (v1 < v2) {
        size_t &x = index_map[std::make_pair(v1, v2)];
        if (x == 0) {
          edge_faces.push_back(EdgeFaces());
          x = edge_faces.size();
        }
        edge_faces[x-1].fwd.push_back(FV(f, i));
      } else {
        size_t &x = index_map[std::make_pair(v2, v1)];
        if (x == 0) {
          edge_faces.push_back(EdgeFaces());
          x = edge_faces.size();
        }
        edge_faces[x-1].rev.push_back(FV(f, i));
      }
    }
  };



  // Interestingly, for one set of inserts and a number of complete
  // traversals, a map seems to be faster than an unordered_map. This
  // may apply in other places.

  struct FaceOrder {
    double ang;
    const FV *fv;
    bool fwd;

    FaceOrder(double _ang, const FV *_fv, bool _fwd) : ang(_ang), fv(_fv), fwd(_fwd) { }
  };



  inline bool operator<(const FaceOrder &a, const FaceOrder &b) { 
    return a.ang < b.ang || (a.ang == b.ang && a.fwd && !b.fwd); 
  }



  inline std::ostream &operator<<(std::ostream &o, const FaceOrder &a) {
    o << (a.fwd ? "+" : "-") << " " << a.ang << " " << a.fv;
    return o;
  }



  bool makeFacePairs(const EdgeFaces &ef,
                     carve::poly::Polyhedron::edge_t *e,
                     std::vector<const carve::poly::Polyhedron::face_t *> &edge_face_pairs) {
    static carve::TimingName FUNC_NAME("static Polyhedron makeFacePairs()");
    carve::TimingBlock block(FUNC_NAME);
    
    edge_face_pairs.clear();

    carve::geom3d::Vector evec = (e->v2->v - e->v1->v).normalized();
    std::vector<FaceOrder> sorted_faces;

    carve::geom3d::Vector base;

    base = ef.fwd.front().face->plane_eqn.N;

    for (std::list<FV>::const_iterator
           f_i = ef.fwd.begin(), f_e = ef.fwd.end(); f_i != f_e; ++f_i) {
      double ang = carve::geom3d::antiClockwiseAngle((*f_i).face->plane_eqn.N, base, evec);
      if (ang == 0.0 && f_i != ef.fwd.begin()) ang = M_TWOPI + carve::EPSILON;
      sorted_faces.push_back(FaceOrder(ang, &(*f_i), true));
    }
    for (std::list<FV>::const_iterator
           f_i = ef.rev.begin(), f_e = ef.rev.end(); f_i != f_e; ++f_i) {
      double ang = carve::geom3d::antiClockwiseAngle(-(*f_i).face->plane_eqn.N, base, evec);
      if (ang == 0.0) ang = M_TWOPI + carve::EPSILON;
      sorted_faces.push_back(FaceOrder(ang, &(*f_i), false));
    }
    std::sort(sorted_faces.begin(), sorted_faces.end());

    for (unsigned i = 0; i < sorted_faces.size();) {
      if (!sorted_faces[i].fwd) {
        const FV &fv2 = (*(sorted_faces[i++].fv));

        edge_face_pairs.push_back(NULL);
        edge_face_pairs.push_back(fv2.face);
        // std::cerr << "face pair: " << NULL << " " << fv2.face << std::endl;
      } else if (i == sorted_faces.size() - 1 || sorted_faces[i + 1].fwd) {
        const FV &fv1 = (*(sorted_faces[i++].fv));

        edge_face_pairs.push_back(fv1.face);
        edge_face_pairs.push_back(NULL);
        // std::cerr << "face pair: " << fv1.face << " " << NULL << std::endl;
      } else {
        const FV &fv1 = (*(sorted_faces[i++].fv));
        const FV &fv2 = (*(sorted_faces[i++].fv));

        edge_face_pairs.push_back(fv1.face);
        edge_face_pairs.push_back(fv2.face);
        // std::cerr << "face pair: " << fv1.face << " " << fv2.face << std::endl;
      }
    }

    return true;
  }



  bool emb_test(carve::poly::Polyhedron *poly,
                std::map<int, std::set<int> > &embedding,
                carve::geom3d::Vector v,
                int m_id) {

    std::map<int, carve::PointClass> result;
#if defined(CARVE_DEBUG)
    std::cerr << "test " << v << " (m_id:" << m_id << ")" << std::endl;
#endif
    poly->testVertexAgainstClosedManifolds(v, result, true);
    std::set<int> inside;
    for (std::map<int, carve::PointClass>::iterator j = result.begin();
         j != result.end();
         ++j) {
      if ((*j).first == m_id) continue;
      if ((*j).second == carve::POINT_IN) inside.insert((*j).first);
      else if ((*j).second == carve::POINT_ON) {
#if defined(CARVE_DEBUG)
        std::cerr << " FAIL" << std::endl;
#endif
        return false;
      }
    }
#if defined(CARVE_DEBUG)
    std::cerr << " OK (inside.size()==" << inside.size() << ")" << std::endl;
#endif
    embedding[m_id] = inside;
    return true;
  }



  struct order_ef_first_vertex {
    bool operator()(const EdgeFaces *a, const EdgeFaces *b) const {
      return a->edge->v1 < b->edge->v1;
    }
  };



  struct order_faces {
    bool operator()(const carve::poly::Polyhedron::face_t * const &a,
                    const carve::poly::Polyhedron::face_t * const &b) const {
      return std::lexicographical_compare(a->vbegin(), a->vend(), b->vbegin(), b->vend());
    }
  };



}



namespace carve {
  namespace poly {



    struct EdgeConnectivityInfo {
      EdgeFaceMap ef_map;
    };



    bool Polyhedron::initSpatialIndex() {
      static carve::TimingName FUNC_NAME("Polyhedron::initSpatialIndex()");
      carve::TimingBlock block(FUNC_NAME);

      octree.setBounds(aabb);
      octree.addFaces(faces);
      octree.addEdges(edges);
      octree.splitTree();

      return true;
    }



    void Polyhedron::invertAll() {
      for (size_t i = 0; i < faces.size(); ++i) {
        faces[i].invert();
      }

      for (size_t i = 0; i < edges.size(); ++i) {
        std::vector<const face_t *> &f = connectivity.edge_to_face[i];
        for (size_t j = 0; j < (f.size() & ~1U); j += 2) {
          std::swap(f[j], f[j+1]);
        }
      }

      for (size_t i = 0; i < manifold_is_negative.size(); ++i) {
        manifold_is_negative[i] = !manifold_is_negative[i];
      }
    }



    void Polyhedron::invert(const std::vector<bool> &selected_manifolds) {
      bool altered = false;
      for (size_t i = 0; i < faces.size(); ++i) {
        if (faces[i].manifold_id >= 0 &&
            (unsigned)faces[i].manifold_id < selected_manifolds.size() &&
            selected_manifolds[faces[i].manifold_id]) {
          altered = true;
          faces[i].invert();
        }
      }

      if (altered) {
        for (size_t i = 0; i < edges.size(); ++i) {
          std::vector<const face_t *> &f = connectivity.edge_to_face[i];
          for (size_t j = 0; j < (f.size() & ~1U); j += 2) {
            int m_id = -1;
            if (f[j]) m_id = f[j]->manifold_id;
            if (f[j+1]) m_id = f[j+1]->manifold_id;
            if (m_id >= 0 && (unsigned)m_id < selected_manifolds.size() && selected_manifolds[m_id]) {
              std::swap(f[j], f[j+1]);
            }
          }
        }

        for (size_t i = 0; i < std::min(selected_manifolds.size(), manifold_is_negative.size()); ++i) {
          manifold_is_negative[i] = !manifold_is_negative[i];
        }
      }
    }



    void Polyhedron::initVertexConnectivity() {
      static carve::TimingName FUNC_NAME("static Polyhedron initVertexConnectivity()");
      carve::TimingBlock block(FUNC_NAME);

      // allocate space for connectivity info.
      connectivity.vertex_to_edge.resize(vertices.size());
      connectivity.vertex_to_face.resize(vertices.size());

      std::vector<size_t> vertex_face_count;

      vertex_face_count.resize(vertices.size());

      // work out how many faces/edges each vertex is connected to, in
      // order to save on array reallocs.
      for (unsigned i = 0; i < faces.size(); ++i) {
        face_t &f = faces[i];
        for (unsigned j = 0; j < f.nVertices(); j++) {
          vertex_face_count[vertexToIndex_fast(f.vertex(j))]++;
        }
      }

      for (size_t i = 0; i < vertices.size(); ++i) {
        connectivity.vertex_to_edge[i].reserve(vertex_face_count[i]);
        connectivity.vertex_to_face[i].reserve(vertex_face_count[i]);
      }

      // record connectivity from vertex to edges.
      for (size_t i = 0; i < edges.size(); ++i) {
        size_t v1i = vertexToIndex_fast(edges[i].v1);
        size_t v2i = vertexToIndex_fast(edges[i].v2);

        connectivity.vertex_to_edge[v1i].push_back(&edges[i]);
        connectivity.vertex_to_edge[v2i].push_back(&edges[i]);
      }

      // record connectivity from vertex to faces.
      for (size_t i = 0; i < faces.size(); ++i) {
        face_t &f = faces[i];
        for (unsigned j = 0; j < f.nVertices(); j++) {
          size_t vi = vertexToIndex_fast(f.vertex(j));
          connectivity.vertex_to_face[vi].push_back(&f);
        }
      }
    }



    bool Polyhedron::initEdgeConnectivity(const EdgeConnectivityInfo &eci) {
      static carve::TimingName FUNC_NAME("static Polyhedron initEdgeConnectivity()");
      carve::TimingBlock block(FUNC_NAME);

      const std::vector<EdgeFaces> &ef = eci.ef_map.edge_faces;

      // pair up incident faces for each edge.
      bool is_ok = true;
      bool complex = false;
      carve::djset::djset face_groups(faces.size());
      std::vector<bool> face_open(faces.size(), false);

      connectivity.edge_to_face.resize(edges.size());

      // simple edges
      for (size_t i = 0; i < ef.size(); ++i) {
        const std::list<FV> &fwd_faces = ef[i].fwd;
        const std::list<FV> &rev_faces = ef[i].rev;

        edge_t *edge = ef[i].edge;
        size_t edge_index = edgeToIndex_fast(edge);
        std::vector<const face_t *> &edge_face_pairs = connectivity.edge_to_face[edge_index];
        edge_face_pairs.clear();

        if (rev_faces.size() == 0) {
          for (std::list<FV>::const_iterator j = fwd_faces.begin(); j != fwd_faces.end(); ++j) {
            face_open[faceToIndex_fast(j->face)] = true;
            edge_face_pairs.push_back(j->face);
            edge_face_pairs.push_back(NULL);
          }
        } else if (fwd_faces.size() == 0) {
          for (std::list<FV>::const_iterator j = rev_faces.begin(); j != rev_faces.end(); ++j) {
            face_open[faceToIndex_fast(j->face)] = true;
            edge_face_pairs.push_back(NULL);
            edge_face_pairs.push_back(j->face);
          }
        } else if (fwd_faces.size() == 1 && rev_faces.size() == 1) {
          edge_face_pairs.push_back(fwd_faces.front().face);
          edge_face_pairs.push_back(rev_faces.front().face);
          face_groups.merge_sets(faceToIndex_fast(fwd_faces.front().face),
                                 faceToIndex_fast(rev_faces.front().face));
        } else {
          complex = true;
        }
      }

      if (!complex) return is_ok;

      // propagate openness of each face set to all members of the set.
      for (size_t i = 0; i < faces.size(); ++i) {
        if (face_open[i]) face_open[face_groups.find_set_head(i)] = true;
      }

      for (size_t i = 0; i < faces.size(); ++i) {
        face_open[i] = face_open[face_groups.find_set_head(i)];
      }

      // complex edges.
      std::map<std::vector<int>, std::list<EdgeFaces> > grouped_ef;

      for (size_t i = 0; is_ok && i < ef.size(); ++i) {
        const std::list<FV> &fwd_faces = ef[i].fwd;
        const std::list<FV> &rev_faces = ef[i].rev;

        edge_t *edge = ef[i].edge;
        size_t edge_index = edgeToIndex_fast(edge);
        std::vector<const face_t *> &edge_face_pairs = connectivity.edge_to_face[edge_index];

        if (!edge_face_pairs.size()) {
          const EdgeFaces &ef_old = ef[i];

          EdgeFaces ef_closed;
          EdgeFaces ef_open;

          ef_open.edge = ef_old.edge;
          ef_closed.edge = ef_old.edge;

          // group faces into those that form part of an open surface and those that form a closed surface.
          for (std::list<FV>::const_iterator j = ef_old.fwd.begin(); j != ef_old.fwd.end(); ++j) {
            if (face_open[faceToIndex_fast(j->face)]) {
              ef_open.fwd.push_back(*j);
            } else {
              ef_closed.fwd.push_back(*j);
            }
          }

          for (std::list<FV>::const_iterator j = ef_old.rev.begin(); j != ef_old.rev.end(); ++j) {
            if (face_open[faceToIndex_fast(j->face)]) {
              ef_open.rev.push_back(*j);
            } else {
              ef_closed.rev.push_back(*j);
            }
          }

          // make open edge connectivity entries for any open surface faces.
          for (std::list<FV>::const_iterator j = ef_open.fwd.begin(); j != ef_open.fwd.end(); ++j) {
            edge_face_pairs.push_back(j->face);
            edge_face_pairs.push_back(NULL);
          }
          for (std::list<FV>::const_iterator j = ef_open.rev.begin(); j != ef_open.rev.end(); ++j) {
            edge_face_pairs.push_back(NULL);
            edge_face_pairs.push_back(j->face);
          }

          if (ef_closed.fwd.size() == 0 && ef_closed.rev.size() == 0) continue;

          // group edges based upon the closed-face sets that are incident to them.
          std::vector<int> tag;
          tag.reserve(ef_closed.fwd.size() + ef_closed.rev.size());
          for (std::list<FV>::const_iterator j = ef_closed.fwd.begin(); j != ef_closed.fwd.end(); ++j) {
            tag.push_back(+face_groups.find_set_head(faceToIndex_fast(j->face)) + 1);
          }
          for (std::list<FV>::const_iterator j = ef_closed.rev.begin(); j != ef_closed.rev.end(); ++j) {
            tag.push_back(-face_groups.find_set_head(faceToIndex_fast(j->face)) - 1);
          }
          std::sort(tag.begin(), tag.end());

          // normalise tag to remove edge direction effects.
          std::vector<int> revtag = tag;
          std::reverse(revtag.begin(), revtag.end());
          std::transform(revtag.begin(), revtag.end(), revtag.begin(), std::negate<int>());

          if (!std::lexicographical_compare(tag.begin(), tag.end(), revtag.begin(), revtag.end())) {
            std::swap(tag, revtag);
          }

          grouped_ef[tag].push_back(ef_closed);
        }
      }


      // std::cerr << "grouped ef tags:" << std::endl;
      for (std::map<std::vector<int>, std::list<EdgeFaces> > ::iterator i = grouped_ef.begin(); i != grouped_ef.end(); ++i) {
        const std::vector<int> &tag = (*i).first;

        // std::cerr << "tag.size() == " << tag.size() << std::endl;
        // for (size_t j = 0; j < tag.size(); ++j) {
        //   std::cerr << "  " << tag[j];
        // }
        // std::cerr << std::endl;

        std::vector<EdgeFaces *> efp;
        efp.reserve((*i).second.size());
        carve::djset::djset efp_group((*i).second.size());
        std::map<const vertex_t *, std::vector<int> > vec_ef;

        for (std::list<EdgeFaces>::iterator j = (*i).second.begin(); j != (*i).second.end(); ++j) {
          EdgeFaces &ef = *j;
          vec_ef[ef.edge->v1].push_back(efp.size());
          vec_ef[ef.edge->v2].push_back(efp.size());
          efp.push_back(&ef);
        }

        for (std::map<const vertex_t *, std::vector<int> >::iterator j = vec_ef.begin(); j != vec_ef.end(); ++j) {
          if ((*j).second.size() == 2) {
            efp_group.merge_sets((*j).second[0], (*j).second[1]);
          }
        }

        std::vector<std::vector<EdgeFaces *> > grouped;
        efp_group.collate(efp.begin(), grouped);

        for (size_t j = 0; j < grouped.size(); ++j) {
          std::vector<EdgeFaces *> grp = grouped[j];
          for (size_t k = 0; k < grp.size(); ++k) {
            EdgeFaces &ef = *grp[k];

            edge_t *edge = ef.edge;
            size_t edge_index = edgeToIndex_fast(edge);
            std::vector<const face_t *> &edge_face_pairs = connectivity.edge_to_face[edge_index];

            if (!makeFacePairs(ef, edge, edge_face_pairs)) {
              is_ok = false;
            }
          }
        }
      }

      return is_ok;
    }



    void Polyhedron::buildEdgeFaceMap(EdgeConnectivityInfo &eci) {
      // make a mapping from pairs of vertices denoting edges to pairs
      // of <face,vertex index> that incorporate this edge in the
      // forward and reverse directions.
      for (unsigned i = 0; i < faces.size(); ++i) {
        face_t &f = faces[i];
        for (unsigned j = 0; j < f.nVertices() - 1; j++) {
          eci.ef_map.record(f.vertex(j), f.vertex(j+1), &f, j);
        }
        eci.ef_map.record(f.vertex(f.nVertices()-1), f.vertex(0), &f, f.nVertices()-1);

        f.manifold_id = -1;
      }
    }



    bool Polyhedron::initConnectivity() {
      static carve::TimingName FUNC_NAME("Polyhedron::initConnectivity()");
      carve::TimingBlock block(FUNC_NAME);

      EdgeConnectivityInfo eci;
      eci.ef_map.sizeHint(faces.size(), vertices.size());
      bool is_ok = true;

      buildEdgeFaceMap(eci);

      // now we know how many edges this polyhedron has.
      edges.clear();
      edges.reserve(eci.ef_map.edge_faces.size());

      // make an edge object for each entry in ef_map.
      for (size_t i = 0; i < eci.ef_map.edge_faces.size(); ++i) {
        EdgeFaces &ef = eci.ef_map.edge_faces[i];
        const std::list<FV> &fwd = ef.fwd;
        const std::list<FV> &rev = ef.rev;

        const vertex_t *v1, *v2;

        if (fwd.size()) {
          face_t *f = fwd.front().face;
          size_t v = fwd.front().vertex;
          v1 = f->vertex(v);
          v2 = f->vertex((v+1) % f->nVertices());
	} else { 
          face_t *f = rev.front().face;
          size_t v = rev.front().vertex;
          v2 = f->vertex(v);
          v1 = f->vertex((v+1) % f->nVertices());
        }

        edges.push_back(edge_t(v1, v2, this));
        ef.edge = &edges.back();

        for (std::list<FV>::const_iterator j = fwd.begin(); j != fwd.end(); ++j) {
          (*j).face->edge((*j).vertex) = &edges.back();
        }

        for (std::list<FV>::const_iterator j = rev.begin(); j != rev.end(); ++j) {
          (*j).face->edge((*j).vertex) = &edges.back();
        }
      }

      initVertexConnectivity();

      return initEdgeConnectivity(eci);
    }



    bool Polyhedron::calcManifoldEmbedding() {
      // this could be significantly sped up using bounding box tests
      // to work out what pairs of manifolds are embedding candidates.
      // A per-manifold AABB could also be used to speed up
      // testVertexAgainstClosedManifolds().

      static carve::TimingName FUNC_NAME("Polyhedron::calcManifoldEmbedding()");
      static carve::TimingName CME_V("Polyhedron::calcManifoldEmbedding() (vertices)");
      static carve::TimingName CME_E("Polyhedron::calcManifoldEmbedding() (edges)");
      static carve::TimingName CME_F("Polyhedron::calcManifoldEmbedding() (faces)");

      carve::TimingBlock block(FUNC_NAME);

      const unsigned MCOUNT = manifoldCount();
      if (MCOUNT < 2) return true;

      std::set<int> vertex_manifolds;
      std::map<int, std::set<int> > embedding;

      carve::Timing::start(CME_V);
      for (size_t i = 0; i < vertices.size(); ++i) {
        vertex_manifolds.clear();
        if (vertexManifolds(&vertices[i], set_inserter(vertex_manifolds)) != 1) continue;
        int m_id = *vertex_manifolds.begin();
        if (embedding.find(m_id) == embedding.end()) {
          if (emb_test(this, embedding, vertices[i].v, m_id) && embedding.size() == MCOUNT) {
            carve::Timing::stop();
            goto done;
          }
        }
      }
      carve::Timing::stop();

      carve::Timing::start(CME_E);
      for (size_t i = 0; i < edges.size(); ++i) {
        if (connectivity.edge_to_face[i].size() == 2) {
          int m_id;
          const face_t *f1 = connectivity.edge_to_face[i][0];
          const face_t *f2 = connectivity.edge_to_face[i][1];
          if (f1) m_id = f1->manifold_id;
          if (f2) m_id = f2->manifold_id;
          if (embedding.find(m_id) == embedding.end()) {
            if (emb_test(this, embedding, (edges[i].v1->v + edges[i].v2->v) / 2, m_id) && embedding.size() == MCOUNT) {
              carve::Timing::stop();
              goto done;
            }
          }
        }
      }
      carve::Timing::stop();

      carve::Timing::start(CME_F);
      for (size_t i = 0; i < faces.size(); ++i) {
        int m_id = faces[i].manifold_id;
        if (embedding.find(m_id) == embedding.end()) {
          carve::geom2d::P2 pv;
          if (!carve::geom2d::pickContainedPoint(faces[i].projectedVertices(), pv)) continue;
          carve::geom3d::Vector v = carve::poly::face::unproject(faces[i], pv);
          if (emb_test(this, embedding, v, m_id) && embedding.size() == MCOUNT) {
            carve::Timing::stop();
            goto done;
          }
        }
      }
      carve::Timing::stop();

      std::cerr << "could not find test points!!!" << std::endl;
      return true;

      CARVE_FAIL("could not find test points");

    done:;
      for (std::map<int, std::set<int> >::iterator i = embedding.begin(); i != embedding.end(); ++i) {
#if defined(CARVE_DEBUG)
        std::cerr << (*i).first << " : ";
        std::copy((*i).second.begin(), (*i).second.end(), std::ostream_iterator<int>(std::cerr, ","));
        std::cerr << std::endl;
#endif
        (*i).second.insert(-1);
      }
      std::set<int> parents, new_parents;
      parents.insert(-1);

      while (embedding.size()) {
        new_parents.clear();
        for (std::map<int, std::set<int> >::iterator i = embedding.begin(); i != embedding.end(); ++i) {
          if ((*i).second.size() == 1) {
            if (parents.find(*(*i).second.begin()) != parents.end()) {
              new_parents.insert((*i).first);
#if defined(CARVE_DEBUG)
              std::cerr << "parent(" << (*i).first << "): " << *(*i).second.begin() << std::endl;
#endif
            } else {
#if defined(CARVE_DEBUG)
              std::cerr << "no parent: " << (*i).first << " (looking for: " << *(*i).second.begin() << ")" << std::endl;
#endif
            }
          }
        }
        for (std::set<int>::const_iterator i = new_parents.begin(); i != new_parents.end(); ++i) {
          embedding.erase(*i);
        }
        for (std::map<int, std::set<int> >::iterator i = embedding.begin(); i != embedding.end(); ++i) {
          size_t n = 0;
          for (std::set<int>::const_iterator j = parents.begin(); j != parents.end(); ++j) {
            n += (*i).second.erase((*j));
          }
          CARVE_ASSERT(n != 0);
        }
        parents.swap(new_parents);
      }

      return true;
    }



    bool Polyhedron::markManifolds() {
      static carve::TimingName FUNC_NAME("Polyhedron::markManifolds()");

      carve::TimingBlock block(FUNC_NAME);

      std::vector<face_t *> to_mark;
      size_t i = 0;
      int m_id = 0;
      int closed_manifold_count = 0;

      const vertex_t *min_vertex = NULL;
      std::set<const face_t *> min_faces;

      manifold_is_closed.clear();
      manifold_is_negative.clear();

      while (1) {

        while (i < faces.size() && faces[i].manifold_id != -1) ++i;
        if (i == faces.size()) break;

        to_mark.push_back(&faces[i]);
        min_vertex = faces[i].vertex(0);

        bool is_closed = true;

        while (to_mark.size()) {
          face_t *f = to_mark.back();
          to_mark.pop_back();

          if (f->manifold_id == -1) {
            f->manifold_id = m_id;

            const vertex_t *v = f->vertex(0);
            for (size_t j = 1; j < f->nVertices(); ++j) {
              if (f->vertex(j)->v < v->v) {
                v = f->vertex(j);
              }
            }
            if (v->v < min_vertex->v) {
              min_vertex = v;
            }

            for (size_t j = 0; j < f->nEdges(); ++j) {
              face_t *g = const_cast<face_t *>(connectedFace(f, f->edge(j)));

              if (g) {
                if (g->manifold_id == -1) to_mark.push_back(g);
              } else {
                is_closed = false;
              }
            }
          }
        }

        vertexToFaces(min_vertex, set_inserter(min_faces));

        double max_abs_x = 0.0;
        for (std::set<const face_t *>::iterator i = min_faces.begin(); i != min_faces.end(); ++i) {
          if (fabs((*i)->plane_eqn.N.x) > fabs(max_abs_x)) max_abs_x = (*i)->plane_eqn.N.x;
        }

        manifold_is_closed.push_back(is_closed);
        manifold_is_negative.push_back(is_closed && max_abs_x > 0.0);
#if defined(CARVE_DEBUG)
        std::cerr << "{manifold: " << m_id << (manifold_is_negative.back() ? " is" : " is not") << " negative}" << std::endl;
#endif
        if (is_closed) closed_manifold_count++;
        ++m_id;
      }

#if defined(CARVE_DEBUG)
      std::cerr << "polyhedron " << this << " has " << m_id << " manifolds (" << closed_manifold_count << " closed)" << std::endl;
#endif

      return true;
    }



    bool Polyhedron::init() {
      static carve::TimingName FUNC_NAME("Polyhedron::init()");
      carve::TimingBlock block(FUNC_NAME);
  
      aabb.fit(vertices.begin(), vertices.end(), vec_adapt_vertex_ref());

      connectivity.vertex_to_edge.clear();
      connectivity.vertex_to_face.clear();
      connectivity.edge_to_face.clear();

      // if (!orderVertices()) return false;
      if (!initConnectivity()) return false;
      if (!initSpatialIndex()) return false;
      if (!markManifolds()) return false;
      // if (!calcManifoldEmbedding()) return false;
      return true;
    }



    void Polyhedron::faceRecalc() {
      for (size_t i = 0; i < faces.size(); ++i) {
        if (!faces[i].recalc()) {
          std::ostringstream out;
          out << "face " << i << " recalc failed";
          throw carve::exception(out.str());
        }
      }
    }



    Polyhedron::Polyhedron(const Polyhedron &poly) {
      faces.reserve(poly.faces.size());

      for (size_t i = 0; i < poly.faces.size(); ++i) {
        const face_t &src = poly.faces[i];
        faces.push_back(src);
      }
      commonFaceInit(false); // calls setFaceAndVertexOwner() and init()
    }



    Polyhedron::Polyhedron(const Polyhedron &poly, const std::vector<bool> &selected_manifolds) {
      size_t n_faces = 0;

      for (size_t i = 0; i < poly.faces.size(); ++i) {
        const face_t &src = poly.faces[i];
        if (src.manifold_id >= 0 &&
            (unsigned)src.manifold_id < selected_manifolds.size() &&
            selected_manifolds[src.manifold_id]) {
          n_faces++;
        }
      }

      faces.reserve(n_faces);

      for (size_t i = 0; i < poly.faces.size(); ++i) {
        const face_t &src = poly.faces[i];
        if (src.manifold_id >= 0 &&
            (unsigned)src.manifold_id < selected_manifolds.size() &&
            selected_manifolds[src.manifold_id]) {
          faces.push_back(src);
        }
      }

      commonFaceInit(false); // calls setFaceAndVertexOwner() and init()
    }



    Polyhedron::Polyhedron(const Polyhedron &poly, int m_id) {
      size_t n_faces = 0;

      for (size_t i = 0; i < poly.faces.size(); ++i) {
        const face_t &src = poly.faces[i];
        if (src.manifold_id == m_id) n_faces++;
      }

      faces.reserve(n_faces);

      for (size_t i = 0; i < poly.faces.size(); ++i) {
        const face_t &src = poly.faces[i];
        if (src.manifold_id == m_id) faces.push_back(src);
      }

      commonFaceInit(false); // calls setFaceAndVertexOwner() and init()
    }



    Polyhedron::Polyhedron(const std::vector<carve::geom3d::Vector> &_vertices,
                           int n_faces,
                           const std::vector<int> &face_indices) {
      // The polyhedron is defined by a vector of vertices, which we
      // want to copy, and a face index list, from which we need to
      // generate a set of Faces.

      vertices.clear();
      vertices.resize(_vertices.size());
      for (size_t i = 0; i < _vertices.size(); ++i) {
        vertices[i].v = _vertices[i];
      }

      faces.reserve(n_faces);
  
      std::vector<int>::const_iterator iter = face_indices.begin();
      std::vector<const vertex_t *> v;
      for (int i = 0; i < n_faces; ++i) {
        int vertexCount = *iter++;
    
        v.clear();
    
        while (vertexCount--) {
          CARVE_ASSERT(*iter >= 0);
          CARVE_ASSERT((unsigned)*iter < vertices.size());
          v.push_back(&vertices[*iter++]);
        }
        faces.push_back(face_t(v));
      }

      setFaceAndVertexOwner();

      if (!init()) {
        throw carve::exception("polyhedron creation failed");
      }
    }



    Polyhedron::Polyhedron(std::vector<face_t> &_faces,
                           std::vector<vertex_t> &_vertices,
                           bool _recalc) {
      faces.swap(_faces);
      vertices.swap(_vertices);

      setFaceAndVertexOwner();

      if (_recalc) faceRecalc();

      if (!init()) {
        throw carve::exception("polyhedron creation failed");
      }
    }



    Polyhedron::Polyhedron(std::vector<face_t> &_faces,
                           bool _recalc) {
      faces.swap(_faces);
      commonFaceInit(_recalc); // calls setFaceAndVertexOwner() and init()
    }



    Polyhedron::Polyhedron(std::list<face_t> &_faces,
                           bool _recalc) {
      faces.reserve(_faces.size());
      std::copy(_faces.begin(), _faces.end(), std::back_inserter(faces));
      commonFaceInit(_recalc); // calls setFaceAndVertexOwner() and init()
    }



    void Polyhedron::collectFaceVertices(std::vector<face_t> &faces,
                                         std::vector<vertex_t> &vertices,
                                         carve::csg::VVMap &vmap) {
      // Given a set of faces, copy all referenced vertices into a
      // single vertex array and update the faces to point into that
      // array. On exit, vmap contains a mapping from old pointer to
      // new pointer.

      vertices.clear();
      vmap.clear();

      for (size_t i = 0, il = faces.size(); i != il; ++i) {
        face_t &f = faces[i];

        for (size_t j = 0, jl = f.nVertices(); j != jl; ++j) {
          vmap[f.vertex(j)] = NULL;
        }
      }

      vertices.reserve(vmap.size());

      for (carve::csg::VVMap::iterator i = vmap.begin(),
             e = vmap.end();
           i != e;
           ++i) {
        vertices.push_back(*(*i).first);
        (*i).second = &vertices.back();
      }

      for (size_t i = 0, il = faces.size(); i != il; ++i) {
        face_t &f = faces[i];

        for (size_t j = 0, jl = f.nVertices(); j != jl; ++j) {
          f.vertex(j) = vmap[f.vertex(j)];
        }
      }
    }



    void Polyhedron::collectFaceVertices(std::vector<face_t> &faces,
                                         std::vector<vertex_t> &vertices) {
      VVMap vmap;
      collectFaceVertices(faces, vertices, vmap);
    }



    void Polyhedron::setFaceAndVertexOwner() {
      for (size_t i = 0; i < vertices.size(); ++i) vertices[i].owner = this;
      for (size_t i = 0; i < faces.size(); ++i) faces[i].owner = this;
    }



    void Polyhedron::commonFaceInit(bool _recalc) {
      collectFaceVertices(faces, vertices);
      setFaceAndVertexOwner();
      if (_recalc) faceRecalc();

      if (!init()) {
        throw carve::exception("polyhedron creation failed");
      }
    }



    Polyhedron::~Polyhedron() {
    }



    void Polyhedron::testVertexAgainstClosedManifolds(const carve::geom3d::Vector &v,
                                                      std::map<int, PointClass> &result,
                                                      bool ignore_orientation) const {

      for (size_t i = 0; i < faces.size(); i++) {
        if (!manifold_is_closed[faces[i].manifold_id]) continue; // skip open manifolds
        if (faces[i].containsPoint(v)) {
          result[faces[i].manifold_id] = POINT_ON;
        }
      }

      double ray_len = aabb.extent.length() * 2;

      std::vector<const face_t *> possible_faces;

      std::vector<std::pair<const face_t *, carve::geom3d::Vector> > manifold_intersections;

      boost::mt19937 rng;
      boost::uniform_on_sphere<double> distrib(3);
      boost::variate_generator<boost::mt19937 &, boost::uniform_on_sphere<double> > gen(rng, distrib);

      while (1) {
        carve::geom3d::Vector ray_dir;
        ray_dir = gen();

        carve::geom3d::Vector v2 = v + ray_dir * ray_len;

        bool failed = false;
        carve::geom3d::LineSegment line(v, v2);
        carve::geom3d::Vector intersection;

        possible_faces.clear();
        manifold_intersections.clear();
        octree.findFacesNear(line, possible_faces);

        for (unsigned i = 0; !failed && i < possible_faces.size(); i++) {
          if (!manifold_is_closed[possible_faces[i]->manifold_id]) continue; // skip open manifolds
          if (result.find(possible_faces[i]->manifold_id) != result.end()) continue; // already ON

          switch (possible_faces[i]->lineSegmentIntersection(line, intersection)) {
          case INTERSECT_FACE: {
            manifold_intersections.push_back(std::make_pair(possible_faces[i], intersection));
            break;
          }
          case INTERSECT_NONE: {
            break;
          }
          default: {
            failed = true;
            break;
          }
          }
        }

        if (!failed) break;
      }

      std::vector<int> crossings(manifold_is_closed.size(), 0);

      for (size_t i = 0; i < manifold_intersections.size(); ++i) {
        const face_t *f = manifold_intersections[i].first;
        crossings[f->manifold_id]++;
      }

      for (size_t i = 0; i < crossings.size(); ++i) {
#if defined(CARVE_DEBUG)
        std::cerr << "crossing: " << i << " = " << crossings[i] << " is_negative = " << manifold_is_negative[i] << std::endl;
#endif
        if (!manifold_is_closed[i]) continue;
        if (result.find(i) != result.end()) continue;
        PointClass pc = (crossings[i] & 1) ? POINT_IN : POINT_OUT;
        if (!ignore_orientation && manifold_is_negative[i]) pc = (PointClass)-pc;
        result[i] = pc;
      }
    }



    PointClass Polyhedron::containsVertex(const carve::geom3d::Vector &v,
                                          const face_t **hit_face,
                                          bool even_odd,
                                          int manifold_id) const {
      if (hit_face) *hit_face = NULL;

#if defined(DEBUG_CONTAINS_VERTEX)
      std::cerr << "{containsVertex " << v << "}" << std::endl;
#endif

      if (!aabb.containsPoint(v)) {
#if defined(DEBUG_CONTAINS_VERTEX)
        std::cerr << "{final:OUT(aabb short circuit)}" << std::endl;
#endif
        // XXX: if the top level manifolds are negative, this should be POINT_IN.
        // for the moment, this only works for a single manifold.
        if (manifold_is_negative.size() == 1 && manifold_is_negative[0]) return POINT_IN;
        return POINT_OUT;
      }

      for (size_t i = 0; i < faces.size(); i++) {
        if (manifold_id != -1 && manifold_id != faces[i].manifold_id) continue;

        // XXX: Do allow the tested vertex to be ON an open
        // manifold. This was here originally because of the
        // possibility of an open manifold contained within a closed
        // manifold.

        // if (!manifold_is_closed[faces[i].manifold_id]) continue;

        if (faces[i].containsPoint(v)) {
#if defined(DEBUG_CONTAINS_VERTEX)
          std::cerr << "{final:ON(hits face " << &faces[i] << ")}" << std::endl;
#endif
          if (hit_face) *hit_face = &faces[i];
          return POINT_ON;
        }
      }

      double ray_len = aabb.extent.length() * 2;

      std::vector<const face_t *> possible_faces;

      std::vector<std::pair<const face_t *, carve::geom3d::Vector> > manifold_intersections;

      while (1) {
        double a1 = random() / double(RAND_MAX) * M_TWOPI;
        double a2 = random() / double(RAND_MAX) * M_TWOPI;

        carve::geom3d::Vector ray_dir = carve::geom::VECTOR(sin(a1) * sin(a2), cos(a1) * sin(a2), cos(a2));

#if defined(DEBUG_CONTAINS_VERTEX)
        std::cerr << "{testing ray: " << ray_dir << "}" << std::endl;
#endif

        carve::geom3d::Vector v2 = v + ray_dir * ray_len;

        bool failed = false;
        carve::geom3d::LineSegment line(v, v2);
        carve::geom3d::Vector intersection;

        possible_faces.clear();
        manifold_intersections.clear();
        octree.findFacesNear(line, possible_faces);

        for (unsigned i = 0; !failed && i < possible_faces.size(); i++) {
          if (manifold_id != -1 && manifold_id != faces[i].manifold_id) continue;

          if (!manifold_is_closed[possible_faces[i]->manifold_id]) continue;

          switch (possible_faces[i]->lineSegmentIntersection(line, intersection)) {
          case INTERSECT_FACE: {

#if defined(DEBUG_CONTAINS_VERTEX)
            std::cerr << "{intersects face: " << possible_faces[i]
                      << " dp: " << dot(ray_dir, possible_faces[i]->plane_eqn.N) << "}" << std::endl;
#endif

            if (!even_odd && fabs(dot(ray_dir, possible_faces[i]->plane_eqn.N)) < EPSILON) {

#if defined(DEBUG_CONTAINS_VERTEX)
              std::cerr << "{failing(small dot product)}" << std::endl;
#endif

              failed = true;
              break;
            }
            manifold_intersections.push_back(std::make_pair(possible_faces[i], intersection));
            break;
          }
          case INTERSECT_NONE: {
            break;
          }
          default: {

#if defined(DEBUG_CONTAINS_VERTEX)
            std::cerr << "{failing(degenerate intersection)}" << std::endl;
#endif
            failed = true;
            break;
          }
          }
        }

        if (!failed) {
          if (even_odd) {
            return (manifold_intersections.size() & 1) ? POINT_IN : POINT_OUT;
          }

#if defined(DEBUG_CONTAINS_VERTEX)
          std::cerr << "{intersections ok [count:"
                    << manifold_intersections.size()
                    << "], sorting}"
                    << std::endl;
#endif

          carve::geom3d::sortInDirectionOfRay(ray_dir,
                                              manifold_intersections.begin(),
                                              manifold_intersections.end(),
                                              carve::geom3d::vec_adapt_pair_second());

          std::vector<int> crossings(manifold_is_closed.size(), 0);

          for (size_t i = 0; i < manifold_intersections.size(); ++i) {
            const face_t *f = manifold_intersections[i].first;
            if (dot(ray_dir, f->plane_eqn.N) < 0.0) {
              crossings[f->manifold_id]++;
            } else {
              crossings[f->manifold_id]--;
            }
          }

#if defined(DEBUG_CONTAINS_VERTEX)
          for (size_t i = 0; i < crossings.size(); ++i) {
            std::cerr << "{manifold " << i << " crossing count: " << crossings[i] << "}" << std::endl;
          }
#endif

          for (size_t i = 0; i < manifold_intersections.size(); ++i) {
            const face_t *f = manifold_intersections[i].first;

#if defined(DEBUG_CONTAINS_VERTEX)
            std::cerr << "{intersection at "
                      << manifold_intersections[i].second
                      << " id: "
                      << f->manifold_id
                      << " count: "
                      << crossings[f->manifold_id]
                      << "}"
                      << std::endl;
#endif

            if (crossings[f->manifold_id] < 0) {
              // inside this manifold.

#if defined(DEBUG_CONTAINS_VERTEX)
              std::cerr << "{final:IN}" << std::endl;
#endif

              return POINT_IN;
            } else if (crossings[f->manifold_id] > 0) {
              // outside this manifold, but it's an infinite manifold. (for instance, an inverted cube)

#if defined(DEBUG_CONTAINS_VERTEX)
              std::cerr << "{final:OUT}" << std::endl;
#endif

              return POINT_OUT;
            }
          }

#if defined(DEBUG_CONTAINS_VERTEX)
          std::cerr << "{final:OUT(default)}" << std::endl;
#endif

          return POINT_OUT;
        }
      }
    }



    void Polyhedron::findEdgesNear(const carve::geom::aabb<3> &aabb,
                                   std::vector<const edge_t *> &outEdges) const {
      outEdges.clear();
      octree.findEdgesNear(aabb, outEdges);
    }



    void Polyhedron::findEdgesNear(const carve::geom3d::LineSegment &line,
                                   std::vector<const edge_t *> &outEdges) const {
      outEdges.clear();
      octree.findEdgesNear(line, outEdges);
    }



    void Polyhedron::findEdgesNear(const carve::geom3d::Vector &v,
                                   std::vector<const edge_t *> &outEdges) const {
      outEdges.clear();
      octree.findEdgesNear(v, outEdges);
    }



    void Polyhedron::findEdgesNear(const face_t &face,
                                   std::vector<const edge_t *> &edges) const {
      edges.clear();
      octree.findEdgesNear(face, edges);
    }



    void Polyhedron::findEdgesNear(const edge_t &edge,
                                   std::vector<const edge_t *> &outEdges) const {
      outEdges.clear();
      octree.findEdgesNear(edge, outEdges);
    }



    void Polyhedron::findFacesNear(const carve::geom3d::LineSegment &line,
                                   std::vector<const face_t *> &outFaces) const {
      outFaces.clear();
      octree.findFacesNear(line, outFaces);
    }



    void Polyhedron::findFacesNear(const carve::geom::aabb<3> &aabb,
                                   std::vector<const face_t *> &outFaces) const {
      outFaces.clear();
      octree.findFacesNear(aabb, outFaces);
    }



    void Polyhedron::findFacesNear(const edge_t &edge,
                                   std::vector<const face_t *> &outFaces) const {
      outFaces.clear();
      octree.findFacesNear(edge, outFaces);
    }



    void Polyhedron::transform(const carve::math::Matrix &xform) {
      for (size_t i = 0; i < vertices.size(); i++) {
        vertices[i].v = xform * vertices[i].v;
      }
      for (size_t i = 0; i < faces.size(); i++) {
        faces[i].recalc();
      }
      init();
    }



    void Polyhedron::print(std::ostream &o) const {
      o << "Polyhedron@" << this << " {" << std::endl;
      for (std::vector<vertex_t >::const_iterator
             i = vertices.begin(), e = vertices.end(); i != e; ++i) {
        o << "  V@" << &(*i) << " " << (*i).v << std::endl;
      }
      for (std::vector<edge_t >::const_iterator
             i = edges.begin(), e = edges.end(); i != e; ++i) {
        o << "  E@" << &(*i) << " {" << std::endl;
        o << "    V@" << (*i).v1 << " - " << "V@" << (*i).v2 << std::endl;
        const std::vector<const face_t *> &faces = connectivity.edge_to_face[edgeToIndex_fast(&(*i))];
        for (size_t j = 0; j < (faces.size() & ~1U); j += 2) {
          o << "      fp: F@" << faces[j] << ", F@" << faces[j+1] << std::endl;
        }
        o << "  }" << std::endl;
      }
      for (std::vector<face_t >::const_iterator
             i = faces.begin(), e = faces.end(); i != e; ++i) {
        o << "  F@" << &(*i) << " {" << std::endl;
        o << "    vertices {" << std::endl;
        for (face_t::const_vertex_iter_t j = (*i).vbegin(), je = (*i).vend(); j != je; ++j) {
          o << "      V@" << (*j) << std::endl;
        }
        o << "    }" << std::endl;
        o << "    edges {" << std::endl;
        for (face_t::const_edge_iter_t j = (*i).ebegin(), je = (*i).eend(); j != je; ++j) {
          o << "      E@" << (*j) << std::endl;
        }
        carve::geom::plane<3> p = (*i).plane_eqn;
        o << "    }" << std::endl;
        o << "    normal " << (*i).plane_eqn.N << std::endl;
        o << "    aabb " << (*i).aabb << std::endl;
        o << "    plane_eqn ";
        carve::geom::operator<< <3>(o, p);
        o << std::endl;
        o << "  }" << std::endl;
      }

      o << "}" << std::endl;
    }



    void Polyhedron::canonicalize() {
      orderVertices();
      for (size_t i = 0; i < faces.size(); i++) {
        face_t &f = faces[i];
        size_t j = std::distance(f.vbegin(),
                                 std::min_element(f.vbegin(),
                                                  f.vend()));
        if (j) {
          {
            std::vector<const vertex_t *> temp;
            temp.reserve(f.nVertices());
            std::copy(f.vbegin() + j, f.vend(),       std::back_inserter(temp));
            std::copy(f.vbegin(),     f.vbegin() + j, std::back_inserter(temp));
            std::copy(temp.begin(),   temp.end(),     f.vbegin());
          }
          {
            std::vector<const edge_t *> temp;
            temp.reserve(f.nEdges());
            std::copy(f.ebegin() + j, f.eend(),       std::back_inserter(temp));
            std::copy(f.ebegin(),     f.ebegin() + j, std::back_inserter(temp));
            std::copy(temp.begin(),   temp.end(),     f.ebegin());
          }
        }
      }

      std::vector<face_t *> face_ptrs;
      face_ptrs.reserve(faces.size());
      for (size_t i = 0; i < faces.size(); ++i) face_ptrs.push_back(&faces[i]);
      std::sort(face_ptrs.begin(), face_ptrs.end(), order_faces());
      std::vector<face_t> sorted_faces;
      sorted_faces.reserve(faces.size());
      for (size_t i = 0; i < faces.size(); ++i) sorted_faces.push_back(*face_ptrs[i]);
      std::swap(faces, sorted_faces);
    }

  }
}

