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

#pragma once

#include "vector.hpp"

namespace gloop {

  struct M3;
  struct M4;

  struct QUAT {
    float x, y, z, w;

    QUAT() { }
    QUAT(const V3 &axis, float angle);
    QUAT(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w) { }

    QUAT &conjugate() {
      x = -x; y = -y; z = -z; return *this;
    }
    
    QUAT &invert() {
      float norme = sqrtf(x*x + y*y + z*z + w*w);
      if (norme == 0.0f) norme = 1.0f;
      float recip = 1.0f / norme;
      x = -x * recip;
      y = -y * recip;
      z = -z * recip;
      w = +w * recip;
      return *this;
    }

    QUAT &normalize() {
      float norme = sqrtf(x*x + y*y + z*z + w*w);
      if (norme == 0.0f) {
        x = y = z = 0.0f;
        w = 1.0f;
      } else {
        float recip = 1.0f / norme;
        x *= recip;
        y *= recip;
        z *= recip;
        w *= recip;
      }
      return *this;
    }
    
    operator M3();
    operator M4();
    
    static QUAT slerp(const QUAT &a, const QUAT &b, float t);
  };


  static inline QUAT operator*(const QUAT &a, const QUAT &b) {
    return QUAT((a.w * b.x) + (a.x * b.w) + (a.y * b.z) - (a.z * b.y),
                (a.w * b.y) - (a.x * b.z) + (a.y * b.w) + (a.z * b.x),
                (a.w * b.z) + (a.x * b.y) - (a.y * b.x) + (a.z * b.w),
                (a.w * b.w) - (a.x * b.x) - (a.y * b.y) - (a.z * b.z));
  }
  static inline QUAT &operator*=(QUAT &a, const QUAT &b) { a = a * b; return a; }

  static inline QUAT &operator*=(QUAT &a, float b) {
    a.x *= b; a.y *= b; a.z *= b; a.w *= b; return a;
  }
  static inline QUAT operator*(const QUAT &a, float b) { QUAT r(a); return r *= b; }

  static inline QUAT operator~(const QUAT &a) { QUAT r(a); return r.conjugate(); }
  static inline QUAT operator-(const QUAT &a) { QUAT r(a); return r.invert(); }

  QUAT slerp(const QUAT &a, const QUAT &b, float t);

}
