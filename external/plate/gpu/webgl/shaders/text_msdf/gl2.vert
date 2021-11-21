#version 300 es

in vec4 position;
in vec2 tex_coord;

uniform  mat4 proj;
uniform  vec4 offset;
uniform  vec4 rot;
uniform  mediump vec2 scale;

out highp vec4 out_tex_coord;

void main()
{
  mat4 a_scale = mat4(scale.x, 0.0, 0.0, 0.0, 0.0, scale.y, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0);
  mat4 a_rot   = mat4(cos(rot.z), sin(rot.z), 0, 0,  -sin(rot.z), cos(rot.z), 0, 0,  0, 0, 1, 0,  0, 0, 0, 1);

  vec4 t = a_rot * position;

  gl_Position = proj * ((a_scale * t) + offset);

  out_tex_coord = vec4(tex_coord.x, tex_coord.y, 0.0, 1.0);
}
