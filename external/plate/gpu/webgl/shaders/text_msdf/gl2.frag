#version 300 es

in highp vec4 out_tex_coord;

out highp vec4 frag_color;

uniform highp vec4 col;
uniform highp vec2 ts;

uniform sampler2D tex;

void main()
{
  highp vec3 msd = texture(tex, out_tex_coord.xy).rgb;
  highp float sd = max(min(msd.r, msd.g), min(max(msd.r, msd.g), msd.b));

  highp vec2 unitRange = 2.0 / ts;
  highp vec2 screenTexSize = vec2(0.001, 0.001);

  screenTexSize = vec2(1.0) / fwidth(out_tex_coord.xy);

  highp float screenPxDistance = max(0.5 * dot(unitRange, screenTexSize), 1.0) * (sd - 0.5);

  highp float opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);

  frag_color = col;
  frag_color.a = frag_color.a * opacity;
}
