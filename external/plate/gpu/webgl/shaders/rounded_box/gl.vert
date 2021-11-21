attribute vec4 position;
attribute vec2 coord;

uniform mat4 proj;
uniform vec4 offset;
uniform mediump vec2 scale;

varying highp vec2 pos;

void main()
{
  mat4 a_scale = mat4(scale.x, 0.0, 0.0, 0.0, 0.0, scale.y, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0);
  gl_Position = (a_scale * position) + offset;
  gl_Position = proj * gl_Position;

  pos = coord;
}
