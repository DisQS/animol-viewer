#pragma once

#include "../shader.hpp"
#include "../../../gpu.hpp"
#include "../../buffer.hpp"
#include "../../../../system/common/projection.hpp"
#include "../../texture.hpp"

#include "gl.vert.h"
#include "gl.frag.h"

namespace plate {


class shader_text {

public:

  struct ubo
  {
    gpu::float_vec4 offset;
    gpu::float_vec4 rot;
    gpu::float_vec2 scale;
  };

  shader_text()
  {
    if (auto r = build_program({reinterpret_cast<const char*>(text_gl_vert), text_gl_vert_len},
                               {reinterpret_cast<const char*>(text_gl_frag), text_gl_frag_len}); !r)
      return;
    else
    { 
      vertex_shader_   = r->vertex_shader_id;
      fragment_shader_ = r->fragment_shader_id;
      program_         = r->program_id;
    }

    attrib_position_  = glGetAttribLocation   (program_, "position");
    attrib_tex_coord_ = glGetAttribLocation   (program_, "tex_coord");

    uniform_tex_      = glGetUniformLocation  (program_, "tex");
    uniform_col_      = glGetUniformLocation  (program_, "col");

    uniform_proj_    = glGetUniformLocation(program_, "proj");
    uniform_offset_  = glGetUniformLocation(program_, "offset");
    uniform_rot_     = glGetUniformLocation(program_, "rot");
    uniform_scale_   = glGetUniformLocation(program_, "scale");
  };

  shader_text(bool compile) noexcept
  {
    if (compile)
      std::tie(vertex_shader_, fragment_shader_) = plate::compile(
                                {reinterpret_cast<const char*>(text_gl_vert), text_gl_vert_len},
                                {reinterpret_cast<const char*>(text_gl_frag), text_gl_frag_len});
  };

  void link() noexcept
  {
    program_ = plate::link(vertex_shader_, fragment_shader_);
  };

  bool check() noexcept
  {
    if (!plate::check(vertex_shader_, fragment_shader_, program_))
      return false;

    attrib_position_  = glGetAttribLocation   (program_, "position");
    attrib_tex_coord_ = glGetAttribLocation   (program_, "tex_coord");

    uniform_tex_      = glGetUniformLocation  (program_, "tex");
    uniform_col_      = glGetUniformLocation  (program_, "col");

    uniform_proj_    = glGetUniformLocation(program_, "proj");
    uniform_offset_  = glGetUniformLocation(program_, "offset");
    uniform_rot_     = glGetUniformLocation(program_, "rot");
    uniform_scale_   = glGetUniformLocation(program_, "scale");

    return true;
  };


  void draw(const projection& p, buffer& uniform_buffer, const buffer& vertex_buffer,
                                                  const texture& tex, const gpu::color_rgb& c)
  {
    auto ptr = uniform_buffer.map_staging();

    ubo* u = new (ptr) ubo;

    glUseProgram(program_);

    glUniformMatrix4fv(uniform_proj_, 1, GL_FALSE, p.matrix_.data());

    glUniform4f(uniform_offset_, u->offset.x, u->offset.y, 0.0, 0.0);
    glUniform4f(uniform_rot_,    u->rot.x, u->rot.y, u->rot.z, u->rot.a);
    glUniform2f(uniform_scale_,   u->scale.x, u->scale.y);

    glUniform3f(uniform_col_, c.r, c.g, c.b);
    glUniform1i(uniform_tex_, 0);

    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer.id_);

    glEnableVertexAttribArray(attrib_position_);
    glVertexAttribDivisor(attrib_position_, 0);
    glVertexAttribPointer(attrib_position_,  4, GL_FLOAT, GL_FALSE, vertex_buffer.stride_, nullptr);

    glEnableVertexAttribArray(attrib_tex_coord_);
    glVertexAttribDivisor(attrib_tex_coord_, 0);
    glVertexAttribPointer(attrib_tex_coord_, 2, GL_FLOAT, GL_FALSE, vertex_buffer.stride_, (const GLvoid *)(0 +   (4 * sizeof(GLfloat))));

    glBindTexture(GL_TEXTURE_2D, tex.get_id());

    glDrawArrays(GL_TRIANGLES, 0, vertex_buffer.count_);

    glDisableVertexAttribArray(attrib_position_);
    glDisableVertexAttribArray(attrib_tex_coord_);

    //if (auto err = glGetError(); err != GL_NO_ERROR) { LOG_ERROR("got error text YYY: %u", err); }
  };


private:


  GLuint program_         = 0;

  GLuint vertex_shader_   = 0;
  GLuint fragment_shader_ = 0;

  GLint  attrib_position_  = 0;
  GLint  attrib_tex_coord_ = 0;
  GLint  uniform_tex_      = 0;
  GLint  uniform_proj_     = 0;
  GLint  uniform_offset_   = 0;
  GLint  uniform_rot_      = 0;
  GLint  uniform_scale_    = 0;
  GLint  uniform_col_      = 0;

}; // shader_text


} // namespace plate
