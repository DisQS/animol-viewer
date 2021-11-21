uniform highp vec4 inputs;

uniform sampler2D tex;

void main()
{
  highp float cx = 1.0 / (inputs.x * 2.0);
  highp float cy = 1.0 / (inputs.y * 2.0);

  highp float m = texture2D(tex, vec2(gl_FragCoord.x / inputs.x, gl_FragCoord.y / inputs.y)).r;

  highp float calc = 0.0;

  const highp float MAX_ITERATIONS = 100000.0;

  highp float ccx = 2.0 * cx;
  highp float ccy = 2.0 * cy;

  highp vec2 pos = vec2(cx, cy);

  for (highp float y = 0.0; y < MAX_ITERATIONS; ++y)
  {
    if (y >= inputs.y) break;

    pos.x = cx;

    for (highp float x = 0.0; x < MAX_ITERATIONS; ++x)
    {
      if (x >= inputs.x) break;

      calc += texture2D(tex, pos).r * m;

      pos.x += ccx;
    }

    pos.y += ccy;
  }

  gl_FragColor = vec4(calc, 0, 0, 0);
}
