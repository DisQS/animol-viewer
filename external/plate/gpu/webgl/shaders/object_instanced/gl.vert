attribute vec4 position;
attribute vec4 normal;
attribute vec4 color;
attribute vec4 offset;
attribute vec4 rot;
attribute vec4 scale;

uniform mat4 proj;

varying highp vec4 out_color;
varying highp vec4 out_normal;

void main()
{
  highp float sin_x = sin(rot.x);
  highp float cos_x = cos(rot.x);
  highp float sin_y = sin(rot.y);
  highp float cos_y = cos(rot.y);
  highp float sin_z = sin(rot.z);
  highp float cos_z = cos(rot.z);

  mat4 a_rot = mat4(cos_y * cos_z, cos_y * sin_z, -sin_y,   0,
                    cos_z * sin_x * sin_y - cos_x * sin_z, cos_x * cos_z + sin_x * sin_y * sin_z, cos_y * sin_x, 0,
                    cos_x * cos_z * sin_y + sin_x * sin_z, cos_x * sin_y * sin_z - cos_z * sin_x, cos_x * cos_y, 0,
                    0, 0, 0, 1 );

  mat4 a_scale = mat4(scale.x, 0.0, 0.0, 0.0,
                      0.0, scale.y, 0.0, 0.0,
                      0.0, 0.0, scale.z, 0.0,
                      0.0, 0.0, 0.0, scale.w);

  out_normal = a_rot * normal;

  vec4 t = a_rot * position;

  gl_Position = proj * ((a_scale * t) + offset);

  out_color = color;
}
