#vertex
#version 460

in vec3 inPosition;
//in vec3 inColorRGB;
in vec2 inUV;

#include "types/vertex.glsl"
void evalAttributes(inout VERTEX vert) {	
	vert.pos = inPosition.xyz;
	vert.col = vec3(1, 1, 1);//inColorRGB.xyz;
	vert.uv = inUV.xy;
	
	vec3 T = vec3(1, 0, 0);
	vec3 B = vec3(0, 1, 0);
	vec3 N = vec3(0, 0, 1);
	vert.TBN = mat3(T, B, N);
	vert.invTBN = vert.TBN;
}

