#version 300 es

in vec4 position;
in vec2 coord;

uniform mat4 proj;
uniform vec4 offset;
uniform mediump vec2 scale;

out highp vec2 out_coord;

void main()
{
  mat4 a_scale = mat4(scale.x, 0.0, 0.0, 0.0, 0.0, scale.y, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0);
  gl_Position = (a_scale * position) + offset;
  gl_Position = proj * gl_Position;

  out_coord = coord;
}
