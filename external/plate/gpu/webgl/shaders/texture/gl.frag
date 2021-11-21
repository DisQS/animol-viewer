varying highp vec4 out_tex_coord;

uniform sampler2D tex;

void main()
{
  gl_FragColor = texture2DProj(tex, out_tex_coord.xyw);
}
