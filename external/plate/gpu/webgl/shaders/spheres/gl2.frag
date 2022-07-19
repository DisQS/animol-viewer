#version 300 es

in highp vec4 out_color;
in highp vec2 out_pos;
in highp float out_sz;
in highp mat4 pers_rotate;

out highp vec4 frag_color;

void main()
{
  highp float dot_out_pos = dot(out_pos, out_pos);

  if (dot_out_pos >= 1.0)
  {
    discard;
  }

  const highp vec4 light = vec4(0.0, 0.0, 1.0, 0.0);

  highp vec4 normal = vec4(out_pos.x, out_pos.y, sqrt(1.0 - dot_out_pos), 1.0);

  normal = pers_rotate * normal;

  highp float diffuse = dot(normal, light) / (0.5 + dot(normal.xyz, normal.xyz) / 2.0);

	diffuse = 1.0 - (0.7 * (1.0 - diffuse));

  frag_color = vec4(out_color.xyz * diffuse, out_color.a);

  gl_FragDepth = gl_FragCoord.z + (out_sz * normal.z);
}
