attribute vec3 position;
attribute vec3 normal;

attribute vec3  offset;
attribute float scale;
attribute vec4  color;

uniform mat4 proj;

uniform vec4 uoffset;
uniform vec4 urot;
uniform vec4 uscale;

varying highp vec4 out_color;
varying highp vec4 out_normal;

void main()
{
  // uniform

  highp float usin_x = sin(urot.x);
  highp float ucos_x = cos(urot.x);
  highp float usin_y = sin(urot.y);
  highp float ucos_y = cos(urot.y);
  highp float usin_z = sin(urot.z);
  highp float ucos_z = cos(urot.z);

  mat4 u_rot = mat4(ucos_y * ucos_z, ucos_y * usin_z, -usin_y,   0,
                    ucos_z * usin_x * usin_y - ucos_x * usin_z, ucos_x * ucos_z + usin_x * usin_y * usin_z, ucos_y * usin_x, 0,
                    ucos_x * ucos_z * usin_y + usin_x * usin_z, ucos_x * usin_y * usin_z - ucos_z * usin_x, ucos_x * ucos_y, 0,
                    0, 0, 0, 1 );

  mat4 u_scale = mat4(uscale.x, 0.0, 0.0, 0.0,
                      0.0, uscale.y, 0.0, 0.0,
                      0.0, 0.0, uscale.z, 0.0,
                      0.0, 0.0, 0.0, uscale.w);

  // attribute

  highp float s = scale / 100.0;

  mat4 a_scale = mat4(  s, 0.0, 0.0, 0.0,
                      0.0,   s, 0.0, 0.0,
                      0.0, 0.0,   s, 0.0,
                      0.0, 0.0, 0.0, 1.0);

  // apply transforms

  out_normal = u_rot * vec4(normal, 1.0);

  vec4 t = (a_scale * vec4(position, 1.0)) + vec4(offset, 0.0);

  gl_Position = proj * ((u_scale * (u_rot * t)) + uoffset);

  out_color = color;
}
