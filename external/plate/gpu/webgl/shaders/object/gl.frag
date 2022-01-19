varying highp vec4 out_color;
varying highp vec4 out_normal;

uniform highp vec4 bg_color;

void main()
{
  const highp vec4 light = vec4(0.0, 0.0, 1.0, 0.0);

  //highp float diffuse = max(dot(out_normal, light), 0.4);

  highp float diffuse = max(0.3, dot(out_normal, light)/(0.5+dot(out_normal.xyz, out_normal.xyz)/2.0));
  diffuse = 1.0 - (0.8 * (1.0 - diffuse));

  highp vec4 color = vec4(out_color.xyz * diffuse, out_color.a);

  gl_FragColor = vec4(bg_color.xyz - (bg_color.xyz - color.xyz) * bg_color.a, color.a);
}
