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

#include <gloop/vector.hpp>
#include <gloop/quaternion.hpp>

namespace gloop {

  // matrices are column major, as per OpenGL. accessed as .m[col][row], or ._cr
  struct M3 {
    union {
      float v[9];
      float m[3][3];
      struct {
        float _11, _12, _13, // n.b. these are transposed
              _21, _22, _23,
              _31, _32, _33;
      };
    };

    M3(float m11, float m21, float m31,
       float m12, float m22, float m32,
       float m13, float m23, float m33) {
      // this initializer gives data in row-major form, hence the swizzle.
      _11 = m11; _12 = m12; _13 = m13;
      _21 = m21; _22 = m22; _23 = m23;
      _31 = m31; _32 = m32; _33 = m33;
    }
    M3(const float *_v) {
      memcpy(v, _v, sizeof(v));
    }
    M3(const float _m[3][3]) {
      memcpy(m, _m, sizeof(m));
    }
    M3() { }
    
    static M3 IDENTITY() {
      M3 r;
      r._11 = 1.0f; r._21 = 0.0f; r._31 = 0.0f;
      r._12 = 0.0f; r._22 = 1.0f; r._32 = 0.0f;
      r._13 = 0.0f; r._23 = 0.0f; r._33 = 1.0f;
      return r;
    }
  };



  static inline M3 &operator+=(M3 &a, const M3 &b) { for (unsigned i = 0; i < 9; ++i) a.v[i] += b.v[i]; return a; }
  static inline M3 operator+(const M3 &a, const M3 &b) {  M3 r(a); return r += b; }

  static inline M3 &operator-=(M3 &a, const M3 &b) { for (unsigned i = 0; i < 9; ++i) a.v[i] -= b.v[i]; return a; }
  static inline M3 operator-(const M3 &a, const M3 &b) {  M3 r(a); return r -= b; }

  static inline M3 &operator*=(M3 &a, float b) { for (unsigned i = 0; i < 9; ++i) a.v[i] *= b; return a; }
  static inline M3 operator*(const M3 &a, float b) {  M3 r(a); return r *= b; }

  static inline V3 operator*(const M3 &a, const V3 &b) {
    return V3(a._11 * b.x + a._21 * b.y + a._31 * b.z,
              a._12 * b.x + a._22 * b.y + a._32 * b.z,
              a._13 * b.x + a._23 * b.y + a._33 * b.z);
  }



  struct M4 {
    union {
      float v[16];
      float m[4][4];
      struct {
        float _11, _12, _13, _14, // n.b. these are transposed.
              _21, _22, _23, _24,
              _31, _32, _33, _34,
              _41, _42, _43, _44;
      };
    };

    M4(float m11, float m21, float m31, float m41,
      float m12, float m22, float m32, float m42,
      float m13, float m23, float m33, float m43,
      float m14, float m24, float m34, float m44) {
      // this initializer gives data in row-major form, hence the swizzle.
      _11 = m11; _12 = m12; _13 = m13; _14 = m14;
      _21 = m21; _22 = m22; _23 = m23; _24 = m24;
      _31 = m31; _32 = m32; _33 = m33; _34 = m34;
      _41 = m41; _42 = m42; _43 = m43; _44 = m44;
    }
    M4(const float *_v) {
      memcpy(v, _v, sizeof(v));
    }
    M4(const float _m[4][4]) {
      memcpy(m, _m, sizeof(m));
    }
    M4() { }
    
    static M4 IDENTITY() {
      M4 r;
      r._11 = 1.0f; r._21 = 0.0f; r._31 = 0.0f; r._41 = 0.0f;
      r._12 = 0.0f; r._22 = 1.0f; r._32 = 0.0f; r._42 = 0.0f;
      r._13 = 0.0f; r._23 = 0.0f; r._33 = 1.0f; r._43 = 0.0f;
      r._14 = 0.0f; r._24 = 0.0f; r._34 = 0.0f; r._44 = 1.0f;
      return r;
    }

    static M4 ROT(float a, float x, float y, float z) {
      return (M4)QUAT(V3(x, y, z), a);
    }
    
    static M4 TRANS(float x, float y, float z) {
      return M4(1.0f, 0.0f, 0.0f,    x,
                0.0f, 1.0f, 0.0f,    y,
                0.0f, 0.0f, 1.0f,    z,
                0.0f, 0.0f, 0.0f, 1.0f);
    }
    
    static M4 SCALE(float x, float y, float z) {
      return M4(  x,  0.0f, 0.0f, 0.0f,
                0.0f,    y, 0.0f, 0.0f,
                0.0f, 0.0f,    z, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f);
    }
  };



  static inline M4 &operator+=(M4 &a, const M4 &b) { for (unsigned i = 0; i < 16; ++i) a.v[i] += b.v[i]; return a; }
  static inline M4 operator+(const M4 &a, const M4 &b) {  M4 r(a); return r += b; }

  static inline M4 &operator-=(M4 &a, const M4 &b) { for (unsigned i = 0; i < 16; ++i) a.v[i] -= b.v[i]; return a; }
  static inline M4 operator-(const M4 &a, const M4 &b) {  M4 r(a); return r -= b; }

  static inline M4 &operator*=(M4 &a, float b) { for (unsigned i = 0; i < 16; ++i) a.v[i] *= b; return a; }
  static inline M4 operator*(const M4 &a, float b) {  M4 r(a); return r *= b; }

  static inline V3 operator*(const M4 &a, const V3 &b) {
    return V3(a._11 * b.x + a._21 * b.y + a._31 * b.z + a._41,
              a._12 * b.x + a._22 * b.y + a._32 * b.z + a._42,
              a._13 * b.x + a._23 * b.y + a._33 * b.z + a._43);
  }

  static inline V4 operator*(const M4 &a, const V4 &b) {
    return V4(a._11 * b.x + a._21 * b.y + a._31 * b.z + a._41 * b.w,
              a._12 * b.x + a._22 * b.y + a._32 * b.z + a._42 * b.w,
              a._13 * b.x + a._23 * b.y + a._33 * b.z + a._43 * b.w,
              a._14 * b.x + a._24 * b.y + a._34 * b.z + a._44 * b.w);
  }

}
