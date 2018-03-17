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
#include <carve/debug_hooks.hpp>

#include <list>
#include <set>
#include <iostream>

#include <algorithm>

#include "intersect_common.hpp"
#include "intersect_classify_common.hpp"
#include "intersect_classify_common_impl.hpp"

typedef carve::poly::Polyhedron poly_t;

namespace carve {
  namespace csg {

    namespace {
      struct GroupPoly : public CSG::Collector {
        const poly_t *want_groups_from;
        std::list<std::pair<FaceClass, poly_t *> > &out;

        struct face_data_t {
          poly_t::face_t *face;
          const poly_t::face_t *orig_face;
          bool flipped;
          face_data_t(poly_t::face_t *_face,
                      const poly_t::face_t *_orig_face,
                      bool _flipped) : face(_face), orig_face(_orig_face), flipped(_flipped) {
          };
        };

        GroupPoly(const poly_t *poly,
                  std::list<std::pair<FaceClass, poly_t *> > &_out) :
            CSG::Collector(),
            want_groups_from(poly),
            out(_out) {
        }

        virtual ~GroupPoly() {
        }

        virtual void collect(FaceLoopGroup *grp, CSG::Hooks &hooks) {
          if (grp->face_loops.head->orig_face->owner != want_groups_from) return;

          std::list<ClassificationInfo> &cinfo = (grp->classification);
          if (cinfo.size() == 0) {
            std::cerr << "WARNING! group " << grp << " has no classification info!" << std::endl;
            return;
          }
          // XXX: check all the cinfo elements for consistency.
          FaceClass fc = cinfo.front().classification;

          std::list<face_data_t> faces;
          std::vector<poly_t::face_t *> new_faces;
          for (FaceLoop *loop = grp->face_loops.head; loop != NULL; loop = loop->next) {
            new_faces.clear();
            new_faces.push_back(loop->orig_face->create(loop->vertices, false));
            hooks.processOutputFace(new_faces, loop->orig_face, false);
            for (size_t i = 0; i < new_faces.size(); ++i) {
              faces.push_back(face_data_t(new_faces[i], loop->orig_face, false));
            }
          }

          std::vector<poly_t::face_t> f;
          f.reserve(faces.size());
          for (std::list<face_data_t>::iterator i = faces.begin(); i != faces.end(); ++i) {
            f.push_back(poly_t::face_t());
            std::swap(f.back(), *(*i).face);
            delete (*i).face;
            (*i).face = &f.back();
          }

          std::vector<poly_t::vertex_t> vertices;
          carve::csg::VVMap vmap;

          poly_t::collectFaceVertices(f, vertices, vmap);

          poly_t *p = new poly_t(f, vertices);

          if (hooks.hasHook(carve::csg::CSG::Hooks::RESULT_FACE_HOOK)) {
            for (std::list<face_data_t>::iterator i = faces.begin(); i != faces.end(); ++i) {
              hooks.resultFace((*i).face, (*i).orig_face, (*i).flipped);
            }
          }

          out.push_back(std::make_pair(fc, p));
        }

        virtual poly_t *done(CSG::Hooks &hooks) {
          return NULL;
        }
      };

      class FaceMaker {
      public:

        bool pointOn(VertexClassification &vclass, FaceLoop *f, size_t index) const {
          return vclass[f->vertices[index]].cls[0] == POINT_ON;
        }

        void explain(FaceLoop *f, size_t index, PointClass pc) const {
#if defined(CARVE_DEBUG)
          std::cerr << "face loop " << f << " from poly b is easy because vertex " << index << " (" << f->vertices[index]->v << ") is " << ENUM(pc) << std::endl;
#endif
        }
      };

      class HalfClassifyFaceGroups {
      public:
        std::list<std::pair<FaceClass, poly_t *> > &b_out;
        CSG::Hooks &hooks;

        HalfClassifyFaceGroups(std::list<std::pair<FaceClass, poly_t *> > &c, CSG::Hooks &h) : b_out(c), hooks(h) {
        }

        void classifySimple(FLGroupList &a_loops_grouped,
                            FLGroupList &b_loops_grouped,
                            VertexClassification &vclass,
                            const poly_t *poly_a,
                            const poly_t *poly_b) const {
          GroupPoly group_poly(poly_b, b_out);
          performClassifySimpleOnFaceGroups(a_loops_grouped, b_loops_grouped, poly_a, poly_b, group_poly, hooks);
#if defined(CARVE_DEBUG)
          std::cerr << "after removal of simple on groups: " << b_loops_grouped.size() << " b groups" << std::endl;
#endif
        }

        void classifyEasy(FLGroupList &a_loops_grouped,
                          FLGroupList &b_loops_grouped,
                          VertexClassification &vclass,
                          const poly_t *poly_a,
                          const poly_t *poly_b) const {
          GroupPoly group_poly(poly_b, b_out);
          performClassifyEasyFaceGroups(b_loops_grouped, poly_a, vclass, FaceMaker(), group_poly, hooks);
#if defined(CARVE_DEBUG)
          std::cerr << "after removal of easy groups: " << b_loops_grouped.size() << " b groups" << std::endl;
#endif
        }

        void classifyHard(FLGroupList &a_loops_grouped,
                          FLGroupList &b_loops_grouped,
                          VertexClassification &vclass,
                          const poly_t *poly_a,
                          const poly_t *poly_b) const {
          GroupPoly group_poly(poly_b, b_out);
          performClassifyHardFaceGroups(b_loops_grouped, poly_a, FaceMaker(), group_poly, hooks);
#if defined(CARVE_DEBUG)
          std::cerr << "after removal of hard groups: " << b_loops_grouped.size() << " b groups" << std::endl;
#endif

        }

        void faceLoopWork(FLGroupList &a_loops_grouped,
                          FLGroupList &b_loops_grouped,
                          VertexClassification &vclass,
                          const poly_t *poly_a,
                          const poly_t *poly_b) const {
          GroupPoly group_poly(poly_b, b_out);
          performFaceLoopWork(poly_a, b_loops_grouped, *this, group_poly, hooks);
        }

        void postRemovalCheck(FLGroupList &a_loops_grouped,
                              FLGroupList &b_loops_grouped) const {
#if defined(CARVE_DEBUG)
          std::cerr << "after removal of on groups: " << b_loops_grouped.size() << " b groups" << std::endl;
#endif
        }

        bool faceLoopSanityChecker(FaceLoopGroup &i) const {
          return false;
          return i.face_loops.size() != 1;
        }

        void finish(FLGroupList &a_loops_grouped,FLGroupList &b_loops_grouped) const {
#if defined(CARVE_DEBUG)
          if (a_loops_grouped.size() || b_loops_grouped.size())
            std::cerr << "UNCLASSIFIED! a=" << a_loops_grouped.size() << ", b=" << b_loops_grouped.size() << std::endl;
#endif
        }
      };
    }

    void CSG::halfClassifyFaceGroups(const V2Set &shared_edges,
                                     VertexClassification &vclass,
                                     const poly_t *poly_a,
                                     FLGroupList &a_loops_grouped,
                                     const detail::LoopEdges &a_edge_map,
                                     const poly_t *poly_b,
                                     FLGroupList &b_loops_grouped,
                                     const detail::LoopEdges &b_edge_map,
                                     std::list<std::pair<FaceClass, poly_t *> > &b_out) {
      HalfClassifyFaceGroups classifier(b_out, hooks);
      GroupPoly group_poly(poly_b, b_out);
      performClassifyFaceGroups(a_loops_grouped, b_loops_grouped, vclass, poly_a, poly_b, classifier, group_poly, hooks);
    }

  }
}
