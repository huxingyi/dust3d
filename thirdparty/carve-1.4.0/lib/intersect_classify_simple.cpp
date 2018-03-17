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

#include <list>
#include <set>
#include <iostream>

#include <algorithm>

#include "intersect_common.hpp"
#include "intersect_classify_common.hpp"

#if 0

class edge_graph_t :
  public std::hash_map<const Vector *,
                       std::pair<int, std::set<const Vector *> >,
                       hash_vector_ptr> {
public:
  typedef mapped_type data_type;
};

#if defined(CARVE_DEBUG)
static void drawEdgeGraph(edge_graph_t &eg) {
  for (edge_graph_t::const_iterator
         j = eg.begin(), je = eg.end(); j != je; ++j) {
    const Vector *v1 = (*j).first;
    for (std::set<const Vector *>::const_iterator
           k = (*j).second.second.begin(), ke = (*j).second.second.end();
         k != ke;
         ++k) {
      const Vector *v2 = (*k);
      HOOK(drawEdge(v1, v2, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 3.0f););
    }
  }
}
#endif

static bool find_cycles(edge_graph_t &eg, std::list<V2Set> &cycles) {
  for (edge_graph_t::const_iterator
         i = eg.begin(), e = eg.end(); i != e; ++i) {
    if ((*i).second.second.size() != 1) {
#if defined(CARVE_DEBUG)
      // HOOK(drawEdgeGraph(eg););
#endif
      std::cerr << "WARNING:edge loop graph does not consist of simple cycles"
                << std::endl;
      return false;
    }
  }

  while (eg.size()) {
#if defined(CARVE_DEBUG)
    std::cerr << "eg.size(): " << eg.size() << std::endl;
#endif
    std::list<const Vector *> path;
    edge_graph_t::iterator i;
    int c = 0;
    const Vector *v;
    i = eg.begin();
    path.push_back(v = (*i).first);
    (*i).second.first = c++;

    while (1) {
      const Vector *v2 = *((*i).second.second.begin());
      path.push_back(v2);

      i = eg.find(v2);
      if (i == eg.end()) {
        break;
      }
      if ((*i).second.first < c) {
        break;
      }
      (*i).second.first = c++;
    }

#if defined(CARVE_DEBUG)
    std::cerr << (i != eg.end() ? "found" : "didn't find") << " cycle "
              << std::endl
              << "path.size(): " << path.size()
              << std::endl;
#endif
    if (i != eg.end()) {
      c = (*i).second.first;
      cycles.push_back(V2Set());
    }

    const Vector *src = NULL, *tgt = NULL;

    for (std::list<const Vector *>::const_iterator
           j = path.begin(), je = path.end(); j != je; ++j) {
      tgt = (*j);
      if (src) {
        edge_graph_t::iterator k = eg.find(src);
        if ((*k).second.first >= c) {
          cycles.back().insert(std::make_pair(src, tgt));
        }
        (*k).second.first = INT_MAX;
        (*k).second.second.erase(tgt);
        if ((*k).second.second.size() == 0) {
          eg.erase(k);
        }
      }
      src = tgt;
    }
  }
  return true;
}

