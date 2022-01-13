varying highp vec4 out_color;
varying highp vec4 out_normal;

void main()
{
  const highp vec4 light = vec4(0.0, 0.0, 1.0, 0.0);

  //stupid interpolation of normals
  //highp float diffuse = max(dot(out_normal, light), 0.0);

  //perfect interpolation of normals using square root
  //highp float diffuse = max(dot(out_normal/sqrt(out_normal.x*out_normal.x + out_normal.y*out_normal.y + out_normal.z*out_normal.z), light), 0.4);

  //approximation of perfect interpolation using first order taylor expansion of square root at 1.0
  highp float diffuse = max(dot(out_normal/(0.5+(out_normal.x*out_normal.x + out_normal.y*out_normal.y + out_normal.z*out_normal.z)/2.0), light), 0.0);


	if (diffuse <= 0.0)
	{
		discard;
	}

	diffuse = 1.0 - 0.8 * (1.0 - diffuse);

  gl_FragColor = vec4(out_color.xyz * diffuse, out_color.a);
}
