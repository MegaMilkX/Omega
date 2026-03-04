#vertex
#version 460

in vec3 inPosition;
in vec3 inColorRGB;
in vec2 inUV;
in vec3 inNormal;
in vec3 inTangent;
in vec3 inBitangent;

#include "interface_blocks/out_vertex.glsl"
#include "uniform_blocks/common.glsl"
#include "uniform_blocks/model.glsl"

#include "types/vertex.glsl"
void evalVertex(inout VERTEX vert);

void main(){
	vec3 T = normalize(vec3(matModel * vec4(inTangent, 0.0)));
	vec3 B = normalize(vec3(matModel * vec4(inBitangent, 0.0)));
	vec3 N = normalize(vec3(matModel * vec4(inNormal, 0.0)));
	out_vertex.TBN = mat3(T, B, N);
	
	VERTEX vert;
	{
		vert.pos = inPosition.xyz;
		vert.col = inColorRGB.xyz;
		vert.uv = inUV.xy;
		vert.normal = inNormal.xyz;
#ifdef ENABLE_VERT_EXTENSION
		evalVertex(vert);
#endif
	}
	
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

