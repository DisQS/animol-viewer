#pragma once

#include "../shader.hpp"
#include "../../../gpu.hpp"
#include "../../buffer.hpp"
#include "../../../../system/common/projection.hpp"
#include "../../texture.hpp"

#include "gl.vert.h"
#include "gl.frag.h"

namespace plate {


class shader_object {

public:

  struct ubo
  {
    gpu::float_vec4 offset;
    gpu::float_vec4 rot;
    gpu::float_vec4 scale;
  };


  /* requires a vertex buffer of the form:

  struct V
  {
    std::array<T1, 3> position;
    std::array<T2, 3> normal;
    std::array<T3, 4> color;
  }

  or: two buffers, a vertex buffer and a color buffer of the forms:

  struct V
  {
    std::array<T1, 3> position;
    std::array<T2, 3> normal;
  }

  struct C
  {
    std::array<T3, 4> color;
  }

  */


  shader_object()
  {
    if (auto r = build_program({reinterpret_cast<const char*>(object_gl_vert), object_gl_vert_len},
                               {reinterpret_cast<const char*>(object_gl_frag), object_gl_frag_len}); !r)
      return;
    else
    { 
      vertex_shader_   = r->vertex_shader_id;
      fragment_shader_ = r->fragment_shader_id;
      program_         = r->program_id;
    }

    attrib_position_ = glGetAttribLocation   (program_, "position");
    attrib_normal_   = glGetAttribLocation   (program_, "normal");
    attrib_color_    = glGetAttribLocation   (program_, "color");

    uniform_proj_    = glGetUniformLocation(program_, "proj");
    uniform_offset_  = glGetUniformLocation(program_, "offset");
    uniform_rot_     = glGetUniformLocation(program_, "rot");
    uniform_scale_   = glGetUniformLocation(program_, "scale");
    uniform_bg_color_= glGetUniformLocation(program_, "bg_color");
  }


  shader_object(bool compile) noexcept
  {
    if (compile)
      std::tie(vertex_shader_, fragment_shader_) = plate::compile(
                                {reinterpret_cast<const char*>(object_gl_vert), object_gl_vert_len},
                                {reinterpret_cast<const char*>(object_gl_frag), object_gl_frag_len});
  }


  void link() noexcept
  {
    program_ = plate::link(vertex_shader_, fragment_shader_);
  }


  bool check() noexcept
  {
    if (!plate::check(vertex_shader_, fragment_shader_, program_))
      return false;

    attrib_position_ = glGetAttribLocation   (program_, "position");
    attrib_normal_   = glGetAttribLocation   (program_, "normal");
    attrib_color_    = glGetAttribLocation   (program_, "color");

    uniform_proj_    = glGetUniformLocation(program_, "proj");
    uniform_offset_  = glGetUniformLocation(program_, "offset");
    uniform_rot_     = glGetUniformLocation(program_, "rot");
    uniform_scale_   = glGetUniformLocation(program_, "scale");
    uniform_bg_color_= glGetUniformLocation(program_, "bg_color");

    return true;
  }


  template<class V>
  void draw(const projection& p, const gpu::color& bg_color, buffer<ubo>& ubuf, const buffer<V>& vbuf)
  {
    auto u = ubuf.map_staging();

    glUseProgram(program_);

    glUniformMatrix4fv(uniform_proj_, 1, GL_FALSE, p.matrix_.data());

    glUniform4f(uniform_offset_, u->offset.x, u->offset.y, u->offset.z, u->offset.w);
    glUniform4f(uniform_rot_,    u->rot.x, u->rot.y, u->rot.z, u->rot.w);
    glUniform4f(uniform_scale_,  u->scale.x, u->scale.y, u->scale.z, u->scale.w);
    glUniform4f(uniform_bg_color_,  bg_color.r, bg_color.g, bg_color.b, bg_color.a);

    glBindBuffer(GL_ARRAY_BUFFER, vbuf.id_);

    const auto pos_type  = vbuf.template get_type<0>();
    const auto norm_type = vbuf.template get_type<1>();
    const auto col_type  = vbuf.template get_type<2>();

    glEnableVertexAttribArray(attrib_position_);
    glVertexAttribPointer(attrib_position_,  3, pos_type, pos_type == GL_FLOAT ? GL_FALSE : GL_TRUE, sizeof(V), nullptr);

    glEnableVertexAttribArray(attrib_normal_);
    glVertexAttribPointer(attrib_normal_, 3, norm_type, norm_type == GL_FLOAT ? GL_FALSE : GL_TRUE, sizeof(V),
                                                                      (const GLvoid *)(0 + vbuf.template get_size_in_bytes<0>()));

    glEnableVertexAttribArray(attrib_color_);
    glVertexAttribPointer(attrib_color_, 4, col_type, col_type == GL_FLOAT ? GL_FALSE : GL_TRUE, sizeof(V),
                               (const GLvoid *)(0 + vbuf.template get_size_in_bytes<0>() + vbuf.template get_size_in_bytes<1>()));

    glDrawArrays(vbuf.get_mode(), 0, vbuf.count_);

    glDisableVertexAttribArray(attrib_position_);
    glDisableVertexAttribArray(attrib_normal_);
    glDisableVertexAttribArray(attrib_color_);
  }


