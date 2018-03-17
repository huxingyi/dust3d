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

#include <functional>

struct cRGB {
  typedef float value_type;
  value_type r, g, b;

  cRGB() : r(0), g(0), b(0) { }

  template <typename T>
  cRGB(T _r, T _g, T _b) : r((value_type)_r), g((value_type)_g), b((value_type)_b) { }
};

struct cRGBA {
  typedef float value_type;
  value_type r, g, b, a;

  cRGBA() : r(0),g(0),b(0),a(1) { }
  template <typename T>
  cRGBA(T _r, T _g, T _b, T _a = T(1)) : r((value_type)_r), g((value_type)_g), b((value_type)_b), a((value_type)_a) { }

  cRGBA(const cRGB &rgb) : r(rgb.r), g(rgb.g), b(rgb.b), a(1) { }
};

static inline cRGB operator+(const cRGB &a, const cRGB &b) {
  return cRGB(a.r + b.r, a.g + b.g, a.b + b.b);
}
static inline cRGB &operator+=(cRGB &a, const cRGB &b) {
  a.r += b.r; a.g += b.g; a.b += b.b;
  return a;
}

static inline cRGB operator*(double s, const cRGB &a) {
  return cRGB(s * a.r, s * a.g, s * a.b);
}

static inline cRGBA operator+(const cRGBA &a, const cRGBA &b) {
  return cRGBA(a.r + b.r, a.g + b.g, a.b + b.b, a.a + b.a);
}
static inline cRGBA &operator+=(cRGBA &a, const cRGBA &b) {
  a.r += b.r; a.g += b.g; a.b += b.b; a.a += b.a;
  return a;
}

static inline cRGBA operator*(double s, const cRGBA &a) {
  return cRGBA(s * a.r, s * a.g, s * a.b, s * a.a);
}

static inline cRGB HSV2RGB(float H, float S, float V) {
  H = 6.0f * H;
  if (S < 5.0e-6) {
    cRGB(V, V, V);
  } else {
    int i = (int)H;
    float f = H - i;
    float p1 = V * (1.0f - S);
    float p2 = V * (1.0f - S * f);
    float p3 = V * (1.0f - S * (1.0f - f));
    switch (i) {
    case 0: return cRGB(V, p3, p1);
    case 1: return cRGB(p2,  V, p1);
    case 2: return cRGB(p1,  V, p3);
    case 3: return cRGB(p1, p2,  V);
    case 4: return cRGB(p3, p1,  V);
    case 5: return cRGB(V, p1, p2);
    }
  }
  return cRGB(0, 0, 0);
}

struct colour_clamp_t {
  cRGB operator()(const cRGB &c) const {
    return cRGB(std::min(std::max(c.r, 0.0f), 1.0f),
                std::min(std::max(c.g, 0.0f), 1.0f),
                std::min(std::max(c.b, 0.0f), 1.0f));
  }
  cRGBA operator()(const cRGBA &c) const {
    return cRGBA(std::min(std::max(c.r, 0.0f), 1.0f),
                 std::min(std::max(c.g, 0.0f), 1.0f),
                 std::min(std::max(c.b, 0.0f), 1.0f),
                 std::min(std::max(c.a, 0.0f), 1.0f));
  }
};
