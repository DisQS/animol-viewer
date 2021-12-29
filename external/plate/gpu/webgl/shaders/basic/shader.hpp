#pragma once

#include "../shader.hpp"
#include "../../../gpu.hpp"
#include "../../buffer.hpp"
#include "../../../../system/common/projection.hpp"

#include "gl.vert.h"
#include "gl.frag.h"

namespace plate {


class shader_basic {

public:

  struct ubo
  {
    gpu::float_vec4 offset;
    gpu::float_vec4 color;
    gpu::float_vec2 scale;
  };

  struct basic_vertex // a basic struct for sending through vertices
  {
    std::array<float, 4> position;
  };

  shader_basic()
  {
    if (auto r = build_program({reinterpret_cast<const char*>(basic_gl_vert), basic_gl_vert_len},
                               {reinterpret_cast<const char*>(basic_gl_frag), basic_gl_frag_len}); !r)
      return;
    else
    { 
      vertex_shader_   = r->vertex_shader_id;
      fragment_shader_ = r->fragment_shader_id;
      program_         = r->program_id;
    }

    attrib_position_ = glGetAttribLocation(program_,  "position");
    uniform_proj_    = glGetUniformLocation(program_, "proj");
    uniform_offset_  = glGetUniformLocation(program_, "offset");
    uniform_scale_   = glGetUniformLocation(program_, "scale");
    uniform_col_     = glGetUniformLocation(program_, "col");
  };


  shader_basic(bool compile) noexcept
  {
    if (compile)
      std::tie(vertex_shader_, fragment_shader_) = plate::compile(
                                {reinterpret_cast<const char*>(basic_gl_vert), basic_gl_vert_len},
                                {reinterpret_cast<const char*>(basic_gl_frag), basic_gl_frag_len});
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
    uniform_proj_    = glGetUniformLocation(program_, "proj");
    uniform_offset_  = glGetUniformLocation(program_, "offset");
    uniform_scale_   = glGetUniformLocation(program_, "scale");
    uniform_col_     = glGetUniformLocation(program_, "col");

    return true;
  };



  template<class V>
  void draw(const projection& p, const float& a, buffer<ubo>& uniform_buffer, const buffer<V>& vbuf)
  {
    auto u = uniform_buffer.map_staging();

    glUseProgram(program_);

    glUniformMatrix4fv(uniform_proj_, 1, GL_FALSE, p.matrix_.data());

    glUniform4f(uniform_offset_, u->offset.x, u->offset.y, 0.0, 0.0);
    glUniform2f(uniform_scale_,  u->scale.x, u->scale.y);
    glUniform4f(uniform_col_,    u->color.r, u->color.g, u->color.b, u->color.a * a);

    glBindBuffer(GL_ARRAY_BUFFER, vbuf.id_);

    const auto pos_type  = vbuf.template get_type<0>();

    glEnableVertexAttribArray(attrib_position_);
    glVertexAttribDivisor(attrib_position_, 0);
    glVertexAttribPointer(attrib_position_, 4, pos_type, pos_type == GL_FLOAT ? GL_FALSE : GL_TRUE, sizeof(V), nullptr);

    glDrawArrays(GL_TRIANGLES, 0, vbuf.count_);

    glDisableVertexAttribArray(attrib_position_);
  };


private:


  GLuint program_         = 0;

  GLuint vertex_shader_   = 0;
  GLuint fragment_shader_ = 0;

  GLint  attrib_position_ = 0;
  GLint  uniform_proj_    = 0;
  GLint  uniform_offset_  = 0;
  GLint  uniform_scale_   = 0;
  GLint  uniform_col_     = 0;

}; // shader_basic


} // namespace plate
