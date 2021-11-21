#pragma once

#include "../shader.hpp"
#include "../../../gpu.hpp"
#include "../../buffer.hpp"
#include "../../../../system/common/projection.hpp"
#include "../../texture.hpp"

#include "gl.vert.h"
#include "gl.frag.h"

namespace plate {


class shader_example_geom {

public:

  struct ubo
  {
    gpu::float_vec4 offset;
    gpu::float_vec2 tex_dim;
    float           radius;
  };


  shader_example_geom()
  {
    if (auto r = build_program({reinterpret_cast<const char*>(example_geom_gl_vert), example_geom_gl_vert_len},
                               {reinterpret_cast<const char*>(example_geom_gl_frag), example_geom_gl_frag_len}); !r)
      return;
    else
    { 
      vertex_shader_   = r->vertex_shader_id;
      fragment_shader_ = r->fragment_shader_id;
      program_         = r->program_id;
    }

    attrib_lookups_   = glGetAttribLocation(program_, "lookups");

    uniform_tex_     = glGetUniformLocation(program_, "tex");
    uniform_proj_    = glGetUniformLocation(program_, "proj");
    uniform_offset_  = glGetUniformLocation(program_, "offset");
    uniform_tex_dim_ = glGetUniformLocation(program_, "tex_dim");
    uniform_radius_  = glGetUniformLocation(program_, "radius");
    uniform_col_     = glGetUniformLocation(program_, "col");
  };

  shader_example_geom(bool compile) noexcept
  { 
    if (compile)
      std::tie(vertex_shader_, fragment_shader_) = plate::compile(
                              {reinterpret_cast<const char*>(example_geom_gl_vert), example_geom_gl_vert_len},
                              {reinterpret_cast<const char*>(example_geom_gl_frag), example_geom_gl_frag_len});
  };

  void link() noexcept
  { 
    program_ = plate::link(vertex_shader_, fragment_shader_);
  };

  bool check() noexcept
  { 
    if (!plate::check(vertex_shader_, fragment_shader_, program_))
      return false;

    attrib_lookups_   = glGetAttribLocation(program_, "lookups");

    uniform_tex_     = glGetUniformLocation(program_, "tex");
    uniform_proj_    = glGetUniformLocation(program_, "proj");
    uniform_offset_  = glGetUniformLocation(program_, "offset");
    uniform_tex_dim_ = glGetUniformLocation(program_, "tex_dim");
    uniform_radius_  = glGetUniformLocation(program_, "radius");
    uniform_col_     = glGetUniformLocation(program_, "col");

    return true;
  };


  void draw(const projection& p, buffer& uniform_buffer, const buffer& vertex_buffer,
                                            const std::uint32_t& tex_id, const gpu::color& c)
  {
    auto ptr = uniform_buffer.map_staging();

    ubo* u = new (ptr) ubo;

    glUseProgram(program_);

    glUniformMatrix4fv(uniform_proj_, 1, GL_FALSE, p.matrix_.data());

    glUniform4f(uniform_offset_,   u->offset.x, u->offset.y, 0.0, 0.0);
    glUniform2f(uniform_tex_dim_,  u->tex_dim.x, u->tex_dim.y);
    glUniform1f(uniform_radius_,   u->radius);

    glUniform4f(uniform_col_, c.r, c.g, c.b, c.a);
    glUniform1i(uniform_tex_, 0);

    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer.id_);

    glEnableVertexAttribArray(attrib_lookups_);
    glVertexAttribDivisor(attrib_lookups_, 0);
    glVertexAttribPointer(attrib_lookups_,  2, GL_FLOAT, GL_FALSE, vertex_buffer.stride_, nullptr);

    glBindTexture(GL_TEXTURE_2D, tex_id);

    glDrawArrays(GL_TRIANGLES, 0, vertex_buffer.count_);

    glDisableVertexAttribArray(attrib_lookups_);
  };


private:


  GLuint program_          = 0;

  GLuint vertex_shader_    = 0;
  GLuint fragment_shader_  = 0;

  GLint  attrib_lookups_   = 0;
  GLint  uniform_tex_      = 0;
  GLint  uniform_proj_     = 0;
  GLint  uniform_offset_   = 0;
  GLint  uniform_tex_dim_  = 0;
  GLint  uniform_radius_   = 0;
  GLint  uniform_col_      = 0;

}; // shader_example_geom


} // namespace plate
