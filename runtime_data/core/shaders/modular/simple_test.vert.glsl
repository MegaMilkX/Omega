#vertex
#version 460

#include "types/vertex.glsl"
#include "uniform_blocks/common.glsl"

void evalVertex(inout VERTEX vert) {
	vec3 N = vert.TBN[2];
	vert.pos = vert.pos + N * (sin(time * 4.0 + vert.pos.y * 2.0) + 1.0) * .5 * .2;
}

