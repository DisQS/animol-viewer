#pragma once

#include "../shader.hpp"
#include "../../../gpu.hpp"
#include "../../buffer.hpp"
#include "../../../../system/common/projection.hpp"
#include "../../../../system/common/alpha.hpp"
#include "../../texture.hpp"

#include "gl.vert.h"
#include "gl.frag.h"
#include "gl2.vert.h"
#include "gl2.frag.h"

namespace plate {


class shader_text_msdf {

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
    std::array<float, 2> tex_coord; // into a supplied font texture
  };


  shader_text_msdf(int v)
  {
    std::string_view vertex_source;
    std::string_view fragment_source;

    if (v == 1)
    {
      vertex_source   = std::string_view(reinterpret_cast<const char*>(text_msdf_gl_vert), text_msdf_gl_vert_len);
      fragment_source = std::string_view(reinterpret_cast<const char*>(text_msdf_gl_frag), text_msdf_gl_frag_len);
    }
    else
    {
      vertex_source   = std::string_view(reinterpret_cast<const char*>(text_msdf_gl2_vert), text_msdf_gl2_vert_len);
      fragment_source = std::string_view(reinterpret_cast<const char*>(text_msdf_gl2_frag), text_msdf_gl2_frag_len);
    }

    if (auto r = build_program(vertex_source, fragment_source); !r)
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
    uniform_ts_       = glGetUniformLocation  (program_, "ts");

    uniform_proj_    = glGetUniformLocation(program_, "proj");
    uniform_offset_  = glGetUniformLocation(program_, "offset");
    uniform_rot_     = glGetUniformLocation(program_, "rot");
    uniform_scale_   = glGetUniformLocation(program_, "scale");
  };

  shader_text_msdf(bool compile, int v) noexcept
  {
    if (compile)
    {
      std::string_view vertex_source;
      std::string_view fragment_source;
    
      if (v == 1)
      {
        vertex_source   = std::string_view(reinterpret_cast<const char*>(text_msdf_gl_vert),
                                                                          text_msdf_gl_vert_len); 
        fragment_source = std::string_view(reinterpret_cast<const char*>(text_msdf_gl_frag),
                                                                          text_msdf_gl_frag_len);
      }
      else
      {
        vertex_source   = std::string_view(reinterpret_cast<const char*>(text_msdf_gl2_vert),
                                                                          text_msdf_gl2_vert_len);
        fragment_source = std::string_view(reinterpret_cast<const char*>(text_msdf_gl2_frag),
                                                                          text_msdf_gl2_frag_len);
      }

      std::tie(vertex_shader_, fragment_shader_) = plate::compile(vertex_source, fragment_source);
    }
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
    uniform_col_      = glGetUniformLocation  (program_, "col");
    uniform_ts_       = glGetUniformLocation  (program_, "ts");
    
    uniform_proj_    = glGetUniformLocation(program_, "proj");
    uniform_offset_  = glGetUniformLocation(program_, "offset");
    uniform_rot_     = glGetUniformLocation(program_, "rot");
    uniform_scale_   = glGetUniformLocation(program_, "scale");

    return true;
  }


  template<class V>
  void draw(const projection& p, const alpha& a, buffer<ubo>& uniform_buffer, const buffer<V>& vbuf,
                  const texture& tex, const gpu::color& c, float tex_width, float tex_height)
  {
    auto u = uniform_buffer.map_staging();

    glUseProgram(program_);

    glUniformMatrix4fv(uniform_proj_, 1, GL_FALSE, p.matrix_.data());

    glUniform4f(uniform_offset_, u->offset.x, u->offset.y, 0.0, 0.0);
    glUniform4f(uniform_rot_,    u->rot.x, u->rot.y, u->rot.z, u->rot.a);
    glUniform2f(uniform_scale_,  u->scale.x, u->scale.y);

    glUniform4f(uniform_col_, c.r, c.g, c.b, c.a * a.alpha_);
    glUniform2f(uniform_ts_, tex_width, tex_height);
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
  }


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
  GLint  uniform_ts_       = 0;

}; // shader_text_msdf


} // namespace plate
