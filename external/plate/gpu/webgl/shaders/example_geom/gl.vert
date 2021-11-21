attribute vec2 lookups;
 
uniform mat4  proj;
uniform vec4  offset;
uniform highp vec2  tex_dim;
uniform float radius;

uniform sampler2D tex;

// tan(30') = 0.57735026919  cos(30') = 0.86602540378

void main()
{

  // convert the index into texture coordinates


  highp float y = floor(lookups.x / tex_dim.x);

  highp float cx = ((lookups.x - (y * tex_dim.x)) + 0.5) / tex_dim.x;

  highp float cy = (y + 0.5) / tex_dim.y;

  highp vec2 point = texture2D(tex, vec2(cx, cy)).xy;


  // build an equilateral triangle


  if (lookups.y == 0.0)
    gl_Position = vec4(point.x, point.y + radius, 0, 1);
  else
  {
    if (lookups.y == 1.0)
      gl_Position = vec4(point.x + (0.86602540378 * radius), point.y - (0.57735026919 * radius), 0, 1);
    else
      gl_Position = vec4(point.x - (0.86602540378 * radius), point.y - (0.57735026919 * radius), 0, 1);
  }

  gl_Position = proj * (gl_Position + offset);
}
