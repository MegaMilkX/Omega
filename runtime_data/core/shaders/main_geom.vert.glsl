#version 450 

in vec3 inPosition;
in vec3 inColorRGB;
in vec2 inUV;
in vec3 inNormal;
in vec3 inTangent;
in vec3 inBitangent;

out VERTEX_DATA {
	vec3 pos;
	vec3 col;
	vec2 uv;
	vec3 normal;
	mat3 TBN;
} out_data;

#include "uniform_blocks/common.glsl"
#include "uniform_blocks/model.glsl"

void main(){
	vec3 T = normalize(vec3(matModel * vec4(inTangent, 0.0)));
	vec3 B = normalize(vec3(matModel * vec4(inBitangent, 0.0)));
	vec3 N = normalize(vec3(matModel * vec4(inNormal, 0.0)));
	out_data.TBN = mat3(T, B, N);
	
	out_data.uv = inUV;
	out_data.normal = normalize((matModel * vec4(inNormal, 0)).xyz);
	out_data.pos = (matModel * vec4(inPosition, 1)).xyz;
	out_data.col = inColorRGB;
	
	vec4 pos = matProjection * matView * matModel * vec4(inPosition, 1);
	gl_Position = pos;
}

