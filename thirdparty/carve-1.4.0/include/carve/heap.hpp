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
namespace carve {
  namespace heap {



    template<typename random_access_iter_t, typename distance_t, typename value_t, typename pred_t>
    void _down_heap(random_access_iter_t begin,
                    distance_t pos,
                    distance_t len,
                    pred_t pred) {
      typedef typename std::iterator_traits<random_access_iter_t>::value_type value_t;

      const distance_t start = pos;
      distance_t child = pos * 2 + 2;

      value_t v = *(begin + pos);

      while (child < len) {
        if (pred(*(begin + child), *(begin + (child-1)))) child--;
        if (!pred(v, *(begin + child))) break;
        *(begin + pos) = *(begin + child);
        pos = child;
      }
      if (child == len) {
        --child;
        if (pred(v, *(begin + child))) {
          *(begin + pos) = *(begin + child);
          pos = child;
        }
      }
      if (pos != start) *(begin + pos) = v;
    }



    template<typename random_access_iter_t, typename distance_t, typename value_t, typename pred_t>
    void _up_heap(random_access_iter_t begin,
                  distance_t pos,
                  distance_t len,
                  pred_t pred) {
      typedef typename std::iterator_traits<random_access_iter_t>::value_type value_t;

      const distance_t start = pos;
      distance_t parent = (pos - 1) / 2;

      value_t v = *(begin + pos);

      while (pos > 0) {
        if (pred(*(begin + parent), v)) break;
        *(begin + pos) = *(begin + parent);
        pos = parent;
        parent = (parent - 1) / 2;
      }
      if (pos != start) *(begin + pos) = v;
    }



    template<typename random_access_iter_t, typename pred_t>
    void _adjust_heap(random_access_iter_t begin,
                      random_access_iter_t end,
                      random_access_iter_t pos,
                      pred_t pred) {
      typedef typename std::iterator_traits<random_access_iter_t>::difference_type distance_t;

      distance_t parent = ((pos - begin) - 1) / 2;
      if (p != 0 && pred(*(begin + parent), *pos)) {
        _up_heap(begin, pos - begin, end - begin, (pos - begin), pred);
      } else {
        _down_heap(begin, pos - begin, end - begin, (pos - begin), pred);
      }
    }



    template<typename random_access_iter_t>
    void adjust_heap(random_access_iter_t begin,
                     random_access_iter_t end,
                     random_access_iter_t pos) {
      typedef typename std::iterator_traits<random_access_iter_t>::value_type value_t;

      _adjust_heap(begin, end, pos, std::less<value_t>());
    }



    template<typename random_access_iter_t, typename pred_t>
    void adjust_heap(random_access_iter_t begin,
                     random_access_iter_t end,
                     random_access_iter_t pos,
                     pred_t pred) {
      _adjust_heap(begin, end, pos, pred);
    }



    template<typename random_access_iter_t, typename pred_t>
    void _remove_heap(random_access_iter_t begin,
                      random_access_iter_t end,
                      random_access_iter_t pos,
                      pred_t pred) {
      --end;
      if (pos != end) {
        std::swap(*pos,  *end);
        _adjust_heap(begin, end, pos, pred);
      }
    }



    template<typename random_access_iter_t>
    void remove_heap(random_access_iter_t begin,
                     random_access_iter_t end,
                     random_access_iter_t pos) {
      typedef typename std::iterator_traits<random_access_iter_t>::value_type value_t;

      _remove_heap(begin, end, pos, std::less<value_t>());
    }



    template<typename random_access_iter_t, typename pred_t>
    void remove_heap(random_access_iter_t begin,
                     random_access_iter_t end,
                     random_access_iter_t pos,
                     pred_t pred) {
      _remove_heap(begin, end, pos, pred);
    }



  }
}
