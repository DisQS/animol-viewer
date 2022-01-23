varying highp vec4 out_tex_coord;

uniform sampler2D tex;
uniform highp float alpha;

void main()
{
  gl_FragColor = texture2DProj(tex, out_tex_coord.xyw);
  gl_FragColor.a = gl_FragColor.a * alpha;
}
