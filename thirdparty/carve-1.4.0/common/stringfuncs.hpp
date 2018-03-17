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

#include <string>

namespace str {

  static inline bool startswith(const std::string &a, const std::string &b) {
    if (b.size() > a.size()) return false;
    return a.compare(0, b.size(), b) == 0;
  }

  template<typename strip_t>
  static inline std::string rstrip(const std::string &a, const strip_t stripchars) {
    std::string::size_type p = a.find_last_not_of(stripchars);
    if (p == std::string::npos) return "";
    return a.substr(0, p + 1);
  }

  static inline std::string rstrip(const std::string &a) {
    std::string::size_type p = a.size();
    while (p && isspace(a[p-1])) p--;
    if (!p) return "";
    return a.substr(0, p);
  }

  template<typename strip_t>
  static inline std::string lstrip(const std::string &a, const strip_t stripchars) {
    std::string::size_type p = a.find_first_not_of(stripchars);
    if (p == std::string::npos) return "";
    return a.substr(p);
  }

  static inline std::string lstrip(const std::string &a) {
    std::string::size_type p = 0;
    while (p < a.size() && isspace(a[p])) p++;
    if (p == a.size()) return "";
    return a.substr(p);
  }

  template<typename strip_t>
  static inline std::string strip(const std::string &a, const strip_t stripchars) {
    std::string::size_type p = a.find_first_not_of(stripchars);
    if (p == std::string::npos) return "";
    std::string::size_type q = a.find_last_not_of(stripchars);
    return a.substr(p, q-p);
  }

  static inline std::string strip(const std::string &a) {
    std::string::size_type p = 0;
    while (p < a.size() && isspace(a[p])) p++;
    if (p == a.size()) return "";
    std::string::size_type q = a.size();
    while (q>p && isspace(a[q-1])) q--;
    return a.substr(p, q-p);
  }
}
