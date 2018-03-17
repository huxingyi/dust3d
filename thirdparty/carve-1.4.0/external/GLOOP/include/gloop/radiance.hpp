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

#if (defined WIN32) || (defined _WIN32)
  typedef char int8_t;
  typedef short int16_t;
  typedef long int32_t;

  typedef unsigned char uint8_t;
  typedef unsigned short uint16_t;
  typedef unsigned long uint32_t;
#else
#include <stdint.h>
#endif


#include <algorithm>
#include <istream>
#include <vector>
#include <string>

#include "vector.hpp"
#include "matrix.hpp"

#ifdef  NTSC
#  define  CIE_x_r                0.670 // standard NTSC primaries
#  define  CIE_y_r                0.330
#  define  CIE_x_g                0.210
#  define  CIE_y_g                0.710
#  define  CIE_x_b                0.140
#  define  CIE_y_b                0.080
#  define  CIE_x_w                0.3333 // use true white
#  define  CIE_y_w                0.3333
#else
#  define  CIE_x_r                0.640 // nominal CRT primaries
#  define  CIE_y_r                0.330
#  define  CIE_x_g                0.290
#  define  CIE_y_g                0.600
#  define  CIE_x_b                0.150
#  define  CIE_y_b                0.060
#  define  CIE_x_w                0.3333 // use true white
#  define  CIE_y_w                0.3333
#endif


#define CIE_D    (CIE_x_r * (CIE_y_g - CIE_y_b) + CIE_x_g * (CIE_y_b - CIE_y_r) + CIE_x_b*(CIE_y_r - CIE_y_g))
#define CIE_C_rD ((1.0f / CIE_y_w) * (CIE_x_w * (CIE_y_g - CIE_y_b) - CIE_y_w * (CIE_x_g - CIE_x_b) + CIE_x_g * CIE_y_b - CIE_x_b * CIE_y_g))
#define CIE_C_gD ((1.0f / CIE_y_w) * (CIE_x_w * (CIE_y_b - CIE_y_r) - CIE_y_w * (CIE_x_b - CIE_x_r) - CIE_x_r * CIE_y_b + CIE_x_b * CIE_y_r))
#define CIE_C_bD ((1.0f / CIE_y_w) * (CIE_x_w * (CIE_y_r - CIE_y_g) - CIE_y_w * (CIE_x_r - CIE_x_g) + CIE_x_r * CIE_y_g - CIE_x_g * CIE_y_r))

#define CIE_rf   (CIE_y_r * CIE_C_rD / CIE_D)
#define CIE_gf   (CIE_y_g * CIE_C_gD / CIE_D)
#define CIE_bf   (CIE_y_b * CIE_C_bD / CIE_D)


// luminous efficacies over visible spectrum
#define  MAXEFFICACY            683.0f          // defined maximum at 550 nm
#define  WHTEFFICACY            179.0f          // uniform white light
#define  D65EFFICACY            203.0f          // standard illuminant D65
#define  INCEFFICACY            160.0f          // illuminant A (incand.)
#define  SUNEFFICACY            208.0f          // illuminant B (solar dir.)
#define  SKYEFFICACY            D65EFFICACY     // skylight (should be 110)
#define  DAYEFFICACY            D65EFFICACY     // combined sky and solar


namespace gloop {

  struct chromacity {
    static const chromacity stdprims;
    V2 r, g, b, w;
    
    M3 xyz_to_rgb() const;
    M3 rgb_to_xyz() const;

  };



  static inline double bright(const V3 &col) {
    return CIE_rf * col.r + CIE_gf * col.g + CIE_bf * col.b;
  }

  static inline double luminance(const V3 &col) {
    return WHTEFFICACY * bright(col);
  }

  static inline float intens(const V3 &col) {
    return std::max(std::max(col.r, col.g), col.b);
  }



  extern const V3 WHITE;
  extern const V3 BLACK;

  extern const M3 RGB_to_XYZ;
  extern const M3 XYZ_to_RGB;



  struct packed_colour {
    static const int excess = 128;
    union {
      struct {
        union {
          struct { uint8_t r,g,b; };
          struct { uint8_t x,y,z; };
        };
        uint8_t e;
      };
      uint8_t v[4];
      uint32_t val;
    };

    packed_colour() { }
    packed_colour(uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _e) { r = _r; g = _g; b = _b; e = _e; }
    packed_colour(uint32_t _val) { val = _val; }
    packed_colour(const void *mem) { memcpy(&v, mem, 4); }
    packed_colour(const V3 &col);
    
    const static packed_colour white;
    const static packed_colour black;
    
    V3 unpack() const {
      if (e) {
        return V3(ldexp(r + 0.5f, (int)e - (excess + 8)),
                  ldexp(g + 0.5f, (int)e - (excess + 8)),
                  ldexp(b + 0.5f, (int)e - (excess + 8)));
      } else {
        return V3(0.0f, 0.0f, 0.0f);
      }
    }

    long normbright() {
      return ((long)(CIE_rf * 256.0f + 0.5f) * r + (long)(CIE_gf * 256.0f + 0.5f) * g + (long)(CIE_bf * 256.0f + 0.5f) * b) >> 8;
    }

    const static packed_colour BLACK;
    const static packed_colour WHITE;
  };

  template<typename T, typename U>
  static inline void unpack_scanline(T begin, T end, U out) {
    while (begin != end) {
      *out = (*begin).unpack();
      ++out; ++begin;
    }
  }



  enum radiance_colour_format {
    RADIANCE_FMT_UNKNOWN = -1,
    RADIANCE_FMT_RGB = 0,
    RADIANCE_FMT_CIE = 1
  };



  struct radiance_reader {
    virtual void header(radiance_colour_format fmt,
                        float exposure,
                        float pixaspect,
                        chromacity prim,
                        V3 colour_correction) = 0;
    virtual void image_size(int width, int height, int depth) = 0;
    virtual void scanline(int scan_axis,
                          int X, int Y, int Z,
                          std::vector<packed_colour> &scanline) = 0;
    virtual ~radiance_reader() {
    }
  };

  struct floatbuf_radiance_reader : public radiance_reader {
    float *target;
    int width, height, depth;
    M3 cvt;
    bool cvt_set;

    int stride(int axis) {
      int stride = 3;
      if (axis > 0) stride *= width;
      if (axis > 1) stride *= height;
      return stride;
    }
    float *pixel(int X = 0, int Y = 0, int Z = 0) {
      return target + 3 * (X + width * (Y + height * Z));
    }
    virtual void header(radiance_colour_format fmt,
                        float exposure,
                        float pixaspect,
                        chromacity prim,
                        V3 colour_correction);
    virtual void image_size(int _width, int _height, int _depth);
    virtual void scanline(int scan_axis,
                          int X, int Y, int Z,
                          std::vector<packed_colour> &scanline);
    float *take_buffer();
    floatbuf_radiance_reader();
    virtual ~floatbuf_radiance_reader();
  };



  void read_radiance(std::istream &in, radiance_reader &reader, bool invert_X = false, bool invert_Y = true, bool invert_Z = false);

}
