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

#include <gloop/shader.hpp>

namespace gloop {

  int Shader::s_tag = 0;

  static inline std::string glsl_log(GLuint handle) {
    if (handle == 0) return "";
    
    GLint log_len;
    glGetObjectParameterivARB(handle, GL_OBJECT_INFO_LOG_LENGTH_ARB, &log_len);
    if (log_len == 0) return "";
    
    std::string log(log_len, '\0');
    GLsizei written;
    glGetInfoLogARB(handle, log_len, &written, (GLcharARB *)log.c_str());
    log.resize(written);
    return log;
  }

  void Shader::_source(std::list<std::string> &result) const {
    if (tag == s_tag) return;
    tag = s_tag;
        
    for (std::list<Ptr>::const_iterator i = dependencies.begin(), e = dependencies.end(); i != e; ++i) {
      (*i)->_source(result);
    }
    result.push_back(prog);
  }
    
  void Shader::_compile() {
    if (shader != 0) return;
      
    shader = glCreateShaderObjectARB(shaderType());
    if (shader == 0) {
      throw std::runtime_error("failed to create shader object");
    }
      
    const char *_prog = prog.c_str();
    GLint length = prog.size();
      
    glShaderSourceARB(shader, 1, &_prog, &length);
    glCompileShaderARB(shader);
      
    GLint compile_status = 0;
    glGetObjectParameterivARB(shader, GL_OBJECT_COMPILE_STATUS_ARB, &compile_status);
    if (compile_status == 0) {
      std::ostringstream err;
      err << "failed to compile shader:" << std::endl << glsl_log(shader) << std::endl;
        
      glDeleteObjectARB(shader);
      shader = 0;
      throw std::runtime_error(err.str());
    }
  }
    
  void Shader::_attachTo(GLuint program, bool compile) {
    if (tag == s_tag) return;
    tag = s_tag;
      
    for (std::list<Ptr>::const_iterator i = dependencies.begin(), e = dependencies.end(); i != e; ++i) {
      (*i)->_attachTo(program, compile);
    }
    if (compile && shader == 0) _compile();
    if (shader != 0) {
      glAttachObjectARB(program, shader);
    }
  }
    
  Shader::Shader(const std::string &_name, const std::string &_prog) :
      name(_name), prog(_prog), shader(0), tag(-1), dependencies() {
  }

  static inline std::string _read_stream(std::istream &_stream) {
    char buf[1024];
    std::string r;
    while (_stream.good()) {
      _stream.read(buf, 1024);
      r.append(buf, _stream.gcount());
    }
    return r;
  }

  Shader::Shader(const std::string &_name, std::istream &_stream) :
      name(_name), prog(_read_stream(_stream)), shader(0), tag(-1), dependencies() {
  }

  Shader::~Shader() {
    destroy();
  }
    
  void Shader::addDependency(const Ptr &dep) {
    dependencies.push_back(dep);
  }
      
  void Shader::compileFlat() {
    if (shader != 0) return;
      
    shader = glCreateShaderObjectARB(shaderType());
    if (shader == 0) {
      throw std::runtime_error("failed to create shader object");
    }
      
    std::list<std::string> all_source;
    s_tag++;
    _source(all_source);

    std::ostringstream _prog;
    std::copy(all_source.begin(), all_source.end(), std::ostream_iterator<std::string>(_prog, "\n"));
      
    GLint length = _prog.str().size();
    const char *_p = _prog.str().c_str();
    glShaderSourceARB(shader, 1, &_p, &length);
    glCompileShaderARB(shader);
      
    GLint compile_status = 0;
    glGetObjectParameterivARB(shader, GL_OBJECT_COMPILE_STATUS_ARB, &compile_status);
    if (compile_status == 0) {
      std::ostringstream err;
      err << "failed to compile shader:" << std::endl << "----" << std::endl << glsl_log(shader) << std::endl << "----" << std::endl;

      glDeleteObjectARB(shader);
      shader = 0;
      throw std::runtime_error(err.str());
    }
  }
    
