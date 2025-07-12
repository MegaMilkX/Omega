#version 450

in VERTEX_DATA {
	vec3 pos;
	vec3 col;
	vec2 uv;
	vec3 normal;
	mat3 TBN;
} in_data;

uniform sampler2D texAlbedo;
uniform sampler2D texNormal;
uniform sampler2D texRoughness;
uniform sampler2D texMetallic;
uniform sampler2D texEmission;
uniform sampler2D texAmbientOcclusion;

#include "uniform_blocks/common.glsl"
#include "functions/tonemapping.glsl"

void frag(
	out vec4 outAlbedo,
	out vec4 outPosition,
	out vec4 outNormal,
	out vec4 outMetalness,
	out vec4 outRoughness,
	out vec4 outEmission,
	out vec4 outAmbientOcclusion
) {	
	vec2 uv_offs = vec2(0, time * .05);
	
	vec3 N = normalize(in_data.normal);
	if(!gl_FrontFacing) {
		N *= -1;
	}
	//N = N * 2.0 - 1.0;
	//N = normalize(TBN_frag * N);
	vec3 normal = texture(texNormal, in_data.uv + uv_offs).xyz;
	normal = normal * 2.0 - 1.0;
	mat3 tbn = in_data.TBN;
	tbn[0] = normalize(tbn[0]);
	tbn[1] = normalize(tbn[1]);
	tbn[2] = normalize(tbn[2]);
	normal = normalize(tbn * normal);
	if(!gl_FrontFacing) {
		normal *= -1;
	}
	
	vec4 pix = texture(texAlbedo, in_data.uv + uv_offs);
	float roughness = texture(texRoughness, in_data.uv + uv_offs).x;
	float metallic = texture(texMetallic, in_data.uv + uv_offs).x;
	vec3 emission = texture(texEmission, in_data.uv + uv_offs).xyz;
	vec4 ao = texture(texAmbientOcclusion, in_data.uv + uv_offs);
	
	emission.xyz = pix.xyz * emission.xyz * 4.0;
	
	pix.xyz = inverseGammaCorrect(pix.xyz, gamma);
	emission.xyz = inverseGammaCorrect(emission.xyz, gamma);
	
	outAlbedo = vec4(pix.rgb * in_data.col.rgb, pix.a);
	outPosition = vec4(in_data.pos, 1);
	outNormal = vec4((normal + 1.0) / 2.0, 1);
	outMetalness = vec4(metallic, 0, 0, 1);
	outRoughness = vec4(roughness, 0, 0, 1);
	outEmission = vec4(emission, 1);
	outAmbientOcclusion = vec4(ao.xyz, 1);
}

