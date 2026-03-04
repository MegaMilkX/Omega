#vertex
#version 460

#include "types/vertex.glsl"
#include "uniform_blocks/common.glsl"

in vec2 inUVLightmap;
out vec2 uv_lm;

void evalVertex(inout VERTEX vert) {
	uv_lm = inUVLightmap;
}

