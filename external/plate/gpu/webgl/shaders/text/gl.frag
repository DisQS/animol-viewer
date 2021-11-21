varying highp vec4 out_tex_coord;

uniform highp vec3 col;

uniform sampler2D tex;

void main()
{
  gl_FragColor = vec4(col, texture2DProj(tex, out_tex_coord.xyw).a);
}
