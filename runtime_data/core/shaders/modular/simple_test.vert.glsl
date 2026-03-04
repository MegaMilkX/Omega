#vertex
#version 460

#include "types/vertex.glsl"
#include "uniform_blocks/common.glsl"

void evalVertex(inout VERTEX vert) {
	vert.pos = vert.pos + vert.normal * (sin(time * 4.0 + vert.pos.y * 2.0) + 1.0) * .5 * .2;
}