static bool eliminate_simple(const LoopEdges &edge_map,
                             V2Set &edges,
                             std::set<FaceLoop *> &loops,
                             V2Set &extra_edges) {
  const Vector *v1, *v2;
  loops.clear();

#if defined(CARVE_DEBUG)
  std::cerr << edges.size() << " edges to eliminate" << std::endl;
#endif

  while (edges.size()) {
    LoopEdges::const_iterator i = edge_map.find(*(edges.begin()));
    if (i == edge_map.end()) {
      std::pair<const Vector *, const Vector *> t = *(edges.begin());
      return false;
    }
    if ((*i).second.size() != 1) {
      throw intersect_exception(
        "eliminate_simple failed: topology too complex");
    }

    FaceLoop *to_remove = *((*i).second.begin());
#if defined(CARVE_DEBUG)
    std::cerr << "  removing face loop " << to_remove << std::endl;
#endif
    loops.insert(to_remove);

    v1 = to_remove->vertices[to_remove->vertices.size() - 1];
    for (unsigned j = 0; j < to_remove->vertices.size(); ++j) {
      v2 = to_remove->vertices[j];

      V2Set::iterator v = edges.find(std::make_pair(v1, v2));
      if (v != edges.end()) {
#if defined(CARVE_DEBUG)
        std::cerr << "    removing edge " << v1 << "-" << v2 << std::endl;
#endif
        edges.erase(v);
      } else {
#if defined(CARVE_DEBUG)
        std::cerr << "    adding edge " << v2 << "-" << v1 << std::endl;
#endif
        edges.insert(std::make_pair(v2, v1));
        extra_edges.insert(std::make_pair(std::min(v1, v2), 
                                          std::max(v1, v2)));
      }

      v1 = v2;
    }
#if defined(CARVE_DEBUG)
    std::cerr << "  " << edges.size() << " edges remain." << std::endl;
#endif
  }

#if defined(CARVE_DEBUG)
  std::cerr << "eliminate succeeded" << std::endl;
#endif

  return true;
}

static void classifyEdges(const V2Set &edges,
                          const Polyhedron *poly,
                          int &n_in, int &n_on, int &n_out) {

  n_in = n_on = n_out = 0;
  for (V2Set::const_iterator
         i = edges.begin(), e = edges.end(); i != e; ++i) {
    Vector c((*(*i).first + *(*i).second) / 2.0);
    switch (poly->containsVertex(c, NULL)) {
    case POINT_IN:  n_in++;  break;
    case POINT_OUT: n_out++; break;
    case POINT_ON:  n_on++;  break;
    default:                 break;
    }
  }
}

static FaceClass faceClassificationBasedOnEdges(const V2Set &edges,
                                                const Polyhedron *poly,
                                                FaceClass on_hint) {
  int n_in, n_on, n_out;

  classifyEdges(edges, poly, n_in, n_on, n_out);

  std::cerr << ">>> classification: n_in: " << n_in << " n_on: " << n_on << " n_out: " << n_out << std::endl;
  CARVE_ASSERT(n_in  == (int)edges.size() || n_out == (int)edges.size() || n_on  == (int)edges.size());

  if (n_in) return FACE_IN;
  if (n_out) return FACE_OUT;

  return on_hint;
}

static void classifySimpleOnFaces(FaceLoopList &a_face_loops,
                                  FaceLoopList &b_face_loops, 
                                  const Polyhedron *poly_a,
                                  const Polyhedron *poly_b,
                                  CSG::Collector &collector) { 
  LoopEdges edge_map;

  for (FaceLoop *i = b_face_loops.head; i; i = i->next) {
    edge_map.addFaceLoop(i);
  }

  for (FaceLoop *i = a_face_loops.head; i;) {
    LoopEdges::const_iterator t;
    t = edge_map.find(std::make_pair(i->vertices[0], i->vertices[1]));
    if (t != edge_map.end()) {
      for (std::list<FaceLoop *>::const_iterator
             u = (*t).second.begin(), ue = (*t).second.end(); u != ue; ++u) {
        FaceLoop *j(*u);
        int k = is_same(i->vertices, j->vertices);
        CARVE_ASSERT(k != -1);
        if (k == +1) {
          collector.collect(i->orig_face,
                            i->vertices,
                            i->orig_face->normal,
                            true,
                            FACE_ON_ORIENT_OUT);
          collector.collect(j->orig_face,
                            j->vertices,
                            j->orig_face->normal,
                            false,
                            FACE_ON_ORIENT_OUT);
          edge_map.removeFaceLoop(j);
          i = a_face_loops.remove(i);
          b_face_loops.remove(j);
          goto next_loop;
        }
      }
    }
    t = edge_map.find(std::make_pair(i->vertices[1], i->vertices[0]));
    if (t != edge_map.end()) {
      for (std::list<FaceLoop *>::const_iterator
             u = (*t).second.begin(), ue = (*t).second.end(); u != ue; ++u) {
        FaceLoop *j(*u);
        int k = is_same(i->vertices, j->vertices);
        CARVE_ASSERT(k != +1);
        if (k == -1) {
          collector.collect(i->orig_face,
                            i->vertices,
                            i->orig_face->normal,
                            true,
                            FACE_ON_ORIENT_IN);
          collector.collect(j->orig_face,
                            j->vertices,
                            j->orig_face->normal,
                            false,
                            FACE_ON_ORIENT_IN);
          edge_map.removeFaceLoop(j);
          i = a_face_loops.remove(i);
          b_face_loops.remove(j);
          goto next_loop;
        }
      }
    }
    i = i->next;
  next_loop:;
  }
}

