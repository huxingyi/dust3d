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

#include <gloop/radiance.hpp>
#include <gloop/exceptions.hpp>
#include <iostream>
#include <cstring>
#include <stdio.h>

namespace gloop {

  const chromacity chromacity::stdprims = {
    V2(CIE_x_r,CIE_y_r),
    V2(CIE_x_g,CIE_y_g),
    V2(CIE_x_b,CIE_y_b),
    V2(CIE_x_w,CIE_y_w)
  };

  static inline bool eq(float a, float b, float epsilon) { return fabs(a - b) < epsilon; }
  static inline bool zero(float a, float epsilon) { return fabs(a) < epsilon; }

  M3 chromacity::xyz_to_rgb() const {
    double  C_rD, C_gD, C_bD;

    if (zero(w.x, 1e-4) | zero(w.y, 1e-4))
      throw std::runtime_error("bad chromacity primaries");

    C_rD = (1.0f / w.y) * (w.x * (g.y - b.y) - w.y * (g.x - b.x) + g.x * b.y - b.x * g.y);
    C_gD = (1.0f / w.y) * (w.x * (b.y - r.y) - w.y * (b.x - r.x) - r.x * b.y + b.x * r.y);
    C_bD = (1.0f / w.y) * (w.x * (r.y - g.y) - w.y *(r.x - g.x) + r.x * g.y - g.x * r.y);

    if (zero(C_rD, 1e-4) || zero(C_gD, 1e-4) || zero(C_bD, 1e-4))
      throw std::runtime_error("bad chromacity primaries");

    return M3((g.y - b.y - b.x * g.y + b.y * g.x) / C_rD, (b.x - g.x - b.x * g.y + g.x * b.y) / C_rD, (g.x * b.y - b.x * g.y) / C_rD,
              (b.y - r.y - b.y * r.x + r.y * b.x) / C_gD, (r.x - b.x - r.x * b.y + b.x * r.y) / C_gD, (b.x * r.y - r.x * b.y) / C_gD,
              (r.y - g.y - r.y * g.x + g.y * r.x) / C_bD, (g.x - r.x - g.x * r.y + r.x * g.y) / C_bD, (r.x * g.y - g.x * r.y) / C_bD);
  }



  M3 chromacity::rgb_to_xyz() const {
    double  C_rD, C_gD, C_bD, D;

    if (zero(w.x, 1e-4) | zero(w.y, 1e-4))
      throw std::runtime_error("bad chromacity primaries");

    C_rD = (1.0f / w.y) * (w.x * (g.y - b.y) - w.y * (g.x - b.x) + g.x * b.y - b.x * g.y);
    C_gD = (1.0f / w.y) * (w.x * (b.y - r.y) - w.y * (b.x - r.x) - r.x * b.y + b.x * r.y);
    C_bD = (1.0f / w.y) * (w.x * (r.y - g.y) - w.y * (r.x - g.x) + r.x * g.y - g.x * r.y);
    D = r.x * (g.y - b.y) + g.x * (b.y - r.y) + b.x * (r.y - g.y);

    if (zero(D, 1e-4))
      throw std::runtime_error("bad chromacity primaries");
    
    return M3(               r.x * C_rD / D,                g.x * C_gD / D,                b.x * C_bD / D,
                            r.y * C_rD / D,                g.y * C_gD / D,                b.y * C_bD / D,
              (1.0f - r.x - r.y) * C_rD / D, (1.0f - g.x - g.y) * C_gD / D, (1.0f - b.x - b.y) * C_bD / D);
  }



  const V3 WHITE(1.0, 1.0, 1.0);
  const V3 BLACK(0.0, 0.0, 0.0);

  const M3 RGB_to_XYZ(chromacity::stdprims.rgb_to_xyz());
  const M3 XYZ_to_RGB(chromacity::stdprims.xyz_to_rgb());



  packed_colour::packed_colour(const V3 &col) {
    float d;
    
    d = std::max(std::max(col.r, col.g), col.b);
    
    r = g = b = e = 0;
    if (d > 1e-30f) {
      int exp;
      d = frexpf(d, &exp) * 255.0f / d;
      
      if (col.r > 0.0) r = (uint8_t)(col.r * d + 0.5f);
      if (col.g > 0.0) g = (uint8_t)(col.g * d + 0.5f);
      if (col.b > 0.0) b = (uint8_t)(col.b * d + 0.5f);
      
      e = exp + excess;
    }
  }

  const packed_colour packed_colour::BLACK(0, 0, 0, 0);
  const packed_colour packed_colour::WHITE(128, 128, 128, packed_colour::excess + 1);



