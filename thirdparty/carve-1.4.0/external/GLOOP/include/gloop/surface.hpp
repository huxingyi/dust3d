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

#include <vector>
#include <map>

#include <gloop/gloopgl.hpp>

#include <gloop/exceptions.hpp>
#include <gloop/texparam.hpp>
#include <gloop/ref.hpp>

namespace gloop {

  static inline void P2(unsigned &x) {
    x--;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x++;
  }


  enum SurfaceType {
    SURF_NONE          = 0,
    SURF_COLOUR        = 1,
    SURF_DEPTH         = 2,
    SURF_STENCIL       = 3,
    SURF_DEPTH_STENCIL = 4
  };

  class SurfP;

  class Surface : public virtual RefObj {
  public:
    typedef Ref<Surface> Ptr;
    
    struct Desc {
      SurfaceType surface_type;
      GLenum gl_tgt;
      GLenum gl_fmt;
      bool is_texture;
      bool is_mipmapped;
      TextureParam param;

      Desc(SurfaceType _surface_type, GLenum _gl_tgt, GLenum _gl_fmt, bool _is_texture, bool _is_mipmapped, const TextureParam &_param) :
        surface_type(_surface_type), gl_tgt(_gl_tgt), gl_fmt(_gl_fmt), is_texture(_is_texture), is_mipmapped(_is_mipmapped), param(_param) {
      }
    } desc;
    GLuint gl_id;
    unsigned width, height, depth;
    
    static std::map<std::pair<GLenum, int>, std::vector<GLuint> > s_bind;
    const static Desc s_desc[5];

    Surface(const Desc &d);
    ~Surface();
    
    void pushBind(int texunit = 0);
    void bind(int texunit = 0);
    void unbind(int texunit = 0);
    void init(unsigned w = 0, unsigned h = 0, unsigned d = 0, GLenum ext_fmt = 0, GLenum ext_type = GL_BYTE, GLvoid **ext_data = NULL);
  };

  class SurfP {
    SurfP();
    SurfP(const SurfP &);
    SurfP &operator=(const SurfP &);
    
  public:
    Surface::Desc d;

    SurfP(int base) : d(Surface::s_desc[base]) {}
    SurfP &tgt(GLenum gl_tgt) { d.gl_tgt = gl_tgt; return *this; }
    SurfP &fmt(GLenum gl_fmt) { d.gl_fmt = gl_fmt; return *this; }
    SurfP &tex(bool tex) { d.is_texture = tex; return *this; }
    SurfP &mipmap(bool mipmap) { d.is_mipmapped = mipmap; return *this; }
    SurfP &param(const TextureParam &param) { d.param = param; return *this; }
    operator Surface::Desc &() { return d; }
  };

}
