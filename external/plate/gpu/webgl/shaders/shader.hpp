#pragma once

#include <cstdio>

#include <optional>

#include <GLES3/gl3.h>

#include "../../../system/log.hpp"

namespace plate {


struct gpu_program
{
  std::uint32_t vertex_shader_id{0};
  std::uint32_t fragment_shader_id{0};
  std::uint32_t program_id{0};
};


std::optional<gpu_program> build_program(std::string_view vertex_source, std::string_view fragment_source)
{
  gpu_program g;

  // vertex shader

  g.vertex_shader_id   = glCreateShader(GL_VERTEX_SHADER);

  const char* src = vertex_source.data();
  int         sz  = vertex_source.size();

  glShaderSource(g.vertex_shader_id, 1, &src, &sz);
  glCompileShader(g.vertex_shader_id);


  // fragment shader

  g.fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);

  src = fragment_source.data();
  sz  = fragment_source.size();

  glShaderSource(g.fragment_shader_id, 1, &src, &sz);
  glCompileShader(g.fragment_shader_id);


  // program

  g.program_id = glCreateProgram();
  glAttachShader(g.program_id, g.vertex_shader_id);
  glAttachShader(g.program_id, g.fragment_shader_id);
  glLinkProgram(g.program_id);


  int success = 0;
  if (glGetProgramiv(g.program_id, GL_LINK_STATUS, &success); success == GL_FALSE)
  {
    int max_len = 0;
    int len = 0;

    {
      glGetShaderiv(g.vertex_shader_id, GL_INFO_LOG_LENGTH, &max_len);

      char log[max_len];
      glGetShaderInfoLog(g.vertex_shader_id, max_len, &len, log);

      if (len > 0)
        log_error(FMT_COMPILE("compile vertex log: {}"), log);
    }
    {
      glGetShaderiv(g.fragment_shader_id, GL_INFO_LOG_LENGTH, &max_len);

      char log[max_len];
      glGetShaderInfoLog(g.fragment_shader_id, max_len, &len, log);

      if (len > 0)
        log_error(FMT_COMPILE("compile fragment log: {}"), log);
    }
    {
      glGetProgramiv(g.program_id, GL_INFO_LOG_LENGTH, &max_len);

      char log[max_len];
      glGetProgramInfoLog(g.program_id, max_len, &len, log);

      if (len > 0)
        log_error(FMT_COMPILE("link log: {}"), log);
    }

    if (g.program_id)         glDeleteProgram(g.program_id);
    if (g.fragment_shader_id) glDeleteShader(g.fragment_shader_id);
    if (g.vertex_shader_id)   glDeleteShader(g.vertex_shader_id);

    return std::nullopt;
  }

  return g;
}


std::pair<std::uint32_t, std::uint32_t> compile(std::string_view vertex_source, std::string_view fragment_source) noexcept
{
  std::pair<std::uint32_t, std::uint32_t> ids;

  // vertex shader
  
  ids.first  = glCreateShader(GL_VERTEX_SHADER);
  
  const char* src = vertex_source.data();
  int         sz  = vertex_source.size();
  
  glShaderSource(ids.first, 1, &src, &sz);
  glCompileShader(ids.first);
  
  // fragment shader

  ids.second = glCreateShader(GL_FRAGMENT_SHADER);
  
  src = fragment_source.data();
  sz  = fragment_source.size();
  
  glShaderSource(ids.second, 1, &src, &sz);
  glCompileShader(ids.second);

  return ids;
}


std::uint32_t link(std::uint32_t vertex_shader, std::uint32_t fragment_shader) noexcept
{
  std::uint32_t id = glCreateProgram();
  glAttachShader(id, vertex_shader);
  glAttachShader(id, fragment_shader);
  glLinkProgram(id);

  return id;
}


bool check(std::uint32_t vertex_shader, std::uint32_t fragment_shader, std::uint32_t program) noexcept
{
  int success = 0;

  if (glGetProgramiv(program, GL_LINK_STATUS, &success); success == GL_FALSE)
  {
    int max_len = 0;
    int len = 0;

    {
      glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &max_len);

      char log[max_len];
      glGetShaderInfoLog(vertex_shader, max_len, &len, log);

      if (len > 0)
        log_error(FMT_COMPILE("compile vertex log: {}"), log);
    }
    {
      glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &max_len);

      char log[max_len];
      glGetShaderInfoLog(fragment_shader, max_len, &len, log);

      if (len > 0)
        log_error(FMT_COMPILE("compile fragment log: {}"), log);
    }
    {
      glGetProgramiv(program, GL_INFO_LOG_LENGTH, &max_len);

      char log[max_len];
      glGetProgramInfoLog(program, max_len, &len, log);

      if (len > 0)
        log_error(FMT_COMPILE("link log: {}"), log);
    }

    if (program)         glDeleteProgram(program);
    if (fragment_shader) glDeleteShader(fragment_shader);
    if (vertex_shader)   glDeleteShader(vertex_shader);

    return false;
  }

  return true;
}


} // namespace plate

