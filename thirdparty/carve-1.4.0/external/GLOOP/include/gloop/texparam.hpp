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

#include <gloop/gloopgl.hpp>

#include "exceptions.hpp"

namespace gloop {

  struct TextureParam {
    static GLfloat s_max_anisotropy;
    
    enum {
      FILTER   = 1,
      LOD      = 2,
      MIPMAP   = 4,
      WRAP     = 8,
      BORDER   = 16,
      PRIORITY = 32,
      ALL      = 63
    };
    
    GLenum min_filter;
    GLenum mag_filter;
    GLfloat min_lod;
    GLfloat max_lod;
    GLint min_mipmap;
    GLint max_mipmap;
    GLenum wrap_s;
    GLenum wrap_t;
    GLenum wrap_r;
    GLfloat priority;
    GLfloat anisotropy;
    GLfloat border_colour[4];
    
    TextureParam(GLenum _wrap = GL_REPEAT, GLenum _filter = GL_LINEAR, GLenum _min_filter = 0) {
      if (s_max_anisotropy == 0.0f) {
        if (GLEW_EXT_texture_filter_anisotropic) {
          glGetFloatv(GL_TEXTURE_MAX_ANISOTROPY_EXT, &s_max_anisotropy);
        } else {
          s_max_anisotropy = 1.0f;
        }
      }
      if (_min_filter == 0) _min_filter = _filter;
      min_filter = _min_filter;
      mag_filter = _filter;
      min_lod = -1000;
      max_lod = 1000;
      min_mipmap = 0;
      max_mipmap = 1000;
      wrap_s = wrap_t = wrap_r = _wrap;
      priority = 0.0f;
      anisotropy = 1.0f;
      border_colour[0] = border_colour[1] = border_colour[2] = border_colour[3] = 0.0f;
    }

    ~TextureParam() {
    }
    
    void apply(GLenum target, int flags = ALL) {
      if (flags & FILTER) {
        glTexParameteri(target, GL_TEXTURE_MIN_FILTER, min_filter);
        glTexParameteri(target, GL_TEXTURE_MAG_FILTER, mag_filter);
        if (s_max_anisotropy > 1.0f) {
          glTexParameterf(target, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisotropy);
        }
      }
      if (flags & LOD) {
        glTexParameterf(target, GL_TEXTURE_MIN_LOD, min_lod);
        glTexParameterf(target, GL_TEXTURE_MAX_LOD, max_lod);
      }
      if (flags & MIPMAP) {
        glTexParameteri(target, GL_TEXTURE_BASE_LEVEL, min_mipmap);
        glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, max_mipmap);
      }
      if (flags & WRAP) {
        glTexParameteri(target, GL_TEXTURE_WRAP_S, wrap_s);
        glTexParameteri(target, GL_TEXTURE_WRAP_T, wrap_t);
        glTexParameteri(target, GL_TEXTURE_WRAP_R, wrap_r);
      }
      if (flags & BORDER) {
        glTexParameterfv(target, GL_TEXTURE_BORDER_COLOR, border_colour);
      }
      if (flags & PRIORITY) {
        glTexParameterf(target, GL_TEXTURE_PRIORITY, priority);
      }
    }
  };

}
