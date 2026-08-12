#vertex
#version 460

#include "uniform_blocks/common.glsl"
#include "uniform_blocks/model.glsl"

#include "types/vertex.glsl"
void evalMesh(inout VERTEX vert);
void evalVertex(inout VERTEX vert);
void evalToWorld(inout VERTEX vert);

void main(){	
	VERTEX vert;
	{
		vert.pos = vec3(0);
		vert.col = vec3(1);
		vert.uv = vec2(0);
		
		evalMesh(vert);
#ifdef ENABLE_VERT_EXTENSION
		evalVertex(vert);
#endif
#ifdef ENABLE_INSTANCING
		evalToWorld(vert);
#endif
	}
	
	vec3 T = normalize(vec3(matModel * vec4(vert.tangent, 0.0)));
	vec3 B = normalize(vec3(matModel * vec4(vert.bitangent, 0.0)));
	vec3 N = normalize(vec3(matModel * vec4(vert.normal, 0.0)));
	out_vertex.TBN = mat3(T, B, N);
	out_vertex.invTBN = inverse(out_vertex.TBN);
	
	vec4 scrTo = (matProjection * matView * matModel * vec4(vert.pos, 1));
	vec4 scrFrom = (matProjection * matView * matModel_prev * vec4(vert.pos, 1));
	//scrTo.xyz /= scrTo.w;
	//scrFrom.xyz /= scrFrom.w;
	
	out_vertex.uv = vert.uv;
	out_vertex.normal = normalize((matModel * vec4(vert.normal, 0)).xyz);
	out_vertex.pos = (matModel * vec4(vert.pos, 1)).xyz;
	//out_vertex.velo = scrTo.xyz - scrFrom.xyz;
	out_vertex.scr_from = scrFrom;
	out_vertex.scr_to = scrTo;
	out_vertex.col = vert.col;
	
	vec4 pos = matProjection * matView * matModel * vec4(vert.pos, 1);
	gl_Position = pos;
}

