#vertex
#version 460

in vec3 inPosition;
in vec3 inColorRGB;
in vec2 inUV;
in vec3 inNormal;
in vec3 inTangent;
in vec3 inBitangent;

#include "uniform_blocks/model.glsl"

#include "types/vertex.glsl"
void evalAttributes(inout VERTEX vert) {	
	vert.pos = inPosition.xyz;
	vert.col = inColorRGB.xyz;
	vert.alpha = 1;
	vert.uv = inUV.xy;
	
	vec3 T = inTangent.xyz;
	vec3 B = inBitangent.xyz;
	vec3 N = inNormal.xyz;
	vert.TBN = mat3(T, B, N);
	vert.invTBN = inverse(vert.TBN);
}

