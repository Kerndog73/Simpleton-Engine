//
//  vertex array.hpp
//  Simpleton Engine
//
//  Created by Indi Kernick on 19/12/17.
//  Copyright © 2017 Indi Kernick. All rights reserved.
//

#ifndef engine_opengl_vertex_array_hpp
#define engine_opengl_vertex_array_hpp

#include "opengl.hpp"
#include "generic raii.hpp"

namespace GL {
  namespace detail {
    void deleteVertexArray(const GLuint &);
  }
  
  class VertexArray {
  public:
    RAII_CLASS_MEMBERS(VertexArray, GLuint, id, detail::deleteVertexArray)
    
    void bind() const;
  
  private:
    GLuint id;
  };
  
  void unbindVertexArray();
  
  VertexArray makeVertexArray();
}

#include "vertex array.inl"

#endif