static void classifyOnFaces_2(FaceLoopList &a_face_loops,
                              FaceLoopList &b_face_loops, 
                              const VertexClassification &vclass,
                              const Polyhedron *poly_a,
                              const Polyhedron *poly_b,
                              CSG::Collector &collector) {

  LoopEdges edge_map_a;
  LoopEdges edge_map_b;

  std::list<V2Set> cycles;

  for (FaceLoop *i = a_face_loops.head; i; i = i->next) {
    edge_map_a.addFaceLoop(i);
  }

  for (FaceLoop *i = b_face_loops.head; i; i = i->next) {
    edge_map_b.addFaceLoop(i);
  }

  edge_graph_t edge_graph;

  edge_graph.clear();
  for (LoopEdges::const_iterator
         i = edge_map_a.begin(), e = edge_map_a.end();
       i != e;
       ++i) {
    const Vector *v1((*i).first.first), *v2((*i).first.second);
    LoopEdges::const_iterator j = edge_map_b.find(std::make_pair(v2, v1));
    if (j != edge_map_b.end()) {
      edge_graph_t::data_type &data(edge_graph[v1]);
      data.first = INT_MAX;
      data.second.insert(v2);
#if defined(CARVE_DEBUG)
      std::cerr << "--- " << v1 << "-" << v2 << std::endl;
#endif
    }
  }
  
  cycles.clear();
  if (!find_cycles(edge_graph, cycles)) return;

#if defined(CARVE_DEBUG)
  std::cerr << cycles.size() << " reverse cycles" << std::endl;
#endif

  for (std::list<V2Set>::iterator
         i = cycles.begin(), e = cycles.end(); i != e; ++i) {
    std::set<FaceLoop *> a_loopset, b_loopset;
    V2Set b_cycle;
    V2Set a_extra_edges, b_extra_edges;

    for (V2Set::const_iterator
           j = (*i).begin(), je = (*i).end(); j != je; ++j) {
      b_cycle.insert(std::make_pair((*j).second, (*j).first));
    }
    if (eliminate_simple(edge_map_a, (*i), a_loopset, a_extra_edges) &&
        eliminate_simple(edge_map_b, b_cycle, b_loopset, b_extra_edges)) {
#if defined(CARVE_DEBUG)
      std::cerr << "paired "
                << a_loopset.size() << " poly_a faces with "
                << b_loopset.size() << " poly_b faces in reverse loop ("
                << a_extra_edges.size() << " extra edges in set a, "
                << b_extra_edges.size() << " extra edges in set b)"
                << std::endl; 
#endif
      FaceClass a_class, b_class;

      a_class = faceClassificationBasedOnEdges(a_extra_edges,
                                               poly_b,
                                               FACE_ON_ORIENT_IN);
      b_class = faceClassificationBasedOnEdges(b_extra_edges,
                                               poly_a,
                                               FACE_ON_ORIENT_IN);

#if defined(CARVE_DEBUG)
      std::cerr << "a_class = " << ENUM(a_class) << std::endl
                << "b_class = " << ENUM(b_class) << std::endl;
#endif

      if (a_class == FACE_ON_ORIENT_IN && b_class != FACE_ON_ORIENT_IN)
        a_class = b_class;
      else if (a_class != FACE_ON_ORIENT_IN && b_class == FACE_ON_ORIENT_IN)
        b_class = a_class;

#if defined(CARVE_DEBUG)
      std::cerr << "modified a_class = " << ENUM(a_class) << std::endl
                << "         b_class = " << ENUM(b_class) << std::endl;
#endif

      for (std::set<FaceLoop *>::const_iterator
             j = a_loopset.begin(), je = a_loopset.end(); j != je; ++j) {
        collector.collect((*j)->orig_face,
                          (*j)->vertices,
                          (*j)->orig_face->normal,
                          true,
                          a_class);
        edge_map_a.removeFaceLoop((*j));
        a_face_loops.remove((*j));
      }
      for (std::set<FaceLoop *>::const_iterator
             j = b_loopset.begin(), je = b_loopset.end(); j != je; ++j) {
        collector.collect((*j)->orig_face,
                          (*j)->vertices,
                          (*j)->orig_face->normal,
                          false,
                          a_class);
        edge_map_b.removeFaceLoop((*j));
        b_face_loops.remove((*j));
      }
    } else {
#if defined(CARVE_DEBUG)
      std::cerr << "pairing failed" << std::endl;
#endif
   }
  }

  edge_graph.clear();
  for (LoopEdges::const_iterator
         i = edge_map_a.begin(), e = edge_map_a.end();
       i != e;
       ++i) {
    const Vector *v1((*i).first.first), *v2((*i).first.second);
    LoopEdges::const_iterator j = edge_map_b.find(std::make_pair(v1, v2));
    if (j != edge_map_b.end()) {
      edge_graph_t::data_type &data(edge_graph[v1]);
      data.first = INT_MAX;
      data.second.insert(v2);
#if defined(CARVE_DEBUG)
      std::cerr << "+++ " << v1 << "-" << v2 << std::endl;
#endif
    }
  }

  cycles.clear();
  if (!find_cycles(edge_graph, cycles)) return;

#if defined(CARVE_DEBUG)
  std::cerr << cycles.size() << " forward cycles" << std::endl;
#endif

  for (std::list<V2Set>::iterator
         i = cycles.begin(), e = cycles.end(); i != e; ++i) {
    std::set<FaceLoop *> a_loopset, b_loopset;
    V2Set b_cycle((*i));
    V2Set a_extra_edges, b_extra_edges;

    if (eliminate_simple(edge_map_a, (*i), a_loopset, a_extra_edges) &&
        eliminate_simple(edge_map_b, b_cycle, b_loopset, b_extra_edges)) {
#if defined(CARVE_DEBUG)
      std::cerr << "paired "
                << a_loopset.size() << " poly_a faces with "
                << b_loopset.size() << " poly_b faces in forward loop"
                << a_extra_edges.size() << " extra edges in set a, "
                << b_extra_edges.size() << " extra edges in set b)"
                << std::endl; 
#endif
      FaceClass a_class, b_class;

      a_class = faceClassificationBasedOnEdges(a_extra_edges,
                                               poly_b,
                                               FACE_ON_ORIENT_OUT);
      b_class = faceClassificationBasedOnEdges(b_extra_edges,
                                               poly_a,
                                               FACE_ON_ORIENT_OUT);
#if defined(CARVE_DEBUG)
      std::cerr << "a_class = " << ENUM(a_class) << std::endl
                << "b_class = " << ENUM(b_class) << std::endl;
#endif

      if (a_class == FACE_ON_ORIENT_OUT && b_class != FACE_ON_ORIENT_OUT)
        a_class = (FaceClass)-b_class;
      else if (a_class != FACE_ON_ORIENT_OUT && b_class == FACE_ON_ORIENT_OUT)
        b_class = (FaceClass)-a_class;

#if defined(CARVE_DEBUG)
      std::cerr << "modified a_class = " << ENUM(a_class) << std::endl
                << "         b_class = " << ENUM(b_class) << std::endl;
#endif

      for (std::set<FaceLoop *>::const_iterator
             j = a_loopset.begin(), je = a_loopset.end(); j != je; ++j) {
        collector.collect((*j)->orig_face,
                          (*j)->vertices,
                          (*j)->orig_face->normal,
                          true,
                          a_class);
        edge_map_a.removeFaceLoop((*j));
        a_face_loops.remove((*j));
      }
      for (std::set<FaceLoop *>::const_iterator
             j = b_loopset.begin(), je = b_loopset.end(); j != je; ++j) {
        collector.collect((*j)->orig_face,
                          (*j)->vertices,
                          (*j)->orig_face->normal,
                          false,
                          b_class);
        edge_map_b.removeFaceLoop((*j));
        b_face_loops.remove((*j));
      }
    } else {
#if defined(CARVE_DEBUG)
      std::cerr << "pairing failed" << std::endl;
#endif
    }
  }
}

