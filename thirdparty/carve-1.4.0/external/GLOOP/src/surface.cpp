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

#include <gloop/surface.hpp>

namespace gloop {

  std::map<std::pair<GLenum, int>, std::vector<GLuint> > Surface::s_bind;

  const Surface::Desc Surface::s_desc[5] = {
    Surface::Desc(SURF_NONE,          0,                       0,                       false, false, TextureParam()),
    Surface::Desc(SURF_COLOUR,        GL_TEXTURE_2D,           GL_RGBA,                 true,  false, TextureParam()),
    Surface::Desc(SURF_DEPTH,         GL_RENDERBUFFER_EXT,     GL_DEPTH_COMPONENT24,    false, false, TextureParam()),
    Surface::Desc(SURF_STENCIL,       GL_RENDERBUFFER_EXT,     GL_STENCIL_INDEX8_EXT,   false, false, TextureParam()),
    Surface::Desc(SURF_DEPTH_STENCIL, GL_RENDERBUFFER_EXT,     GL_DEPTH24_STENCIL8_EXT, false, false, TextureParam())
  };

  static inline GLenum externalFormatForInternal(GLenum internal) {
    switch (internal) {
      case GL_ALPHA:
      case GL_ALPHA4:
      case GL_ALPHA8:
      case GL_ALPHA12:
      case GL_ALPHA16:
        return GL_ALPHA;
        
      case GL_LUMINANCE:
      case GL_LUMINANCE4:
      case GL_LUMINANCE8:
      case GL_LUMINANCE12:
      case GL_LUMINANCE16:
        return GL_LUMINANCE;
        
      case GL_LUMINANCE_ALPHA:
      case GL_LUMINANCE4_ALPHA4:
      case GL_LUMINANCE8_ALPHA8:
      case GL_LUMINANCE12_ALPHA12:
      case GL_LUMINANCE16_ALPHA16:
        return GL_LUMINANCE_ALPHA;
        
      case GL_INTENSITY:
      case GL_INTENSITY4:
      case GL_INTENSITY8:
      case GL_INTENSITY12:
      case GL_INTENSITY16:
        return GL_INTENSITY;
        
      case GL_R3_G3_B2:
      case GL_RGB:
      case GL_RGB4:
      case GL_RGB5:
      case GL_RGB8:
      case GL_RGB10:
      case GL_RGB12:
      case GL_RGB16:
        return GL_RGB;
        
      case GL_RGBA:
      case GL_RGBA2:
      case GL_RGBA4:
      case GL_RGB5_A1:
      case GL_RGBA8:
      case GL_RGB10_A2:
      case GL_RGBA12:
      case GL_RGBA16:
        return GL_RGBA;
        
      case GL_DEPTH_COMPONENT16_ARB:
      case GL_DEPTH_COMPONENT24_ARB:
      case GL_DEPTH_COMPONENT32_ARB:
      case GL_TEXTURE_DEPTH_SIZE_ARB:
      case GL_DEPTH_TEXTURE_MODE_ARB:
        return GL_DEPTH_COMPONENT;
        
      default:
        return GL_RGBA;
    }
  }

  Surface::Surface(const Desc &d) : desc(d), gl_id(0), width(0), height(0), depth(0) {
  }

  Surface::~Surface() {
    if (gl_id) {
      if (desc.is_texture) {
        glDeleteTextures(1, &gl_id);
      } else {
        glDeleteRenderbuffersEXT(1, &gl_id);
      }
    }
  }
    
  void Surface::pushBind(int texunit) {
    std::vector<GLuint> &b(s_bind[std::make_pair(desc.gl_tgt, texunit)]);
    if (!b.size()) b.push_back(0);
    
    if (gl_id != b.back()) {
      glActiveTexture(GL_TEXTURE0_ARB + texunit);
      glBindTexture(desc.gl_tgt, gl_id);
    }
    b.push_back(gl_id);
  }
    
  void Surface::bind(int texunit) {
    std::vector<GLuint> &b(s_bind[std::make_pair(desc.gl_tgt, texunit)]);
    if (!b.size()) b.push_back(0);
      
    if (gl_id != b.back()) {
      glActiveTexture(GL_TEXTURE0_ARB + texunit);
      glBindTexture(desc.gl_tgt, gl_id);
      if (b.size() == 1) {
        b.push_back(gl_id);
      } else {
        b.back() = gl_id;
      }
    }
  }
    
  void Surface::unbind(int texunit) {
    std::vector<GLuint> &b(s_bind[std::make_pair(desc.gl_tgt, texunit)]);
    if (b.size() > 1) {
      b.pop_back();
      if (b.back() != gl_id) {
        glActiveTexture(GL_TEXTURE0_ARB + texunit);
        glBindTexture(desc.gl_tgt, b.back());
      }
    }
  }
    
  void Surface::init(unsigned w, unsigned h, unsigned d, GLenum ext_fmt, GLenum ext_type, GLvoid **ext_data) {
    if (gl_id > 0) {
      throw std::runtime_error("Already initialized");
    }
    
    switch (desc.surface_type) {
      case SURF_NONE:
        throw std::invalid_argument("SURF_NONE");
      case SURF_COLOUR:
        if (!desc.is_texture) return throw std::invalid_argument("Colour buffers must be textures");
        break;
      case SURF_STENCIL:
        if (desc.is_texture) return throw std::invalid_argument("Stencil buffers must not be textures");
        break;
    }
    
    if (desc.is_texture) {
      if (desc.gl_tgt != GL_TEXTURE_RECTANGLE_ARB) {
        P2(w); P2(h); P2(d);
      }
      glGenTextures(1, &gl_id);
      glBindTexture(desc.gl_tgt, gl_id);
      glGetError();
      if (ext_fmt == 0) ext_fmt = externalFormatForInternal(desc.gl_fmt);
      switch (desc.gl_tgt) {
        case GL_TEXTURE_1D:
          glTexImage1D(desc.gl_tgt, 0, desc.gl_fmt, w, 0, ext_fmt, ext_type, ext_data ? ext_data[0] : NULL);
          break;
        case GL_TEXTURE_2D:
        case GL_TEXTURE_RECTANGLE_ARB:
          glTexImage2D(desc.gl_tgt, 0, desc.gl_fmt, w, h, 0, ext_fmt, ext_type, ext_data ? ext_data[0] : NULL);
          break;
        case GL_TEXTURE_CUBE_MAP:
          for (int i = 0; i < 6; ++i) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, desc.gl_fmt, w, h, 0, ext_fmt, ext_type, ext_data ? ext_data[i] : NULL);
          }
          break;
        case GL_TEXTURE_3D:
          glTexImage3D(desc.gl_tgt, 0, desc.gl_fmt, w, h, d, 0, ext_fmt, ext_type, ext_data ? ext_data[0] : NULL);
          break;
      }
      GLuint err = glGetError();
      if (err) {
        glDeleteTextures(1, &gl_id);
        gl_id = 0;
        throw std::runtime_error("failed to initialize texture");
      }
      if (desc.is_mipmapped)
        glGenerateMipmapEXT(desc.gl_tgt);
      desc.param.apply(desc.gl_tgt);
      glBindTexture(desc.gl_tgt, 0);
    } else {
      glGenRenderbuffersEXT(1, &gl_id);
      glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, gl_id);
      glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, desc.gl_fmt, w, h);
      glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);
    }
    width = w;
    height = h;
    depth = d;
  }

}