  void Shader::compile() {
    if (shader != 0) return;
    
    for (std::list<Ptr>::const_iterator i = dependencies.begin(), e = dependencies.end(); i != e; ++i) {
      (*i)->compile();
    }
    _compile();
  }

  void Shader::attachFlat(GLuint program, bool compile) {
    if (shader == 0 && compile) {
      compileFlat();
    }
    if (shader != 0) {
      glAttachObjectARB(program, shader);
    }
  }

  void Shader::attachTo(GLuint program, bool compile) {
    s_tag++;
    _attachTo(program, compile);
  }

  void Shader::destroy() {
    if (shader != 0) {
      glDeleteObjectARB(shader);
      shader = 0;
    }
  }

  GLenum VertexShader::shaderType() {
    return GL_VERTEX_SHADER_ARB;
  }
    
  VertexShader::VertexShader(const std::string &_name, const std::string &_prog) :
      Shader(_name, _prog) {
  }

  VertexShader::VertexShader(const std::string &_name, std::istream &_stream) :
      Shader(_name, _stream) {
  }

  VertexShader::~VertexShader() {
  }

  GLenum FragmentShader::shaderType() {
    return GL_FRAGMENT_SHADER_ARB;
  }
    
  FragmentShader::FragmentShader(const std::string &_name, const std::string &_prog) :
      Shader(_name, _prog) {
  }

  FragmentShader::FragmentShader(const std::string &_name, std::istream &_stream) :
      Shader(_name, _stream) {
  }

  FragmentShader::~FragmentShader() {
  }


  ShaderProgram::ShaderProgram() : vertex_shader(NULL), fragment_shader(NULL), program(0) {
  }

  void ShaderProgram::destroy() {
    if (program != 0) {
      glDeleteObjectARB(program);
      program = 0;
    }
  }

  ShaderProgram::~ShaderProgram() {
    destroy();
  }

  ShaderProgram &ShaderProgram::connect(VertexShader *shader) {
    destroy();
    vertex_shader = shader;
    return *this;
  }

  ShaderProgram &ShaderProgram::connect(FragmentShader *shader) {
    destroy();
    fragment_shader = shader;
    return *this;
  }

  ShaderProgram &ShaderProgram::connect(const VertexShader::Ptr &shader) {
    destroy();
    vertex_shader = shader;
    return *this;
  }

  ShaderProgram &ShaderProgram::connect(const FragmentShader::Ptr &shader) {
    destroy();
    fragment_shader = shader;
    return *this;
  }

  ShaderProgram &ShaderProgram::connect(Shader *shader) {
    VertexShader *v;
    FragmentShader *f;
    v = dynamic_cast<VertexShader *>(shader); if (v) connect(v);
    f = dynamic_cast<FragmentShader *>(shader); if (f) connect(f);
    return *this;
  }

  ShaderProgram &ShaderProgram::connect(Shader::Ptr &shader) {
    VertexShader *v;
    FragmentShader *f;
    v = dynamic_cast<VertexShader *>(shader.ptr()); if (v) connect(v);
    f = dynamic_cast<FragmentShader *>(shader.ptr()); if (f) connect(f);
    return *this;
  }

  ShaderProgram &ShaderProgram::link() {
    if (vertex_shader != NULL) vertex_shader->compileFlat();
    if (fragment_shader != NULL) fragment_shader->compileFlat();
    
    program = glCreateProgramObjectARB();
    if (program == 0) {
      throw std::runtime_error("failed to create program object");
    }
    
    if (vertex_shader != NULL) vertex_shader->attachFlat(program);
    if (fragment_shader != NULL) fragment_shader->attachFlat(program);
    
    glLinkProgramARB(program);
    
    GLint link_status = 0;
    glGetObjectParameterivARB(program, GL_OBJECT_LINK_STATUS_ARB, &link_status);
    
    if (link_status == 0) {
      std::string err = glsl_log(program);
      destroy();
      throw std::runtime_error(err);
    }
    return *this;
  }
  
}
