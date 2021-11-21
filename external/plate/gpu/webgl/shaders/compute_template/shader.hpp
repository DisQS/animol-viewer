#pragma once

#include "../shader.hpp"
#include "../../../gpu.hpp"
#include "../../buffer.hpp"
#include "../../texture.hpp"

#include "gl.vert.h"
#include "gl.frag.h"

namespace plate {


class shader_compute_template {

public:

  shader_compute_template()
  {
    if (auto r = build_program({reinterpret_cast<const char*>(compute_template_gl_vert), compute_template_gl_vert_len},
                               {reinterpret_cast<const char*>(compute_template_gl_frag), compute_template_gl_frag_len}); !r)
      return;
    else
    { 
      vertex_shader_   = r->vertex_shader_id;
      fragment_shader_ = r->fragment_shader_id;
      program_         = r->program_id;
    }

    attrib_position_  = glGetAttribLocation   (program_, "position");
    uniform_tex_      = glGetUniformLocation  (program_, "tex");
    uniform_inputs_   = glGetUniformLocation(program_, "inputs");
  };

  shader_compute_template(bool compile) noexcept
  {
    if (compile)
      std::tie(vertex_shader_, fragment_shader_) = plate::compile(
                      {reinterpret_cast<const char*>(compute_template_gl_vert), compute_template_gl_vert_len},
                      {reinterpret_cast<const char*>(compute_template_gl_frag), compute_template_gl_frag_len});
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
    uniform_tex_      = glGetUniformLocation  (program_, "tex");
    uniform_inputs_   = glGetUniformLocation(program_, "inputs");

    return true;
  };


  void draw(const buffer& vertex_buffer, const unsigned int& tex_id, gpu::float_vec4& i)
  {
    glUseProgram(program_);

    glUniform1i(uniform_tex_, 0);
    glUniform4f(uniform_inputs_, i.x, i.y, i.z, i.w);

    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer.id_);
    glEnableVertexAttribArray(attrib_position_);
    glVertexAttribPointer(attrib_position_,  4, GL_FLOAT, GL_FALSE, vertex_buffer.stride_, nullptr);

    glBindTexture(GL_TEXTURE_2D, tex_id);

//    const GLenum drawBuffers[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
//    glDrawBuffers(2, drawBuffers);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, vertex_buffer.count_);

    glDisableVertexAttribArray(attrib_position_);

    //if (auto err = glGetError(); err != GL_NO_ERROR)
    //{
    //  LOG_ERROR("got error YYY: %u", err);
    //}
  };


private:


  GLuint program_         = 0;

  GLuint vertex_shader_   = 0;
  GLuint fragment_shader_ = 0;

  GLint  attrib_position_  = 0;
  GLint  uniform_tex_      = 0;
  GLint  uniform_inputs_   = 0;

}; // shader_compute_template


} // namespace plate
