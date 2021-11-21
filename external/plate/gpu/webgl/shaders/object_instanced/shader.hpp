#pragma once

#include "../shader.hpp"
#include "../../../gpu.hpp"
#include "../../buffer.hpp"
#include "../../../../system/common/projection.hpp"
#include "../../texture.hpp"

#include "gl.vert.h"
#include "gl.frag.h"

namespace plate {


class shader_object_instanced {

public:

  shader_object_instanced()
  {
    if (auto r = build_program({reinterpret_cast<const char*>(object_instanced_gl_vert), object_instanced_gl_vert_len},
                               {reinterpret_cast<const char*>(object_instanced_gl_frag), object_instanced_gl_frag_len}); !r)
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

    attrib_offset_   = glGetAttribLocation   (program_, "offset");
    attrib_rot_      = glGetAttribLocation   (program_, "rot");
    attrib_scale_    = glGetAttribLocation   (program_, "scale");

    uniform_proj_    = glGetUniformLocation  (program_, "proj");
  };

  shader_object_instanced(bool compile) noexcept
  { 
    if (compile)
      std::tie(vertex_shader_, fragment_shader_) = plate::compile(
                      {reinterpret_cast<const char*>(object_instanced_gl_vert), object_instanced_gl_vert_len},
                      {reinterpret_cast<const char*>(object_instanced_gl_frag), object_instanced_gl_frag_len});
  };

  void link() noexcept
  { 
    program_ = plate::link(vertex_shader_, fragment_shader_);
  };

  bool check() noexcept
  { 
    if (!plate::check(vertex_shader_, fragment_shader_, program_))
      return false;

    attrib_position_ = glGetAttribLocation   (program_, "position");
    attrib_normal_   = glGetAttribLocation   (program_, "normal");
    attrib_color_    = glGetAttribLocation   (program_, "color");

    attrib_offset_   = glGetAttribLocation   (program_, "offset");
    attrib_rot_      = glGetAttribLocation   (program_, "rot");
    attrib_scale_    = glGetAttribLocation   (program_, "scale");

    uniform_proj_    = glGetUniformLocation  (program_, "proj");

    return true;
  };


  void draw(const projection& p, const buffer& vertex_buffer, const buffer& instance_buffer)
  {
    glUseProgram(program_);

    glUniformMatrix4fv(uniform_proj_, 1, GL_FALSE, p.matrix_.data());

    // object/vertex buffer

    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer.id_);

    glEnableVertexAttribArray(attrib_position_);
    glVertexAttribPointer(attrib_position_,  4, GL_FLOAT, GL_FALSE, vertex_buffer.stride_, nullptr);

    glEnableVertexAttribArray(attrib_normal_);
    glVertexAttribPointer(attrib_normal_, 4, GL_FLOAT, GL_FALSE, vertex_buffer.stride_,
                                                                      (const GLvoid *)(0 + (4 * sizeof(GLfloat))));

    glEnableVertexAttribArray(attrib_color_);
    glVertexAttribPointer(attrib_color_, 4, GL_FLOAT, GL_FALSE, vertex_buffer.stride_,
                                                                      (const GLvoid *)(0 + (8 * sizeof(GLfloat))));

    // instance buffer

    glBindBuffer(GL_ARRAY_BUFFER, instance_buffer.id_);

    glEnableVertexAttribArray(attrib_offset_);
    glVertexAttribDivisor(attrib_offset_, 1);
    glVertexAttribPointer(attrib_offset_, 4, GL_FLOAT, GL_FALSE, instance_buffer.stride_, (const GLvoid *)0);

    glEnableVertexAttribArray(attrib_rot_);
    glVertexAttribDivisor(attrib_rot_, 1);
    glVertexAttribPointer(attrib_rot_, 4, GL_FLOAT, GL_FALSE, instance_buffer.stride_,
                                                                    (const GLvoid *)(0 + 4 * sizeof(GLfloat)));

