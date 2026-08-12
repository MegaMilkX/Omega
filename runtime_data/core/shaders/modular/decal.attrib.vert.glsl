#vertex
#version 460

in vec3 inPosition;

#include "interface_blocks/out_decal.glsl"
#include "uniform_blocks/common.glsl"
#include "uniform_blocks/model.glsl"
#include "uniform_blocks/decal.glsl"
#include "types/vertex.glsl"
void evalAttributes(inout VERTEX vert) {	
	vert.pos = inPosition.xyz * boxSize;
	vert.col = RGBA.xyz;
	vert.uv = vec2(0);

	// Decals are set up a bit dumb, their N is up
	vec3 T = normalize(vec3(matModel * vec4(-1, 0, 0, 0.0)));
	vec3 B = normalize(vec3(matModel * vec4(0, 0, 1, 0.0)));
	vec3 N = normalize(vec3(matModel * vec4(0, 1, 0, 0.0)));
	vert.TBN = mat3(T, B, N);
	vert.invTBN = inverse(vert.TBN);
	
	vec4 pos = matProjection * matView * matModel * vec4(vert.pos, 1);
	out_decal.projection = matProjection;
	out_decal.view = matView;
	out_decal.model = matModel;
	out_decal.clip_pos = pos;
}