void classifyEasyFaces(FaceLoopList &face_loops,
                       VertexClassification &vclass,
                       const  Polyhedron *other_poly,
                       int other_poly_num,
                       Intersections &intersections,
                       CSG::Collector &collector) {


  for (FaceLoop *i = face_loops.head; i;) {
    unsigned j;

    const std::vector<const Vector *> &f_loop(i->vertices);

    PointClass pc = POINT_ON;
    unsigned test = 0;
    for (j = 0; j < f_loop.size(); j++) {
      PointClass temp = vclass[f_loop[j]].cls[other_poly_num];
      if (temp == POINT_ON) continue;
      pc = temp;
      test = j;
      if (pc != POINT_UNK) break;
    }

    if (pc != POINT_ON) {
      // EASY CASE

      // IF a face uses a POINT_OUT vertex, it is OUT, and no face
      // vertices may be POINT_IN
      
      // IF a face uses a POINT_IN vertex, it is IN, and no face
      // vertices may be POINT_OUT

      if (pc == POINT_UNK) {
        pc = other_poly->containsVertex(*f_loop[test]);
#if defined(CARVE_DEBUG)
        std::cerr << "testing " << f_loop[test] << " pc = " << pc << std::endl;
#endif

#if defined(CARVE_DEBUG)
        if (pc != POINT_IN && pc != POINT_OUT) {
          HOOK(drawFaceLoop(f_loop, i->orig_face->normal, 1.0, 0.0, 0.0, 0.5, true););
          HOOK(drawFaceLoopWireframe(f_loop, i->orig_face->normal, 1.0, 1.0, 0.0, 1.0););
          HOOK(drawPoint(f_loop[test], 1, 1, 1, 1, 8.0););
        }
#endif
        CARVE_ASSERT(pc == POINT_OUT || pc == POINT_IN);
      }

      for (j = 0; j < f_loop.size(); j++) {
        PC2 &pc2(vclass[f_loop[j]]);
        if (pc2.cls[other_poly_num] == POINT_UNK) {
          pc2.cls[other_poly_num] = pc;
        } else {
          CARVE_ASSERT(pc2.cls[other_poly_num] == POINT_ON || pc2.cls[other_poly_num] == pc);
        }
      }

      // this face loop is trivially either IN or OUT.

      {
        std::vector<const Face *> ifaces;
        intersections.commonFaces(f_loop, ifaces);
        if (ifaces.size()) {
          std::cerr << "ERROR: JUST CLASSIFIED A FACE AS IN OR OUT WHEN "
            "ifaces.size() == " << ifaces.size() << " [ ";

          for (std::vector<const Face *>::const_iterator
                 x = ifaces.begin(), xe = ifaces.end(); x != xe; ++x) {
            std::cerr << (*x) << " ";
          }
          std::cerr << "]" << std::endl;
        }
      }

      if (pc == POINT_IN) {
        collector.collect(i->orig_face,
                          f_loop,
                          i->orig_face->normal,
                          other_poly_num == 1,
                          FACE_IN);
      } else {
        collector.collect(i->orig_face,
                          f_loop,
                          i->orig_face->normal,
                          other_poly_num == 1,
                          FACE_OUT);
      }

      i = face_loops.remove(i);
    } else {
      i = i ->next;
    }
  }
}
#endif

