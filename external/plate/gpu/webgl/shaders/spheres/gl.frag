#extension GL_EXT_frag_depth : enable

varying highp vec4 out_color;
varying highp vec2 out_pos;
varying highp float out_sz;
varying highp mat4 pers_rotate;

void main()
{
  highp float dot_out_pos = dot(out_pos, out_pos);

  if (dot_out_pos >= 1.0)
  {
    // to help debugging, uncomment the below and rm deiscard to see the quads
    //gl_FragDepthEXT = gl_FragCoord.z;
    //gl_FragColor = vec4(0.0, 1.0, 0.0, 1.0); 
    //return;
    discard;
  }

  const highp vec4 light = vec4(0.0, 0.0, 1.0, 0.0);

  highp vec4 normal = vec4(out_pos.x, out_pos.y, sqrt(1.0 - dot_out_pos), 1.0);

  normal = pers_rotate * normal;

  highp float diffuse = dot(normal, light) / (0.5 + dot(normal.xyz, normal.xyz) / 2.0);

	//diffuse = 1.0 - (0.8 * (1.0 - diffuse));
	diffuse = 1.0 - (0.7 * (1.0 - diffuse));

  gl_FragColor = vec4(out_color.xyz * diffuse, out_color.a);

#ifdef GL_EXT_frag_depth
  gl_FragDepthEXT = gl_FragCoord.z + (out_sz * normal.z);
#endif
}
