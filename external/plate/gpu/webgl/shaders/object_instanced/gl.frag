varying highp vec4 out_color;
varying highp vec4 out_normal;

void main()
{
  const highp vec4 light = vec4(0.0, 0.0, 1.0, 0.0);

  highp float diffuse = max(dot(out_normal, light), 0.4);

  gl_FragColor = vec4(out_color.xyz * diffuse, out_color.a);
}
