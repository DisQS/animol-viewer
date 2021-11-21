#version 300 es

uniform mediump vec4 col;

in highp vec2 w;

out highp vec4 frag_color;

void main()
{
  float dist = sqrt(dot(val, val));

  if (dist >= 1.0)
    discard;

  float delta = fwidth(dist);

  float alpha = smoothstep(1.0, 1.0 - delta, dist);

  frag_color = col;
  frag_color.a = gl_FragColor.a * alpha;
}
