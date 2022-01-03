varying highp vec4 out_color;
varying highp vec4 out_normal;

uniform highp vec4 bg_color;

void main()
{
  const highp vec4 light = vec4(0.0, 0.0, 1.0, 0.0);

  highp float diffuse = max(dot(out_normal, light), 0.4);

  highp vec4 color = vec4(out_color.xyz * diffuse, out_color.a);

  gl_FragColor = vec4(bg_color.xyz - (bg_color.xyz - color.xyz) * bg_color.a, color.a);
}
