#extension GL_OES_standard_derivatives : enable

uniform mediump vec4 col;

varying highp vec2 pos;

void main()
{
  highp float dist = sqrt(dot(pos, pos));

  if (dist >= 1.0)
    discard;

  highp float delta = 0.001;

#ifdef GL_OES_standard_derivatives
  delta = fwidth(dist);
#endif

  highp float alpha = smoothstep(1.0, 1.0 - delta, dist);

  if (alpha == 0.0)
    discard;

  gl_FragColor = col;
  gl_FragColor.a = gl_FragColor.a * alpha;
}
