#pragma once

#include "../shader.hpp"
#include "../../../gpu.hpp"
#include "../../buffer.hpp"
#include "../../../../system/common/projection.hpp"

#include "gl.vert.h"
#include "gl.frag.h"
#include "gl2.vert.h"
#include "gl2.frag.h"

namespace plate {


class shader_circle {

public:

  struct ubo
  {
    gpu::float_vec4 offset;
    gpu::float_vec4 color;
    gpu::float_vec2 scale;
  };

  shader_circle(int v)
  {
    std::string_view vertex_source;
    std::string_view fragment_source;
    
    if (v == 1)
    {
      vertex_source   = std::string_view(reinterpret_cast<const char*>(circle_gl_vert), circle_gl_vert_len);   
      fragment_source = std::string_view(reinterpret_cast<const char*>(circle_gl_frag), circle_gl_frag_len);   
    }
    else
    {
      vertex_source   = std::string_view(reinterpret_cast<const char*>(circle_gl2_vert), circle_gl2_vert_len); 
      fragment_source = std::string_view(reinterpret_cast<const char*>(circle_gl2_frag), circle_gl2_frag_len); 
    }

    
    if (auto r = build_program(vertex_source, fragment_source); !r)
      return;
    else
    { 
      vertex_shader_   = r->vertex_shader_id;
      fragment_shader_ = r->fragment_shader_id;
      program_         = r->program_id;
    }

    attrib_position_ = glGetAttribLocation(program_,  "position");
    attrib_coords_   = glGetAttribLocation(program_,  "coord");
    uniform_proj_    = glGetUniformLocation(program_, "proj");
    uniform_offset_  = glGetUniformLocation(program_, "offset");
    uniform_scale_   = glGetUniformLocation(program_, "scale");
    uniform_col_     = glGetUniformLocation(program_, "col");
  };

  shader_circle(int v, bool compile) noexcept
  {
    if (compile)
    {
      if (v == 1)
        std::tie(vertex_shader_, fragment_shader_) = plate::compile(
                                {reinterpret_cast<const char*>(circle_gl_vert), circle_gl_vert_len},
                                {reinterpret_cast<const char*>(circle_gl_frag), circle_gl_frag_len});
      else
        std::tie(vertex_shader_, fragment_shader_) = plate::compile(
                                {reinterpret_cast<const char*>(circle_gl2_vert), circle_gl2_vert_len},
                                {reinterpret_cast<const char*>(circle_gl2_frag), circle_gl2_frag_len});
    }
  };

  void link() noexcept
  {
    program_ = plate::link(vertex_shader_, fragment_shader_);
  };

  bool check() noexcept
  {
    if (!plate::check(vertex_shader_, fragment_shader_, program_))
      return false;

    attrib_position_ = glGetAttribLocation(program_,  "position");
    attrib_coords_   = glGetAttribLocation(program_,  "coord");
    uniform_proj_    = glGetUniformLocation(program_, "proj");
    uniform_offset_  = glGetUniformLocation(program_, "offset");
    uniform_scale_   = glGetUniformLocation(program_, "scale");
    uniform_col_     = glGetUniformLocation(program_, "col");

    return true;
  };


  void draw(const projection& p, buffer& uniform_buffer, const buffer& vertex_buffer)
  {
    auto ptr = uniform_buffer.map_staging();

    ubo* u = new (ptr) ubo;

    glUseProgram(program_);

    glUniformMatrix4fv(uniform_proj_, 1, GL_FALSE, p.matrix_.data());

    glUniform4f(uniform_offset_, u->offset.x, u->offset.y, 0.0, 0.0);
    glUniform2f(uniform_scale_,  u->scale.x, u->scale.y);
    glUniform4f(uniform_col_,    u->color.r, u->color.g, u->color.b, u->color.a);

    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer.id_);

    glEnableVertexAttribArray(attrib_position_);
    glVertexAttribDivisor(attrib_position_, 0);
    glVertexAttribPointer(attrib_position_, 4, GL_FLOAT, GL_FALSE, vertex_buffer.stride_, nullptr);

    glEnableVertexAttribArray(attrib_coords_);
    glVertexAttribDivisor(attrib_coords_, 0);
    glVertexAttribPointer(attrib_coords_, 2, GL_FLOAT, GL_FALSE, vertex_buffer.stride_,
                                                      (const GLvoid *)(0 + (4 * sizeof(GLfloat))));

    glDrawArrays(GL_TRIANGLES, 0, vertex_buffer.count_);

    glDisableVertexAttribArray(attrib_position_);
    glDisableVertexAttribArray(attrib_coords_);
  };


private:


  GLuint program_         = 0;

  GLuint vertex_shader_   = 0;
  GLuint fragment_shader_ = 0;

  GLint  attrib_position_ = 0;
  GLint  attrib_coords_   = 0;
  GLint  uniform_proj_    = 0;
  GLint  uniform_offset_  = 0;
  GLint  uniform_scale_   = 0;
  GLint  uniform_col_     = 0;

}; // shader_circle


} // namespace plate
