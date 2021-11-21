#version 300 es

in highp vec4 out_tex_coord;

out highp vec4 frag_color;

uniform highp vec4 col;
uniform highp vec2 tx;

uniform sampler2D tex;

highp float median(float r, float g, float b)
{
  return max(min(r, g), min(max(r, g), b));
}

void main()
{
  highp vec2 msdf_unit = 2.0 / vec2(textureSize(tex, 0));
  highp vec3 s = texture(tex, out_tex_coord.xy).rgb;
  highp float sig_dist = median(s.r, s.g, s.b) - 0.5;
  sig_dist *= dot(msdf_unit, 0.5 / fwidth(out_tex_coord.xy));
  highp float opacity = clamp(sig_dist + 0.5, 0.0, 1.0);
  frag_color = col;
  frag_color.a = frag_color.a * opacity;

}

