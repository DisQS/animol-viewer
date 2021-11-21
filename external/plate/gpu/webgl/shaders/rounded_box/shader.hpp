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


class shader_rounded_box {

public:

  struct ubo
  {
    gpu::float_vec4 offset;
    gpu::float_vec4 color;
    gpu::float_vec2 scale;
    gpu::float_vec2 dim;
    float           radius;
  };


  struct basic // a basic struct for sending through vertices
  {
    std::array<float, 4> position;
    std::array<float, 2> coords;
  };


  shader_rounded_box(int v)
  {
    std::string_view vertex_source;
    std::string_view fragment_source;
    
    if (v == 1)
    {
      vertex_source   = std::string_view(reinterpret_cast<const char*>(rounded_box_gl_vert), rounded_box_gl_vert_len);   
      fragment_source = std::string_view(reinterpret_cast<const char*>(rounded_box_gl_frag), rounded_box_gl_frag_len);   
    }
    else
    {
      vertex_source   = std::string_view(reinterpret_cast<const char*>(rounded_box_gl2_vert), rounded_box_gl2_vert_len); 
      fragment_source = std::string_view(reinterpret_cast<const char*>(rounded_box_gl2_frag), rounded_box_gl2_frag_len); 
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
    uniform_dim_     = glGetUniformLocation(program_, "dim");
    uniform_radius_  = glGetUniformLocation(program_, "radius");
  };

  shader_rounded_box(int v, bool compile) noexcept
  {
    if (compile)
    {
      if (v == 1)
        std::tie(vertex_shader_, fragment_shader_) = plate::compile(
                                {reinterpret_cast<const char*>(rounded_box_gl_vert), rounded_box_gl_vert_len},
                                {reinterpret_cast<const char*>(rounded_box_gl_frag), rounded_box_gl_frag_len});
      else
        std::tie(vertex_shader_, fragment_shader_) = plate::compile(
                                {reinterpret_cast<const char*>(rounded_box_gl2_vert), rounded_box_gl2_vert_len},
                                {reinterpret_cast<const char*>(rounded_box_gl2_frag), rounded_box_gl2_frag_len});
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
    uniform_dim_     = glGetUniformLocation(program_, "dim");
    uniform_radius_  = glGetUniformLocation(program_, "radius");

    return true;
  };


  template<class V>
  void draw(const projection& p, const float& alpha, buffer<ubo>& uniform_buffer, const buffer<V>& vbuf)
  {
    auto u = uniform_buffer.map_staging();

    glUseProgram(program_);

    glUniformMatrix4fv(uniform_proj_, 1, GL_FALSE, p.matrix_.data());

    glUniform4f(uniform_offset_, u->offset.x, u->offset.y, 0.0, 0.0);
    glUniform2f(uniform_scale_,  u->scale.x, u->scale.y);
    glUniform4f(uniform_col_,    u->color.r, u->color.g, u->color.b, u->color.a * alpha);
    glUniform2f(uniform_dim_,    u->dim.x, u->dim.y);
    glUniform1f(uniform_radius_, u->radius);

    glBindBuffer(GL_ARRAY_BUFFER, vbuf.id_);

    const auto pos_type    = vbuf.template get_type<0>();
    const auto coords_type = vbuf.template get_type<1>();

    glEnableVertexAttribArray(attrib_position_);
    glVertexAttribDivisor(attrib_position_, 0);
    glVertexAttribPointer(attrib_position_, 4, pos_type, pos_type == GL_FLOAT ? GL_FALSE : GL_TRUE, sizeof(V), nullptr);

    glEnableVertexAttribArray(attrib_coords_);
    glVertexAttribDivisor(attrib_coords_, 0);
    glVertexAttribPointer(attrib_coords_, 2, coords_type, coords_type == GL_FLOAT ? GL_FALSE : GL_TRUE, sizeof(V),
                                                                  (const GLvoid *)(0 + vbuf.template get_size_in_bytes<0>()));

    glDrawArrays(vbuf.get_mode(), 0, vbuf.count_);

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
  GLint  uniform_dim_     = 0;
  GLint  uniform_radius_  = 0;

}; // shader_rounded_box


} // namespace plate
