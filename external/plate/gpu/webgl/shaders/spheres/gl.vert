attribute vec3 v_position;

attribute vec3  i_offset;
attribute float i_scale;
attribute vec4  i_color;

uniform mat4 u_proj;

uniform vec4 u_offset;
uniform vec4 u_rot;
uniform vec4 u_scale;

varying highp vec4 out_color;
varying highp vec2 out_pos;
varying highp float out_sz;
varying highp mat4 pers_rotate;

//varying highp vec4 out_normal;


void main()
{
  // uniform

  highp float u_sin_x = sin(u_rot.x);
  highp float u_cos_x = cos(u_rot.x);
  highp float u_sin_y = sin(u_rot.y);
  highp float u_cos_y = cos(u_rot.y);
  highp float u_sin_z = sin(u_rot.z);
  highp float u_cos_z = cos(u_rot.z);

  mat4 u_m_rot = mat4(u_cos_y * u_cos_z, u_cos_y * u_sin_z, -u_sin_y,   0,
                    u_cos_z * u_sin_x * u_sin_y - u_cos_x * u_sin_z, u_cos_x * u_cos_z + u_sin_x * u_sin_y * u_sin_z, u_cos_y * u_sin_x, 0,
                    u_cos_x * u_cos_z * u_sin_y + u_sin_x * u_sin_z, u_cos_x * u_sin_y * u_sin_z - u_cos_z * u_sin_x, u_cos_x * u_cos_y, 0,
                    0, 0, 0, 1 );

  mat4 u_m_scale = mat4(u_scale.x, 0.0, 0.0, 0.0,
                        0.0, u_scale.y, 0.0, 0.0,
                        0.0, 0.0, u_scale.z, 0.0,
                        0.0, 0.0, 0.0, u_scale.w);

  // attribute

  highp float i_s = i_scale / 100.0;

  highp mat4 i_m_scale = mat4(  i_s, 0.0, 0.0, 0.0,
                                0.0, i_s, 0.0, 0.0,
                                0.0, 0.0, i_s, 0.0,
                                0.0, 0.0, 0.0, 1.0);

  // scale the vertex by the atom size

  highp vec4 t = u_m_scale * i_m_scale * vec4(v_position.xyz / 400.0, 0.0); // 404.5

  // shift the atom center point according to the view

  highp vec4 i = u_m_scale * (u_m_rot * vec4(i_offset, 1.0)) + u_offset;

  // rotate quad to face viewer

  highp float d = -i.z - 1.0 / u_proj[2][3];  // total z distance from viewer
  highp float x = i.x - 1.0 / u_proj[0][0];   // x distance
  highp float y = i.y - 1.0 / u_proj[1][1];   // y distance

  highp float n = sqrt(x*x+y*y);
  highp float angle = atan(n / d);

  highp float cosa = cos(angle);
  highp float cosb = 1.0 - cosa;
  highp float sina = sin(angle);

  highp vec2 axis = vec2(y/n, -x/n);

  pers_rotate = mat4(cosa+axis.x*axis.x*cosb, axis.x*axis.y*cosb,     -axis.y*sina, 0.0,
                     axis.x*axis.y*cosb,      cosa+axis.y*axis.y*cosb, axis.x*sina, 0.0,
                     axis.y*sina,            -axis.x*sina,             cosa,        0.0,
                     0.0,                     0.0,                     0.0,         1.0);

  t = pers_rotate * t;

  // output

  gl_Position = u_proj * (i + t);

  out_color = i_color;
  out_pos   = v_position.xy;

  // calculate distance from centre to atom edge in depth_buffer units

  highp vec4 ic = i + vec4(0.0, 0.0, (u_scale.z * i_s / 400.0), 0.0);

  highp vec4 proj_ic = u_proj * ic;

  out_sz = (proj_ic.z / proj_ic.w - gl_Position.z / gl_Position.w) / 2.0;
}
