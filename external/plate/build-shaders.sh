#!/bin/bash

EMBED=('basic/gl.frag' 'basic/gl.vert' 'texture/gl.frag' 'texture/gl.vert' 'text/gl.frag' 'text/gl.vert' 'compute_template/gl.frag' 'compute_template/gl.vert' 'object_instanced/gl.frag' 'object_instanced/gl.vert' 'object/gl.frag' 'object/gl.vert' 'text_msdf/gl.frag' 'text_msdf/gl.vert' 'text_msdf/gl2.frag' 'text_msdf/gl2.vert' 'example_geom/gl.frag' 'example_geom/gl.vert' 'graph_points/gl.frag' 'graph_points/gl.vert' 'example_geom_instanced/gl.frag' 'example_geom_instanced/gl.vert' 'graph_points_i/gl.frag' 'graph_points_i/gl.vert' 'circle/gl.vert' 'circle/gl.frag' ' circle/gl2.vert' 'circle/gl2.frag' 'rounded_box/gl.vert' 'rounded_box/gl.frag' 'rounded_box/gl2.vert' 'rounded_box/gl2.frag')

pushd gpu/webgl/shaders

for F in "${EMBED[@]}"
do
  echo "processing $F"
  xxd -i $F > $F.h
done

popd
