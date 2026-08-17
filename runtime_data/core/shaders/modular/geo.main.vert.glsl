#vertex
#version 460


#include "interface_blocks/out_vertex.glsl"
#include "uniform_blocks/common.glsl"
#include "uniform_blocks/model.glsl"

#include "types/vertex.glsl"
void evalAttributes(inout VERTEX vert);
void evalVertex(inout VERTEX vert);
void evalToWorld(inout VERTEX vert);

// Fallback for when gpuMeshDesc does not provide an attrib vertex shader
// position is pretty much guaranteed to exist, so can at least draw a buggy shape
#ifndef ENABLE_VERT_ATTRIB
in vec3 inPosition;

void evalAttributes(inout VERTEX vert) {	
	vert.pos = inPosition.xyz;
	vert.col = vec3(1, 1, 1);
	vert.alpha = 1;
	vert.uv = vec2(0);
	vert.TBN[2] = vec3(0, 0, 1);
	vert.TBN[0] = vec3(1, 0, 0);
	vert.TBN[1] = vec3(0, 1, 0);
}
#endif

void main(){	
	VERTEX vert;	
	vert.pos = vec3(0);
	vert.world_pos = vec3(0);
	vert.col = vec3(1);
	vert.alpha = 1;
	vert.uv = vec2(0);
	vert.TBN = mat3(1, 0, 0, 0, 1, 0, 0, 0, 1);
	vert.invTBN = mat3(1, 0, 0, 0, 1, 0, 0, 0, 1);
	
	evalAttributes(vert);
#ifdef ENABLE_VERT_EXTENSION
	evalVertex(vert);
#endif
	evalToWorld(vert);
	
	
	vec4 scrTo = (matProjection * matView * matModel * vec4(vert.pos, 1));
	vec4 scrFrom = (matProjection * matView * matModel_prev * vec4(vert.pos, 1));
	//scrTo.xyz /= scrTo.w;
	//scrFrom.xyz /= scrFrom.w;
	
	out_vertex.col = vert.col;
	out_vertex.alpha = vert.alpha;
	out_vertex.uv = vert.uv;
	//out_vertex.velo = scrTo.xyz - scrFrom.xyz;
	out_vertex.scr_from = scrFrom;
	out_vertex.scr_to = scrTo;
	/*
	vec3 T = normalize(vec3(mat_Model * vec4(vert.TBN[0].xyz, 0.0)));
	vec3 B = normalize(vec3(mat_Model * vec4(vert.TBN[1].xyz, 0.0)));
	vec3 N = normalize(vec3(mat_Model * vec4(vert.TBN[2].xyz, 0.0)));
	mat3 TBN = mat3(T, B, N);*/
	out_vertex.pos = vert.world_pos;
	out_vertex.TBN = vert.TBN;
	out_vertex.invTBN = inverse(vert.TBN);
	out_vertex.normal = vert.TBN[2];
	
	vec4 pos = matProjection * vec4(vert.pos, 1);
	gl_Position = pos;
}