void carve::csg::CSG::classifyFaceGroupsSimple(const V2Set &shared_edges,
                                               VertexClassification &vclass,
                                               const carve::poly::Polyhedron  *poly_a,                           
                                               FLGroupList &a_loops_grouped,
                                               const LoopEdges &a_edge_map,
                                               const carve::poly::Polyhedron  *poly_b,
                                               FLGroupList &b_loops_grouped,
                                               const LoopEdges &b_edge_map,
                                               Collector &collector) {
#if 0
  // Old proto
  void carve::csg::CSG::classifyFaces(carve::csg::FaceLoopList &a_face_loops,
                                      size_t a_edge_count,
                                      carve::csg::FaceLoopList &b_face_loops, 
                                      size_t b_edge_count,
                                      VertexClassification &vclass,
                                      const carve::poly::Polyhedron *poly_a,
                                      const carve::poly::Polyhedron *poly_b,
                                      Collector &collector);

  std::cerr << "classify:  "
            << a_face_loops.size() << " cases in polyhedron a"
            << std::endl;
  std::cerr << "classify:  "
            << b_face_loops.size() << " cases in polyhedron b"
            << std::endl;

  classifyEasyFaces(a_face_loops, vclass, poly_b, 1, intersections, collector);
  classifyEasyFaces(b_face_loops, vclass, poly_a, 0, intersections, collector);

#if 0 && defined(CARVE_DEBUG)
  HOOK(drawFaceLoopList(a_face_loops, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f););
  HOOK(drawFaceLoopList(b_face_loops, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f););
#endif

  std::cerr << "classify:  "
            << a_face_loops.size() << " hard cases in polyhedron a"
            << std::endl;
  std::cerr << "classify:  "
            << b_face_loops.size() << " hard cases in polyhedron b"
            << std::endl;

  classifySimpleOnFaces(a_face_loops, b_face_loops, poly_a, poly_b, collector);

  std::cerr << "classify:  "
            << a_face_loops.size()
            << " after elimination of easy pairs: polyhedron a" << std::endl;
  std::cerr << "classify:  "
            << b_face_loops.size()
            << " after elimination of easy pairs: polyhedron b" << std::endl;

  classifyOnFaces_2(a_face_loops,
                    b_face_loops,
                    vclass, poly_a,
                    poly_b,
                    collector);

  std::cerr << "classify:  " << a_face_loops.size() << " after elimination of hard pairs: polyhedron a" << std::endl;
  std::cerr << "classify:  " << b_face_loops.size() << " after elimination of hard pairs: polyhedron b" << std::endl;

  FaceLoop *fla, *flb;

  for (fla = a_face_loops.head; fla; fla = fla->next) {
    const Face *f(fla->orig_face);
    std::vector<const Vector *> &loop(fla->vertices);
    std::vector<P2> proj;
    proj.reserve(loop.size());
    for (unsigned i = 0; i < loop.size(); ++i) {
      proj.push_back((f->*(f->project))(*loop[i]));
    }
    P2 pv;
    if (!pickContainedPoint(proj, pv)) {
      CARVE_FAIL("pickContainedPoint failed");
    }
    Vector v = (f->*(f->unproject))(pv);

    const Face *hit_face;
    PointClass pc = poly_b->containsVertex(v, &hit_face);

    FaceClass fc = FACE_ON;

    if (pc != POINT_IN && pc != POINT_OUT) {
      std::cerr << "WARNING: last resort classifier found an ON face"
                << std::endl;
    }

    switch (pc) {
    case POINT_IN:  fc = FACE_IN; break;
    case POINT_OUT: fc = FACE_OUT; break;
    case POINT_ON: {
      double d = dot(hit_face->normal, f->normal);
      fc = d < 0.0 ? FACE_ON_ORIENT_IN : FACE_ON_ORIENT_OUT;
      break;
    }
    default:
      CARVE_FAIL("should not happen");
    }

    // CARVE_ASSERT(pc == POINT_IN || pc == POINT_OUT);
#if defined(CARVE_DEBUG)
    {
      float r,g,b,a;
      switch(fc) {
      case FACE_IN:            r=1; g=0; b=0; a=1; break;
      case FACE_OUT:           r=0; g=0; b=1; a=1; break;
      case FACE_ON_ORIENT_OUT: r=1; g=1; b=0; a=1; break;
      case FACE_ON_ORIENT_IN:  r=0; g=1; b=1; a=1; break;
      }
      if (fc != FACE_IN && fc != FACE_OUT) {
        HOOK(drawFaceLoopWireframe(loop, f->normal, r, g, b, a););
        HOOK(drawPoint(&v, 1, 1, 1, 1, 30.0););
      }
    }
#endif

    collector.collect(f, loop, f->normal, true, fc);
  }

  for (flb = b_face_loops.head; flb; flb = flb->next) {
    const Face *f(flb->orig_face);
    std::vector<const Vector *> &loop(flb->vertices);
    std::vector<P2> proj;
    proj.reserve(loop.size());
    for (unsigned i = 0; i < loop.size(); ++i) {
      proj.push_back((f->*(f->project))(*loop[i]));
    }
    P2 pv;
    if (!pickContainedPoint(proj, pv)) {
      CARVE_FAIL("pickContainedPoint failed");
    }
    Vector v = (f->*(f->unproject))(pv);

    const Face *hit_face;
    PointClass pc = poly_a->containsVertex(v, &hit_face);

    FaceClass fc = FACE_ON;

    if (pc != POINT_IN && pc != POINT_OUT) {
      std::cerr << "WARNING: last resort classifier found an ON face"
                << std::endl;
    }

    switch (pc) {
    case POINT_IN:  fc = FACE_IN; break;
    case POINT_OUT: fc = FACE_OUT; break;
    case POINT_ON: {
      double d = dot(hit_face->normal, f->normal);
      fc = d < 0.0 ? FACE_ON_ORIENT_IN : FACE_ON_ORIENT_OUT;
      break;
    }
    default:
      CARVE_FAIL("should not happen");
    }

    // CARVE_ASSERT(pc == POINT_IN || pc == POINT_OUT);
#if defined(CARVE_DEBUG)
    {
      float r,g,b,a;
      switch(fc) {
      case FACE_IN:            r=1; g=0; b=0; a=1; break;
      case FACE_OUT:           r=0; g=0; b=1; a=1; break;
      case FACE_ON_ORIENT_OUT: r=1; g=1; b=0; a=1; break;
      case FACE_ON_ORIENT_IN:  r=0; g=1; b=1; a=1; break;
      }
      // HOOK(drawFaceLoop(loop, f->normal, r, g, b, a););
      if (fc != FACE_IN && fc != FACE_OUT) {
        HOOK(drawFaceLoopWireframe(loop, f->normal, r, g, b, a););
        HOOK(drawPoint(&v, 1, 1, 1, 1, 30.0););
      }
    }
#endif

    collector.collect(f, loop, f->normal, false, fc);
  }
#endif
}
