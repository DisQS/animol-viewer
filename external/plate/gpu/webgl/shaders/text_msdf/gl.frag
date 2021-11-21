#extension GL_OES_standard_derivatives : enable

varying highp vec4 out_tex_coord;

uniform highp vec4 col;
uniform highp vec2 ts;

uniform sampler2D tex;
/*
void main()
{
  highp vec2 msdf_unit = 2.0 / ts;
  highp vec3 s = texture2D(tex, out_tex_coord.xy).rgb;
  highp float sig_dist = max(min(s.r, s.g), min(max(s.r, s.g), s.b)) - 0.5;
#ifdef GL_OES_standard_derivatives
  sig_dist *= dot(msdf_unit, 0.5 / fwidth(out_tex_coord.xy));
#endif
  highp float opacity = clamp(sig_dist + 0.5, 0.0, 1.0);
  gl_FragColor = col;
  gl_FragColor.a = gl_FragColor.a * opacity;
}
*/

void main()
{
  highp vec3 msd = texture2D(tex, out_tex_coord.xy).rgb;
  highp float sd = max(min(msd.r, msd.g), min(max(msd.r, msd.g), msd.b));

  highp vec2 unitRange = 2.0 / ts;
  highp vec2 screenTexSize = vec2(0.001, 0.001);
#ifdef GL_OES_standard_derivatives
  screenTexSize = vec2(1.0) / fwidth(out_tex_coord.xy);
#endif
  highp float screenPxDistance = max(0.5 * dot(unitRange, screenTexSize), 1.0) * (sd - 0.5);

  highp float opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);

  gl_FragColor = col;
  gl_FragColor.a = gl_FragColor.a * opacity;
}

