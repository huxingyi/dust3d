#include <gloop/fbo.hpp>

namespace gloop {

  std::vector<GLuint> FrameBuffer::s_bound_fbo(1, 0);
  
  bool FrameBuffer::_attach(GLenum target, const Surface::Ptr &surf, GLenum attachment, int mipmap_level) {
    if (surf == NULL || surf->gl_id == 0) return false;
    if (surf->desc.is_texture) {
      glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, attachment, target, surf->gl_id, mipmap_level);
    } else {
      glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, attachment, target, surf->gl_id);
    }
    return true;
  }
  
  FrameBuffer::FrameBuffer(unsigned _width, unsigned _height) : width(_width), height(_height), fbo(0) {
  }
  
  void FrameBuffer::add(GLenum target, const Surface::Ptr &surf) {
    if (target != GL_TEXTURE_2D &&
        target != GL_TEXTURE_RECTANGLE_ARB &&
        target != GL_RENDERBUFFER_EXT &&
        !(GL_TEXTURE_CUBE_MAP_POSITIVE_X <= target && target < GL_TEXTURE_CUBE_MAP_POSITIVE_X + 6)) {
      throw std::runtime_error("Invalid target");
    }
    
    switch (surf->desc.surface_type) {
      case SURF_NONE:
        throw std::runtime_error("Invalid surface");
      case SURF_COLOUR:
        colour.push_back(std::make_pair(target, surf));
        break;
      case SURF_DEPTH:
        depth = std::make_pair(target, surf);
        break;
      case SURF_STENCIL:
        stencil = std::make_pair(target, surf);
        break;
      case SURF_DEPTH_STENCIL:
        depth = stencil = std::make_pair(target, surf);
        break;
    }
  }
  
  void FrameBuffer::add(const Surface::Ptr &surf) {
    add(surf->desc.gl_tgt, surf);
  }
  
  void FrameBuffer::init() {
    for (unsigned i = 0; i < colour.size(); ++i) {
      colour[i].second->init(width, height);
    }
    
    if (depth.second != NULL) depth.second->init(width, height);
    if (stencil.second != NULL && depth.second != stencil.second) stencil.second->init(width, height);
    
    glGenFramebuffersEXT(1, &fbo);
    if (fbo == 0) {
      throw std::runtime_error("Failed to create Framebuffer");
    }
  }
  
  void FrameBuffer::pushBind() {
    if (fbo != s_bound_fbo.back()) {
      glFlush();
      glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);
    }
    s_bound_fbo.push_back(fbo);
  }
  
  void FrameBuffer::bind() {
    if (s_bound_fbo.size() == 1) {
      pushBind();
    } else {
      if (fbo != s_bound_fbo.back()) {
        glFlush();
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);
        s_bound_fbo.back() = fbo;
      }
    }
  }
  
  void FrameBuffer::popBind() {
    if (s_bound_fbo.size() > 1) {
      GLuint cur_fbo = s_bound_fbo.back();
      s_bound_fbo.pop_back();
      if (cur_fbo != s_bound_fbo.back()) {
        glFlush();
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, s_bound_fbo.back());
      }
    }
  }
  
  void FrameBuffer::attach(int mipmap_level) {
    if (fbo == 0) throw std::runtime_error("Not initialized");
    
    bind();
    
    for (unsigned i = 0; i < colour.size(); ++i) {
      _attach(colour[i].first, colour[i].second, GL_COLOR_ATTACHMENT0_EXT + i, mipmap_level);
    }
    _attach(depth.first, depth.second, GL_DEPTH_ATTACHMENT_EXT, mipmap_level);
    _attach(stencil.first, stencil.second, GL_STENCIL_ATTACHMENT_EXT, mipmap_level);
  }
  
}