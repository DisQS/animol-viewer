#pragma once

#include "../shader.hpp"
#include "../../../gpu.hpp"
#include "../../buffer.hpp"
#include "../../../../system/common/projection.hpp"
#include "../../texture.hpp"

#include "gl.vert.h"
#include "gl.frag.h"

namespace plate {


class shader_example_geom_instanced {

public:

  struct ubo
  {
    gpu::float_vec4 offset;
    gpu::float_vec2 tex_dim;
    float           radius;
  };


  shader_example_geom_instanced()
  {
    if (auto r = build_program({reinterpret_cast<const char*>(example_geom_instanced_gl_vert), example_geom_instanced_gl_vert_len},
                               {reinterpret_cast<const char*>(example_geom_instanced_gl_frag), example_geom_instanced_gl_frag_len}); !r)
      return;
    else
    { 
      vertex_shader_   = r->vertex_shader_id;
      fragment_shader_ = r->fragment_shader_id;
      program_         = r->program_id;
    }

    attrib_instance_id_ = glGetAttribLocation(program_, "instance_id");
    attrib_vertex_id_   = glGetAttribLocation(program_, "vertex_id");

    uniform_tex_     = glGetUniformLocation(program_, "tex");
    uniform_proj_    = glGetUniformLocation(program_, "proj");
    uniform_offset_  = glGetUniformLocation(program_, "offset");
    uniform_tex_dim_ = glGetUniformLocation(program_, "tex_dim");
    uniform_radius_  = glGetUniformLocation(program_, "radius");
    uniform_col_     = glGetUniformLocation(program_, "col");
  };

  shader_example_geom_instanced(bool compile) noexcept
  { 
    if (compile)
      std::tie(vertex_shader_, fragment_shader_) = plate::compile(
          {reinterpret_cast<const char*>(example_geom_instanced_gl_vert), example_geom_instanced_gl_vert_len},
          {reinterpret_cast<const char*>(example_geom_instanced_gl_frag), example_geom_instanced_gl_frag_len});
  };

  void link() noexcept
  { 
    program_ = plate::link(vertex_shader_, fragment_shader_);
  };

  bool check() noexcept
  { 
    if (!plate::check(vertex_shader_, fragment_shader_, program_))
      return false;

    attrib_instance_id_ = glGetAttribLocation(program_, "instance_id");
    attrib_vertex_id_   = glGetAttribLocation(program_, "vertex_id");

    uniform_tex_     = glGetUniformLocation(program_, "tex");
    uniform_proj_    = glGetUniformLocation(program_, "proj");
    uniform_offset_  = glGetUniformLocation(program_, "offset");
    uniform_tex_dim_ = glGetUniformLocation(program_, "tex_dim");
    uniform_radius_  = glGetUniformLocation(program_, "radius");
    uniform_col_     = glGetUniformLocation(program_, "col");

    return true;
  };


  void draw(const projection& p, buffer& uniform_buffer, const buffer& vertex_buffer,
                      const buffer& instance_buffer, const std::uint32_t& tex_id, const gpu::color& c)
  {
    auto ptr = uniform_buffer.map_staging();

    ubo* u = new (ptr) ubo;

    glUseProgram(program_);

    // uniforms

    glUniformMatrix4fv(uniform_proj_, 1, GL_FALSE, p.matrix_.data());

    glUniform4f(uniform_offset_,   u->offset.x, u->offset.y, 0.0, 0.0);
    glUniform2f(uniform_tex_dim_,  u->tex_dim.x, u->tex_dim.y);
    glUniform1f(uniform_radius_,   u->radius);

    glUniform4f(uniform_col_, c.r, c.g, c.b, c.a);
    glUniform1i(uniform_tex_, 0);


    // object/vertex buffer

    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer.id_);

    glEnableVertexAttribArray(attrib_vertex_id_);
    glVertexAttribPointer(attrib_vertex_id_,  1, GL_FLOAT, GL_FALSE, vertex_buffer.stride_, nullptr);


    // instance buffer

    glBindBuffer(GL_ARRAY_BUFFER, instance_buffer.id_);

    glEnableVertexAttribArray(attrib_instance_id_);
    glVertexAttribDivisor(attrib_instance_id_, 1);
    glVertexAttribPointer(attrib_instance_id_, 1, GL_FLOAT, GL_FALSE, instance_buffer.stride_, (const GLvoid *)0);


    // draw

    glBindTexture(GL_TEXTURE_2D, tex_id);

    glDrawArraysInstanced(GL_TRIANGLES, 0, vertex_buffer.count_, instance_buffer.count_);

    // unhook

    glDisableVertexAttribArray(attrib_vertex_id_);
    glDisableVertexAttribArray(attrib_instance_id_);
  };


  void draw(const projection& p, buffer& uniform_buffer, const buffer& vertex_buffer,
              const buffer& instance_buffer, std::uint32_t* vertex_array_object,
              const std::uint32_t& tex_id, const gpu::color& c)
  { 
    if (*vertex_array_object == 0) // create the Vertex Array
    {
      glGenVertexArrays(1, vertex_array_object);

      glBindVertexArray(*vertex_array_object);

      // object/vertex buffer

      glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer.id_);

      glEnableVertexAttribArray(attrib_vertex_id_);
      glVertexAttribPointer(attrib_vertex_id_,  1, GL_FLOAT, GL_FALSE, vertex_buffer.stride_, nullptr);

      // instance buffer

      glBindBuffer(GL_ARRAY_BUFFER, instance_buffer.id_);

      glEnableVertexAttribArray(attrib_instance_id_);
      glVertexAttribDivisor(attrib_instance_id_, 1);
      glVertexAttribPointer(attrib_instance_id_, 1, GL_FLOAT, GL_FALSE, instance_buffer.stride_, (const GLvoid   *)0);

      glBindVertexArray(0);

      glDisableVertexAttribArray(attrib_vertex_id_);
      glDisableVertexAttribArray(attrib_instance_id_);
    }

    // draw
     
    auto ptr = uniform_buffer.map_staging();

    ubo* u = new (ptr) ubo;
      
    glUseProgram(program_);
  
    glUniformMatrix4fv(uniform_proj_, 1, GL_FALSE, p.matrix_.data());
  
    glUniform4f(uniform_offset_,   u->offset.x, u->offset.y, 0.0, 0.0);
    glUniform2f(uniform_tex_dim_,  u->tex_dim.x, u->tex_dim.y);
    glUniform1f(uniform_radius_,   u->radius);
  
    glUniform4f(uniform_col_, c.r, c.g, c.b, c.a);
    glUniform1i(uniform_tex_, 0);

    glBindVertexArray(*vertex_array_object);

    glBindTexture(GL_TEXTURE_2D, tex_id);

    glDrawArraysInstanced(GL_TRIANGLES, 0, vertex_buffer.count_, instance_buffer.count_);

    glBindVertexArray(0);
  };


private:


  GLuint program_            = 0;

  GLuint vertex_shader_      = 0;
  GLuint fragment_shader_    = 0;

  GLint  attrib_instance_id_ = 0;
  GLint  attrib_vertex_id_   = 0;
  GLint  uniform_tex_        = 0;
  GLint  uniform_proj_       = 0;
  GLint  uniform_offset_     = 0;
  GLint  uniform_tex_dim_    = 0;
  GLint  uniform_radius_     = 0;
  GLint  uniform_col_        = 0;

}; // shader_example_geom_instanced


} // namespace plate
