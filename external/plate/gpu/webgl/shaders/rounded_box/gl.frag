#extension GL_OES_standard_derivatives : enable

uniform mediump vec4 col;
uniform highp vec2 dim;
uniform highp float radius;

varying highp vec2 pos;

void curve(in highp vec2 p, in highp vec2 c, out highp float alpha)
{
  highp float dist = distance(p, c);

  alpha = smoothstep(radius, radius - 1.0, dist);
}


void main()
{
  highp float alpha;

  highp vec2 spos = pos * dim;

  if ((spos.x <= radius) && (spos.y <= radius)) // bottom left
  {
    curve(spos, vec2(radius, radius), alpha);
  }
  else
  {
    if ((spos.x <= radius) && (spos.y >= dim.y - radius)) // top left
    {
      curve(spos, vec2(radius, dim.y - radius), alpha);
    }
    else
    {
      if ((spos.x >= dim.x - radius) && (spos.y <= radius)) // bottom right
      {
        curve(spos, vec2(dim.x - radius, radius), alpha);
      }
      else
      {
        if ((spos.x >= dim.x - radius) && (spos.y >= dim.y - radius)) // top right
        {
          curve(spos, vec2(dim.x - radius, dim.y - radius), alpha);
        }
        else
          alpha = 1.0;
      }
    }
  }

  if (alpha == 0.0)
    discard;

  gl_FragColor = col;
  gl_FragColor.a = gl_FragColor.a * alpha;
}
