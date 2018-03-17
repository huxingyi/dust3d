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

#include "vertexformat.hpp"

namespace gloop {

  namespace buffer {
    enum XferType {
      STATIC = GL_STATIC_DRAW_ARB,
      DYNAMIC = GL_DYNAMIC_DRAW_ARB,
      STREAM = GL_STREAM_DRAW_ARB
    };
    
    enum AccessType {
      RO = GL_READ_ONLY_ARB,
      RW = GL_READ_WRITE_ARB,
      WO = GL_WRITE_ONLY_ARB
    };
    
    template<typename Transfer, GLuint BUFTYPE = GL_ARRAY_BUFFER_ARB>
    struct VBO {
      typedef Transfer FMT;
        
      size_t vbo_size;
      XferType xfer_type;
      GLuint vbo;
      Transfer *buf;
      Transfer *base;
        
      VBO(size_t buflen, XferType xfer) : vbo_size(buflen), xfer_type(xfer), vbo(0), buf(NULL) {
        glGenBuffersARB(1 , &vbo);
      }

      ~VBO() {
        glDeleteBuffersARB(1, &vbo);
      }
        
      void bind() {
        glBindBufferARB(BUFTYPE, vbo);
      }

      void data(size_t size, void *mem = NULL) {
        glBufferDataARB(BUFTYPE, sizeof(Transfer) * (vbo_size = size), mem, xfer_type);
      }

      void data(void *mem = NULL) {
        glBufferDataARB(BUFTYPE, sizeof(Transfer) * vbo_size, mem, xfer_type);
      }

      void map(AccessType access = WO) {
        buf = base = (Transfer *)glMapBufferARB(BUFTYPE, access);
      }
        
      void push(const Transfer *data) {
        *buf++ = *data;
      }
      void push(const Transfer *data, int count) {
        while (count--) *buf++ = *data;
      }
      void pull(Transfer *data) {
        *data = *buf++;
      }
      void pull(const Transfer *data, int count) {
        while (count--) *data = *buf++;
      }
      
      Transfer &operator[](int i) {
        return base[i];
      }
        
      void unmap() {
        glUnmapBufferARB(BUFTYPE);
        buf = base = NULL;
      }
        
      void unbind() {
        glBindBufferARB(BUFTYPE, 0);
      }
  private:
      VBO();
      VBO(const VBO &);
      VBO &operator=(const VBO &);
    };

    typedef VBO<GLuint, GL_ELEMENT_ARRAY_BUFFER_ARB> IBO32;
    typedef VBO<GLushort, GL_ELEMENT_ARRAY_BUFFER_ARB> IBO16;
  };

}