  #define MINELEN 8
  #define MAXELEN 0x7fff
  #define MINRUN 4



  static void read_scanline_old(std::istream &in, std::vector<packed_colour> &scanline, size_t i = 0) {
    int rshift = 0;
    while (i < scanline.size()) {
      in.read((char *)&scanline[i].val, 4);
      if (in.gcount() != 4)
        throw std::runtime_error("premature EOF");
      if (scanline[i].r == 1 && scanline[i].g == 1 && scanline[i].b == 1) {
        size_t j = i + (scanline[i].e << rshift);
        if (j > scanline.size())
          throw std::runtime_error("RLE overflow");
        uint32_t copy = scanline[i - 1].val;
        for (; i < j; i++) { scanline[i].val = copy; }
        rshift += 8;
      } else {
        i++;
        rshift = 0;
      }
    }
  }

  static void read_scanline(std::istream &in, std::vector<packed_colour> &scanline) {
    uint8_t buf[0x80];

    if (scanline.size() < MINELEN || scanline.size() > MAXELEN) {
      return read_scanline_old(in, scanline);
    }

    in.read((char *)&scanline[0].val, 4);
    if (in.gcount() != 4)
      throw std::runtime_error("premature EOF");

    if (scanline[0].r != 2 || scanline[0].g != 2 || (scanline[0].b & 0x80))
      return read_scanline_old(in, scanline, 1);

    if ((scanline[0].b << 8 | scanline[0].e) != scanline.size())
      throw std::runtime_error("length mismatch");

    for (size_t component = 0; component < 4; component++) {
      size_t i, j;
      for (i = 0; i < scanline.size(); ) {
        int code, val;
        code = in.get();
        if (code == std::istream::traits_type::eof())
          throw std::runtime_error("premature EOF");
        bool run = code > 0x80;
        if (run) code &= 0x7f;
        j = i + code;
        if (j > scanline.size())
          throw std::runtime_error("RLE overflow");
        if (run) {
          val = in.get();
          if (val == std::istream::traits_type::eof())
            throw std::runtime_error("premature EOF");
          while (i < j) scanline[i++].v[component] = (uint8_t)val;
        } else {
          in.read((char *)buf, code);
          if (in.gcount() != code)
            throw std::runtime_error("premature EOF");
          size_t k = 0;
          while (i < j) scanline[i++].v[component] = buf[k++];
        }
      }
    }
  }

