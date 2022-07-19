#pragma once

#include "../shader.hpp"
#include "../../../gpu.hpp"
#include "../../buffer.hpp"
#include "../../../../system/common/projection.hpp"
#include "../../texture.hpp"

#include "../object/shader.hpp" // for ubo

#include "gl.vert.h"
#include "gl.frag.h"
#include "gl2.vert.h"
#include "gl2.frag.h"


namespace plate {


class shader_spheres {

public:


// the ubo used is shader_object::ubo
//
//  struct ubo
//  {
//    gpu::float_vec4 offset;
//    gpu::float_vec4 rot;
//    gpu::float_vec4 scale;
//  };


  template<class T1, class T2> // eg: short, short
  struct basic_vert
  {
    std::array<T1, 4> position;
  };


  template<class T1, class T2> // eg: short, std::uint8_t
  struct basic_inst
  {
    std::array<T1, 3> offset;
    T1                scale;
    std::array<T2, 4> color;  // with color[3] being set to 1 (or eg: 255)
  };


  shader_spheres(int version) noexcept
  {
    if (version == 1)
      std::tie(vertex_shader_, fragment_shader_) = plate::compile(
                      {reinterpret_cast<const char*>(spheres_gl_vert), spheres_gl_vert_len},
                      {reinterpret_cast<const char*>(spheres_gl_frag), spheres_gl_frag_len});
    else // version == 2
      std::tie(vertex_shader_, fragment_shader_) = plate::compile(
                      {reinterpret_cast<const char*>(spheres_gl2_vert), spheres_gl2_vert_len},
                      {reinterpret_cast<const char*>(spheres_gl2_frag), spheres_gl2_frag_len});
  }


  void link() noexcept
  { 
    program_ = plate::link(vertex_shader_,       fragment_shader_);
  }


  bool check() noexcept
  { 
    if (!plate::check(vertex_shader_, fragment_shader_, program_))
      return false;

    attrib_position_ = glGetAttribLocation   (program_, "v_position");

    attrib_offset_   = glGetAttribLocation   (program_, "i_offset");
    attrib_scale_    = glGetAttribLocation   (program_, "i_scale");
    attrib_color_    = glGetAttribLocation   (program_, "i_color");

    uniform_proj_    = glGetUniformLocation(program_, "u_proj");
    uniform_offset_  = glGetUniformLocation(program_, "u_offset");
    uniform_rot_     = glGetUniformLocation(program_, "u_rot");
    uniform_scale_   = glGetUniformLocation(program_, "u_scale");
    uniform_m_       = glGetUniformLocation(program_, "u_m");

    return true;
  }

  // store the buffer configs into a vertex_array_object and use that for drawing

  template<class VB, class IB>
  void draw(const projection& p, buffer<shader_object::ubo>& ubuf, const buffer<VB>& vbuf,
                                                        const buffer<IB>& ibuf, std::uint32_t* vertex_array_object)
  {
    if (*vertex_array_object == 0) // create the Vertex Array
    {
      glGenVertexArrays(1, vertex_array_object);

      glBindVertexArray(*vertex_array_object);

      // object/vertex buffer

      glBindBuffer(GL_ARRAY_BUFFER, vbuf.id_);

      const auto pos_type = vbuf.template get_type<0>();

      glEnableVertexAttribArray(attrib_position_);
      glVertexAttribPointer(attrib_position_, 3, pos_type, pos_type == GL_FLOAT ? GL_FALSE : GL_TRUE, sizeof(VB), nullptr);

      // instance buffer

      glBindBuffer(GL_ARRAY_BUFFER, ibuf.id_);

      const auto offset_type = ibuf.template get_type<0>();
      const auto scale_type  = ibuf.template get_type<1>();
      const auto color_type  = ibuf.template get_type<2>();

      glEnableVertexAttribArray(attrib_offset_);
      glVertexAttribDivisor(attrib_offset_, 1);
      glVertexAttribPointer(attrib_offset_, 3, offset_type, offset_type == GL_FLOAT ? GL_FALSE : GL_TRUE, sizeof(IB), nullptr);

      glEnableVertexAttribArray(attrib_scale_);
      glVertexAttribDivisor(attrib_scale_, 1);
      glVertexAttribPointer(attrib_scale_, 1, scale_type, GL_FALSE, sizeof(IB), (const GLvoid *)(0 + ibuf.template get_size_in_bytes_up_to<1>()));

      glEnableVertexAttribArray(attrib_color_);
      glVertexAttribDivisor(attrib_color_, 1);
      glVertexAttribPointer(attrib_color_, 4, color_type, color_type == GL_FLOAT ? GL_FALSE : GL_TRUE, sizeof(IB), (const GLvoid *)(0 + ibuf.template get_size_in_bytes_up_to<2>()));

      glBindVertexArray(0);

      glDisableVertexAttribArray(attrib_position_);

      glDisableVertexAttribArray(attrib_offset_);
      glDisableVertexAttribArray(attrib_scale_);
      glDisableVertexAttribArray(attrib_color_);
    }

    // draw

    glUseProgram(program_);

    auto u = ubuf.map_staging();

    glUniformMatrix4fv(uniform_proj_, 1, GL_FALSE, p.matrix_.data());

    glUniform4f(uniform_offset_, u->offset.x, u->offset.y, u->offset.z, u->offset.w);
    glUniform4f(uniform_rot_,    u->rot.x, u->rot.y, u->rot.z, u->rot.w);
    glUniform4f(uniform_scale_,  u->scale.x, u->scale.y, u->scale.z, u->scale.w);

    glBindVertexArray(*vertex_array_object);

    glDrawArraysInstanced(vbuf.get_mode(), 0, vbuf.count_, ibuf.count_);

    glBindVertexArray(0);
  }


private:


  GLuint program_         = 0;

  GLuint vertex_shader_   = 0;
  GLuint fragment_shader_ = 0;

  GLint  attrib_position_ = 0;

  GLint  attrib_offset_   = 0;
  GLint  attrib_scale_    = 0;
  GLint  attrib_color_    = 0;

  GLint  uniform_proj_   = 0;
  GLint  uniform_offset_ = 0;
  GLint  uniform_rot_    = 0;
  GLint  uniform_scale_  = 0;
  GLint  uniform_m_      = 0;

}; // shader_spheres


} // namespace plate