    glEnableVertexAttribArray(attrib_scale_);
    glVertexAttribDivisor(attrib_scale_, 1);
    glVertexAttribPointer(attrib_scale_, 4, GL_FLOAT, GL_FALSE, instance_buffer.stride_,
                                                                    (const GLvoid *)(0 + (8 * sizeof(GLfloat))));

    glDrawArraysInstanced(GL_TRIANGLES, 0, vertex_buffer.count_, instance_buffer.count_);

    glDisableVertexAttribArray(attrib_position_);
    glDisableVertexAttribArray(attrib_normal_);
    glDisableVertexAttribArray(attrib_color_);

    glDisableVertexAttribArray(attrib_offset_);
    glDisableVertexAttribArray(attrib_rot_);
    glDisableVertexAttribArray(attrib_scale_);

    //if (auto err = glGetError(); err != GL_NO_ERROR) { LOG_ERROR("got error inst YYY: %u", err); }
  };


  // store the buffer configs into a vertex_array_object and use that for drawing

  void draw(const projection& p, const buffer& vertex_buffer, const buffer& instance_buffer,
                                                                std::uint32_t* vertex_array_object)
  {
    if (*vertex_array_object == 0) // create the Vertex Array
    {
      glGenVertexArrays(1, vertex_array_object);

      glBindVertexArray(*vertex_array_object);

      // object/vertex buffer

      glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer.id_);

      glEnableVertexAttribArray(attrib_position_);
      glVertexAttribPointer(attrib_position_,  4, GL_FLOAT, GL_FALSE, vertex_buffer.stride_, nullptr);

      glEnableVertexAttribArray(attrib_normal_);
      glVertexAttribPointer(attrib_normal_, 4, GL_FLOAT, GL_FALSE, vertex_buffer.stride_,
                                                                        (const GLvoid *)(0 + (4 * sizeof(GLfloat))));

      glEnableVertexAttribArray(attrib_color_);
      glVertexAttribPointer(attrib_color_, 4, GL_FLOAT, GL_FALSE, vertex_buffer.stride_,
                                                                        (const GLvoid *)(0 + (8 * sizeof(GLfloat))));

      // instance buffer

      glBindBuffer(GL_ARRAY_BUFFER, instance_buffer.id_);

      glEnableVertexAttribArray(attrib_offset_);
      glVertexAttribDivisor(attrib_offset_, 1);
      glVertexAttribPointer(attrib_offset_, 4, GL_FLOAT, GL_FALSE, instance_buffer.stride_, (const GLvoid *)0);

      glEnableVertexAttribArray(attrib_rot_);
      glVertexAttribDivisor(attrib_rot_, 1);
      glVertexAttribPointer(attrib_rot_, 4, GL_FLOAT, GL_FALSE, instance_buffer.stride_,
                                                                      (const GLvoid *)(0 + 4 * sizeof(GLfloat)));

      glEnableVertexAttribArray(attrib_scale_);
      glVertexAttribDivisor(attrib_scale_, 1);
      glVertexAttribPointer(attrib_scale_, 4, GL_FLOAT, GL_FALSE, instance_buffer.stride_,
                                                                      (const GLvoid *)(0 + (8 * sizeof(GLfloat))));

      glBindVertexArray(0);

      glDisableVertexAttribArray(attrib_position_);
      glDisableVertexAttribArray(attrib_normal_);
      glDisableVertexAttribArray(attrib_color_);

      glDisableVertexAttribArray(attrib_offset_);
      glDisableVertexAttribArray(attrib_rot_);
      glDisableVertexAttribArray(attrib_scale_);
    }

    glUseProgram(program_);

    glUniformMatrix4fv(uniform_proj_, 1, GL_FALSE, p.matrix_.data());

    glBindVertexArray(*vertex_array_object);

    glDrawArraysInstanced(GL_TRIANGLES, 0, vertex_buffer.count_, instance_buffer.count_);

    glBindVertexArray(0);
  };



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

  GLint  uniform_proj_   = 0;

}; // shader_object_instanced


} // namespace plate