  void read_radiance(std::istream &in, radiance_reader &reader, bool invert_X, bool invert_Y, bool invert_Z) {
    std::string str;
    const char *c;
    
    std::getline(in, str);
    if (str != "#?RADIANCE") throw std::runtime_error("not radiance format");
    
    float exposure = 1.0;
    float pixaspect = 1.0;
    chromacity prim = chromacity::stdprims;
    V3 colour_correction(1.0, 1.0, 1.0);
    radiance_colour_format fmt = RADIANCE_FMT_UNKNOWN;
    
    while (in.good()) {
      std::getline(in, str);
      if (str == "") break;
      if (str[0] == '#') continue;

      if (sscanf(str.c_str(), "EXPOSURE= %e",
                &exposure) == 1) continue;

      if (sscanf(str.c_str(), "PIXASPECT= %f",
                &pixaspect) == 1) continue;

      if (sscanf(str.c_str(), "PRIMARIES= %f %f %f %f %f %f %f %f",
                &prim.r.x, &prim.r.y,
                &prim.g.x, &prim.g.y,
                &prim.b.x, &prim.b.y,
                &prim.w.x, &prim.w.y) == 8) continue;

      if (sscanf(str.c_str(), "COLORCORR= %f %f %f",
                &colour_correction.r,
                &colour_correction.g,
                &colour_correction.b) == 3) continue;

      if (!str.compare(0, 7, "FORMAT=", 7)) {
        c = str.c_str() + 7;
        while (*c && isspace(*c)) c++;
        if (!std::strcmp(c, "32-bit_rle_rgbe")) {
          fmt = RADIANCE_FMT_RGB;
        } else if (!std::strcmp(c, "32-bit_rle_xyze")) {
          fmt = RADIANCE_FMT_CIE;
        } else {
          throw std::runtime_error("bad image format: " + std::string(c));
        }
        continue;
      }

      throw std::runtime_error("strange header line: " + str);
    }
    reader.header(fmt,
                  exposure,
                  pixaspect,
                  prim,
                  colour_correction);

    int dim = 0;
    char dir[3];
    char axis[3];
    int size[3], curr[3];
    
    std::getline(in, str);
    c = str.c_str();
    for (dim = 0; *c && dim < 3; dim++) {
      switch (c[0]) {
        case '-': dir[dim] = -1; break;
        case '+': dir[dim] = +1; break;
        default: throw std::runtime_error("strange image dimensions: " + str);
      }
      switch (c[1]) {
        case 'X':axis[dim] = 0; if (invert_X) dir[dim] = -dir[dim]; break;
        case 'Y':axis[dim] = 1; if (invert_Y) dir[dim] = -dir[dim]; break;
        case 'Z':axis[dim] = 2; if (invert_Z) dir[dim] = -dir[dim]; break;
        default: throw std::runtime_error("strange image dimensions: " + str);
      }

      c += 2;
      while (*c && isspace(*c)) c++;
    
      if (sscanf(c, "%u", &size[dim]) != 1)
        throw std::runtime_error("strange image dimensions: " + str);
    
      while (*c && !isspace(*c)) c++;
      while (*c &&  isspace(*c)) c++;
    }

    int s[3];
    s[0] = s[1] = s[2] = 1;
    for (int i = 0; i < dim; i++)
      s[axis[i]] = size[i];

    reader.image_size(s[0], s[1], s[2]);

    std::vector<packed_colour> scanline;
    scanline.resize(size[dim - 1]);

    curr[0] = curr[1] = curr[2] = 0;
    for (int i = 0; i < dim - 1; i++)
      curr[axis[i]] = dir[i] == +1 ? 0 : size[i] - 1;
    curr[axis[dim - 1]] = 0;

  next:
    read_scanline(in, scanline);
    reader.scanline(axis[dim - 1], curr[0], curr[1], curr[2], scanline);

    for (int d = dim - 2; d >= 0; d--) {
      if (dir[d] == -1) {
        if (curr[axis[d]]-- > 0) goto next;
        curr[axis[d]] = size[d] - 1;
      } else {
        if (++curr[axis[d]] < size[d]) goto next;
        curr[axis[d]] = 0;
      }
    }
  }



  struct floatbuf_iter {
    float *p;
    unsigned s;
    
    floatbuf_iter(int scan_axis, int X, int Y, int Z, floatbuf_radiance_reader *tgt) {
      p = tgt->pixel(X, Y, Z);
      s = tgt->stride(scan_axis);
    }
    V3 &operator*() {
      return *reinterpret_cast<V3 *>(p);
    }
    floatbuf_iter &operator++() { p += s; return *this; }
  };



  struct floatbuf_iter_cvt {
    float *p;
    unsigned s;
    M3 &cvt;
    
    floatbuf_iter_cvt(int scan_axis, int X, int Y, int Z, floatbuf_radiance_reader *tgt) : cvt(tgt->cvt) {
      p = tgt->pixel(X, Y, Z);
      s = tgt->stride(scan_axis);
    }
    V3 operator*() {
      return cvt * *reinterpret_cast<V3 *>(p);
    }
    floatbuf_iter_cvt &operator++() { p += s; return *this; }
  };



  floatbuf_radiance_reader::floatbuf_radiance_reader() :
  target(NULL), width(0), height(0), depth(0), cvt(M3::IDENTITY()), cvt_set(false) {
  }

  void floatbuf_radiance_reader::header(radiance_colour_format fmt,
                                        float exposure,
                                        float pixaspect,
                                        chromacity prim,
                                        V3 colour_correction) {
    if (fmt == RADIANCE_FMT_CIE) {
      cvt = prim.xyz_to_rgb();
    }
  }

  void floatbuf_radiance_reader::image_size(int _width, int _height, int _depth) {
    width = _width;
    height = _height;
    depth = _depth;
    target = new float[width * height * depth * 3];
  }

  void floatbuf_radiance_reader::scanline(int scan_axis,
                                          int X, int Y, int Z,
                                          std::vector<packed_colour> &scanline) {
    if (!cvt_set) {
      unpack_scanline(scanline.begin(), scanline.end(), floatbuf_iter(scan_axis, X, Y, Z, this));
    } else {
      unpack_scanline(scanline.begin(), scanline.end(), floatbuf_iter_cvt(scan_axis, X, Y, Z, this));
    }
  }

  float *floatbuf_radiance_reader::take_buffer() {
    float *result = target;
    target = NULL;
    return result;
  }

  floatbuf_radiance_reader::~floatbuf_radiance_reader() {
    if (target) delete [] target;
  }

}
