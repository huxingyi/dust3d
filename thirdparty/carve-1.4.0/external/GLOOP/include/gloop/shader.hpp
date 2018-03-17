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

#include <string>
#include <list>
#include <sstream>
#include <istream>
#include <iterator>

#include "ref.hpp"
#include "exceptions.hpp"
#include "surface.hpp"

namespace gloop {

  class Shader : public virtual RefObj {
  private:
    Shader();
    Shader(const Shader &);
    Shader &operator=(const Shader &);
    
  protected:
    static int s_tag;
    
  public:
    typedef Ref<Shader> Ptr;

    const std::string name;
    const std::string prog;
    GLuint shader;
    mutable int tag;
    std::list<Ptr> dependencies;

  protected:
    void _source(std::list<std::string> &result) const;
    
    virtual GLenum shaderType() = 0;

    void _compile();
    void _attachTo(GLuint program, bool compile = true);
    
  public:
    Shader(const std::string &_name, const std::string &_prog);
    Shader(const std::string &_name, std::istream &_stream);
    virtual ~Shader();
    virtual void destroy();
    
    bool isCompiled() const {
      return shader != 0;
    }

    void addDependency(const Ptr &dep);
    void compileFlat();
    void compile();
    void attachFlat(GLuint program, bool compile = true);
    void attachTo(GLuint program, bool compile = true);
  };



  class VertexShader : public Shader {
  public:
    typedef Ref<VertexShader> Ptr;
    
  protected:
    virtual GLenum shaderType();
    
  public:
    VertexShader(const std::string &_name, const std::string &_prog);
    VertexShader(const std::string &_name, std::istream &_stream);

    virtual ~VertexShader();
  };



  class FragmentShader : public Shader {
  public:
    typedef Ref<FragmentShader> Ptr;
    
  protected:
    virtual GLenum shaderType();
    
  public:
    FragmentShader(const std::string &_name, const std::string &_prog);
    FragmentShader(const std::string &_name, std::istream &_stream);

    virtual ~FragmentShader();
  };



  class ShaderProgram : public virtual RefObj {
    VertexShader::Ptr vertex_shader;
    FragmentShader::Ptr fragment_shader;
    GLuint program;
    
  public:
    ShaderProgram();
    virtual void destroy();
    virtual ~ShaderProgram();
    
    virtual ShaderProgram &connect(VertexShader *shader);
    virtual ShaderProgram &connect(FragmentShader *shader);
    virtual ShaderProgram &connect(const VertexShader::Ptr &shader);
    virtual ShaderProgram &connect(const FragmentShader::Ptr &shader);
    virtual ShaderProgram &connect(Shader *shader);
    virtual ShaderProgram &connect(Shader::Ptr &shader);

    ShaderProgram &link();
    GLuint prog() { return program; }
    ShaderProgram &install() {
      if (program) glUseProgramObjectARB(program);
      return *this;
    }
    static void uninstall() {
      glUseProgramObjectARB(0);
    }
    int uniform(const char *var) {
      return glGetUniformLocationARB(program, var);
    }

    ShaderProgram &u(const char *var, GLfloat x) { glUniform1fARB(uniform(var), x); return *this; }
    ShaderProgram &u(const char *var, GLfloat x, GLfloat y) { glUniform2fARB(uniform(var), x, y); return *this; }
    ShaderProgram &u(const char *var, GLfloat x, GLfloat y, GLfloat z) { glUniform3fARB(uniform(var), x, y, z); return *this; }
    ShaderProgram &u(const char *var, GLfloat x, GLfloat y, GLfloat z, GLfloat w) { glUniform4fARB(uniform(var), x, y, z, w); return *this; }
    
    ShaderProgram &u(const char *var, GLint x) { glUniform1iARB(uniform(var), x); return *this; }
    ShaderProgram &u(const char *var, GLint x, GLint y) { glUniform2iARB(uniform(var), x, y); return *this; }
    ShaderProgram &u(const char *var, GLint x, GLint y, GLint z) { glUniform3iARB(uniform(var), x, y, z); return *this; }
    ShaderProgram &u(const char *var, GLint x, GLint y, GLint z, GLint w) { glUniform4iARB(uniform(var), x, y, z, w); return *this; }

    ShaderProgram &u1(const char *var, GLfloat *f) { glUniform1fvARB(uniform(var), 1, f); return *this; }
    ShaderProgram &u2(const char *var, GLfloat *f) { glUniform2fvARB(uniform(var), 1, f); return *this; }
    ShaderProgram &u3(const char *var, GLfloat *f) { glUniform3fvARB(uniform(var), 1, f); return *this; }
    ShaderProgram &u4(const char *var, GLfloat *f) { glUniform4fvARB(uniform(var), 1, f); return *this; }

    ShaderProgram &u1(const char *var, GLint *f) { glUniform1ivARB(uniform(var), 1, f); return *this; }
    ShaderProgram &u2(const char *var, GLint *f) { glUniform2ivARB(uniform(var), 1, f); return *this; }
    ShaderProgram &u3(const char *var, GLint *f) { glUniform3ivARB(uniform(var), 1, f); return *this; }
    ShaderProgram &u4(const char *var, GLint *f) { glUniform4ivARB(uniform(var), 1, f); return *this; }

    ShaderProgram &um2(const char *var, GLfloat *m) { glUniformMatrix2fvARB(uniform(var), 1, GL_FALSE, m); return *this; }
    ShaderProgram &um3(const char *var, GLfloat *m) { glUniformMatrix3fvARB(uniform(var), 1, GL_FALSE, m); return *this; }
    ShaderProgram &um4(const char *var, GLfloat *m) { glUniformMatrix4fvARB(uniform(var), 1, GL_FALSE, m); return *this; }

    ShaderProgram &umt2(const char *var, GLfloat *m) { glUniformMatrix2fvARB(uniform(var), 1, GL_TRUE, m); return *this; }
    ShaderProgram &umt3(const char *var, GLfloat *m) { glUniformMatrix3fvARB(uniform(var), 1, GL_TRUE, m); return *this; }
    ShaderProgram &umt4(const char *var, GLfloat *m) { glUniformMatrix4fvARB(uniform(var), 1, GL_TRUE, m); return *this; }
    
    
    ShaderProgram &ut(const char *var, int u) {
      glUniform1iARB(uniform(var), u);
      return *this;
    }
    ShaderProgram &ut(const char *var, int u, Surface *s) {
      glUniform1iARB(uniform(var), u);
      s->bind(u);
      return *this;
    }
    ShaderProgram &ut(const char *var, int u, GLenum tgt, GLuint tex) {
      glUniform1iARB(uniform(var), u);
      glActiveTexture(GL_TEXTURE0 + u);
      glBindTexture(tgt, tex);
      return *this;
    }
  };

}
