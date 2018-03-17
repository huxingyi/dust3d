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

#include <math.h>
#include <stdlib.h>
#include <memory.h>

namespace gloop {

  struct V2 {
    union {
      struct { float x, y; };
      struct { float r, g; };
      struct { float s, t; };
      float v[2];
    };
    
    V2(float _x, float _y) : x(_x), y(_y) { }
    V2() : x(0.0f), y(0.0f) { }
    V2(const float *_v) { memcpy(v, _v, sizeof(v)); }
  };

  static inline V2 &operator+=(V2 &a, const V2 &b) { a.x += b.x; a.y += b.y; return a; }
  static inline V2 operator+(const V2 &a, const V2 &b) { V2 r(a); return r += b; }

  static inline V2 &operator-=(V2 &a, const V2 &b) { a.x -= b.x; a.y -= b.y; return a; }
  static inline V2 operator-(const V2 &a, const V2 &b) { V2 r(a); return r -= b; }

  static inline V2 &operator*=(V2 &a, float &b) { a.x *= b; a.y *= b; return a; }
  static inline V2 operator*(const V2 &a, float b) { V2 r(a); return r *= b; }

  static inline V2 &operator/=(V2 &a, float &b) { a.x /= b; a.y /= b; return a; }
  static inline V2 operator/(const V2 &a, float b) { V2 r(a); return r /= b; }

  static inline float dot(const V2 &a, const V2 &b) { return a.x * b.x + a.y * b.y; }
  static inline float cross(const V2 &a, const V2 &b) {
    return a.x * b.y - a.y * b.x;
  }



  struct V3 {
    union {
      struct { float x, y, z; };
      struct { float r, g, b; };
      struct { float s, t, p; };
      float v[3];
    };
    
    V3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) { }
    V3() : x(0.0f), y(0.0f), z(0.0f) { }
    V3(const float *_v) { memcpy(v, _v, sizeof(v)); }
  };

  static inline V3 &operator+=(V3 &a, const V3 &b) { a.x += b.x; a.y += b.y; a.z += b.z; return a; }
  static inline V3 operator+(const V3 &a, const V3 &b) { V3 r(a); return r += b; }

  static inline V3 &operator-=(V3 &a, const V3 &b) { a.x -= b.x; a.y -= b.y; a.z -= b.z; return a; }
  static inline V3 operator-(const V3 &a, const V3 &b) { V3 r(a); return r -= b; }

  static inline V3 &operator*=(V3 &a, float &b) { a.x *= b; a.y *= b; a.z *= b; return a; }
  static inline V3 operator*(const V3 &a, float b) { V3 r(a); return r *= b; }

  static inline V3 &operator/=(V3 &a, float &b) { a.x /= b; a.y /= b; a.z /= b; return a; }
  static inline V3 operator/(const V3 &a, float b) { V3 r(a); return r /= b; }

  static inline float dot(const V3 &a, const V3 &b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
  static inline V3 cross(const V3 &a, const V3 &b) {
    return V3(+(a.y * b.z - a.z * b.y), -(a.x * b.z - a.z * b.x), +(a.x * b.y - a.y * b.x));
  }



  struct V4 {
    union {
      struct { float x, y, z, w; };
      struct { float r, g, b, a; };
      struct { float s, t, p, q; };
      float v[4];
    };

    V4(float _x, float _y, float _z, float _w = 1.0f) : x(_x), y(_y), z(_z), w(_w) { }
    V4() : x(0.0f), y(0.0f), z(0.0f), w(0.0f) { }
    V4(const float *_v) { memcpy(v, _v, sizeof(v)); }
  };

  static inline V4 &operator+=(V4 &a, const V4 &b) { a.x += b.x; a.y += b.y; a.z += b.z; a.w += b.w; return a; }
  static inline V4 operator+(const V4 &a, const V4 &b) { V4 r(a); return r += b; }

  static inline V4 &operator-=(V4 &a, const V4 &b) { a.x -= b.x; a.y -= b.y; a.z -= b.z; a.w -= b.w; return a; }
  static inline V4 operator-(const V4 &a, const V4 &b) { V4 r(a); return r -= b; }

  static inline V4 &operator*=(V4 &a, float &b) { a.x *= b; a.y *= b; a.z *= b; a.w *= b; return a; }
  static inline V4 operator*(const V4 &a, float b) { V4 r(a); return r *= b; }

  static inline V4 &operator/=(V4 &a, float &b) { a.x /= b; a.y /= b; a.z /= b; a.w /= b; return a; }
  static inline V4 operator/(const V4 &a, float b) { V4 r(a); return r /= b; }

  static inline float dot(const V4 &a, const V4 &b) { return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w; }

}
