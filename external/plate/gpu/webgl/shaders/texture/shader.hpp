#pragma once

#include "../shader.hpp"
#include "../../../gpu.hpp"
#include "../../buffer.hpp"
#include "../../texture.hpp"
#include "../../../../system/common/projection.hpp"

#include "gl.vert.h"
#include "gl.frag.h"

namespace plate {


class shader_texture {

public:

  struct ubo
  {
    gpu::float_vec4 offset;
    gpu::float_vec4 rot;
    gpu::float_vec2 scale;
  };

  struct basic  // a basic form of supplying coords
  {
    std::array<float, 4> position;
    std::array<float, 2> tex_coord;
  };


  shader_texture(bool compile) noexcept
  {
    std::tie(vertex_shader_, fragment_shader_) = plate::compile(
                              {reinterpret_cast<const char*>(texture_gl_vert), texture_gl_vert_len},
                              {reinterpret_cast<const char*>(texture_gl_frag), texture_gl_frag_len});
  }


  void link() noexcept
  {
    program_ = plate::link(vertex_shader_, fragment_shader_);
  }


  bool check() noexcept
  {
    if (!plate::check(vertex_shader_, fragment_shader_, program_))
      return false;

    attrib_position_  = glGetAttribLocation   (program_, "position");
    attrib_tex_coord_ = glGetAttribLocation   (program_, "tex_coord");

    uniform_tex_      = glGetUniformLocation  (program_, "tex");

    uniform_proj_    = glGetUniformLocation(program_, "proj");
    uniform_alpha_   = glGetUniformLocation(program_, "alpha");
    uniform_offset_  = glGetUniformLocation(program_, "offset");
    uniform_rot_     = glGetUniformLocation(program_, "rot");
    uniform_scale_   = glGetUniformLocation(program_, "scale");

    return true;
  }


  template<class V>
  void draw(const projection& p, float alpha, buffer<ubo>& uniform_buffer, const buffer<V>& vbuf, const texture& tex)
  {
    auto u = uniform_buffer.map_staging();

    glUseProgram(program_);

    glUniformMatrix4fv(uniform_proj_, 1, GL_FALSE, p.matrix_.data());

    glUniform1f(uniform_alpha_,  alpha);
    glUniform4f(uniform_offset_, u->offset.x, u->offset.y, 0.0, 0.0);
    glUniform4f(uniform_rot_,    u->rot.x, u->rot.y, u->rot.z, u->rot.a);
    glUniform2f(uniform_scale_,  u->scale.x, u->scale.y);

    glUniform1i(uniform_tex_, 0);

    glBindBuffer(GL_ARRAY_BUFFER, vbuf.id_);

    const auto pos_type   = vbuf.template get_type<0>();
    const auto coord_type = vbuf.template get_type<1>();

    glEnableVertexAttribArray(attrib_position_);
    glVertexAttribDivisor(attrib_position_, 0);
    glVertexAttribPointer(attrib_position_,  4, pos_type, pos_type == GL_FLOAT ? GL_FALSE : GL_TRUE, sizeof(V), nullptr);

    glEnableVertexAttribArray(attrib_tex_coord_);
    glVertexAttribDivisor(attrib_tex_coord_, 0);
    glVertexAttribPointer(attrib_tex_coord_, 2, coord_type, coord_type == GL_FLOAT ? GL_FALSE : GL_TRUE, sizeof(V),
                                                          (const GLvoid *)(0 +   vbuf.template get_size_in_bytes<0>()));

    glBindTexture(GL_TEXTURE_2D, tex.get_id());

    glDrawArrays(vbuf.get_mode(), 0, vbuf.count_);

    glDisableVertexAttribArray(attrib_position_);
    glDisableVertexAttribArray(attrib_tex_coord_);
  };


private:


  GLuint program_         = 0;

  GLuint vertex_shader_   = 0;
  GLuint fragment_shader_ = 0;

  GLint  attrib_position_  = 0;
  GLint  attrib_tex_coord_ = 0;

  GLint  uniform_proj_     = 0;
  GLint  uniform_alpha_    = 0;
  GLint  uniform_offset_   = 0;
  GLint  uniform_rot_      = 0;
  GLint  uniform_scale_    = 0;

  GLint  uniform_tex_      = 0;

}; // shader_texture


} // namespace plate
