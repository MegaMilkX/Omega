#vertex
#version 460 
in vec3 inPosition;
in float inLineThickness;
in vec4 inColorRGBA;

#include "interface_blocks/out_vertex.glsl"

out LINE_DATA {
	float thickness;
} out_line;

#include "uniform_blocks/common.glsl"
#include "uniform_blocks/model.glsl"

#include "types/vertex.glsl"
void evalAttributes(inout VERTEX vert) {	
	vert.pos = inPosition.xyz;
	vert.col = inColorRGBA.xyz;
	vert.alpha = inColorRGBA.w;
	vert.uv = vec2(0);
	
	vec3 T = vec3(1, 0, 0);
	vec3 B = vec3(0, 1, 0);
	vec3 N = vec3(0, 0, 1);
	vert.TBN = mat3(T, B, N);
	vert.invTBN = mat3(T, B, N);
	
	out_line.thickness = inLineThickness;
}