  template<class V, class C>
  void draw(const projection& p, const gpu::color& bg_color, buffer<ubo>& uniform_buffer, const buffer<V>& vbuf, const buffer<C>& cbuf)
  {
    auto u = uniform_buffer.map_staging();

    glUseProgram(program_);

    glUniformMatrix4fv(uniform_proj_, 1, GL_FALSE, p.matrix_.data());

    glUniform4f(uniform_offset_, u->offset.x, u->offset.y, u->offset.z, u->offset.w);
    glUniform4f(uniform_rot_,    u->rot.x, u->rot.y, u->rot.z, u->rot.w);
    glUniform4f(uniform_scale_,  u->scale.x, u->scale.y, u->scale.z, u->scale.w);
    glUniform4f(uniform_bg_color_,  bg_color.r, bg_color.g, bg_color.b, bg_color.a);

    glBindBuffer(GL_ARRAY_BUFFER, vbuf.id_);

    const auto pos_type  = vbuf.template get_type<0>();
    const auto norm_type = vbuf.template get_type<1>();

    glEnableVertexAttribArray(attrib_position_);
    glVertexAttribPointer(attrib_position_,  3, pos_type, pos_type == GL_FLOAT ? GL_FALSE : GL_TRUE, sizeof(V), nullptr);

    glEnableVertexAttribArray(attrib_normal_);
    glVertexAttribPointer(attrib_normal_, 3, norm_type, norm_type == GL_FLOAT ? GL_FALSE : GL_TRUE, sizeof(V),
                                                                      (const GLvoid *)(0 + vbuf.template get_size_in_bytes<0>()));

    glBindBuffer(GL_ARRAY_BUFFER, cbuf.id_);

    const auto col_type  = cbuf.template get_type<0>();

    glEnableVertexAttribArray(attrib_color_);
    glVertexAttribPointer(attrib_color_, 4, col_type, col_type == GL_FLOAT ? GL_FALSE : GL_TRUE, sizeof(C), nullptr);

    glDrawArrays(vbuf.get_mode(), 0, vbuf.count_);

    glDisableVertexAttribArray(attrib_position_);
    glDisableVertexAttribArray(attrib_normal_);
    glDisableVertexAttribArray(attrib_color_);
  }


/* todo
  void draw_elements(const projection& p, buffer& uniform_buffer, const buffer& vertex_buffer, const buffer& element_buffer)
  {
    auto ptr = uniform_buffer.map_staging();
  
    ubo* u = new (ptr) ubo;
  
    glUseProgram(program_);
  
    glUniformMatrix4fv(uniform_proj_, 1, GL_FALSE, p.matrix_.data());
  
    glUniform4f(uniform_offset_, u->offset.x, u->offset.y, u->offset.z, u->offset.w);
    glUniform4f(uniform_rot_,    u->rot.x, u->rot.y, u->rot.z, u->rot.w);
    glUniform4f(uniform_scale_,  u->scale.x, u->scale.y, u->scale.z, u->scale.w);

    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer.id_);

    glEnableVertexAttribArray(attrib_position_);
    glVertexAttribPointer(attrib_position_,  3, GL_SHORT, GL_TRUE, vertex_buffer.stride_, nullptr);

    glEnableVertexAttribArray(attrib_normal_);
    glVertexAttribPointer(attrib_normal_, 3, GL_SHORT, GL_TRUE, vertex_buffer.stride_,
                                                                      (const GLvoid *)(0 + (3 * sizeof(GLshort))));

    glEnableVertexAttribArray(attrib_color_);
    glVertexAttribPointer(attrib_color_, 4, GL_UNSIGNED_BYTE, GL_TRUE, vertex_buffer.stride_,
                                                                      (const GLvoid *)(0 + (6 * sizeof(GLshort))));

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, element_buffer.id_);

    glDrawElements(vertex_buffer.get_mode(), element_buffer.count_, GL_UNSIGNED_INT, 0);

    glDisableVertexAttribArray(attrib_position_);
    glDisableVertexAttribArray(attrib_normal_);
    glDisableVertexAttribArray(attrib_color_);
  }
  */


private:


  GLuint program_         = 0;

  GLuint vertex_shader_   = 0;
  GLuint fragment_shader_ = 0;

  GLint  attrib_position_ = 0;
  GLint  attrib_normal_   = 0;
  GLint  attrib_color_    = 0;

  GLint  attrib_offset_   = 0;
  GLint  attrib_rot_      = 0;
  GLint  attrib_scale_    = 0;

  GLint  uniform_proj_    = 0;
  GLint  uniform_offset_  = 0;
  GLint  uniform_rot_     = 0;
  GLint  uniform_scale_   = 0;
  GLint  uniform_bg_color_= 0;

}; // shader_object


} // namespace plate
