#vertex
#version 460

in vec3 inPosition;
in vec3 inColorRGB;
in vec2 inUV; // TODO: Why is this even here?
in float inTextUVLookup;

uniform sampler2D texTextUVLookupTable;


#include "types/vertex.glsl"
void evalAttributes(inout VERTEX vert) {	
	vert.pos = inPosition.xyz;
	vert.col = inColorRGB.xyz;
	
	vert.uv = texelFetch(texTextUVLookupTable, ivec2(inTextUVLookup, 0), 0).xy;
	
	vec3 T = vec3(1, 0, 0);
	vec3 B = vec3(0, 1, 0);
	vec3 N = vec3(0, 0, 1);
	vert.TBN = mat3(T, B, N);
	vert.invTBN = vert.TBN;
	
	// TODO: Text uv lookup value
}

