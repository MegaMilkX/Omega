#fragment
#version 460

out vec4 outAlbedo;

#include "interface_blocks/in_vertex.glsl"
#include "uniform_blocks/common.glsl"

void main() {
	float depth = gl_FragCoord.z;
	float biasFactor = 0.00001;
	float bias = biasFactor * depth;
	gl_FragDepth = depth - bias;
	outAlbedo = vec4(1, 1, 1, 1);
}

