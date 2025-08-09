#vertex
#version 450 

in vec3 inPosition;
in vec3 inColorRGB;
in vec2 inUV;
in vec2 inUVLightmap;
in vec3 inNormal;
in vec3 inTangent;
in vec3 inBitangent;
out vec3 pos_frag;
out vec3 col_frag;
out vec2 uv_frag;
out vec2 uv_lightmap_frag;
out vec3 normal_frag;
out mat3 fragTBN;

#include "uniform_blocks/common.glsl"
#include "uniform_blocks/model.glsl"

void main(){
	vec3 T = normalize(vec3(matModel * vec4(inTangent, 0.0)));
	vec3 B = normalize(vec3(matModel * vec4(inBitangent, 0.0)));
	vec3 N = normalize(vec3(matModel * vec4(inNormal, 0.0)));
	fragTBN = mat3(T, B, N);
	
	uv_frag = inUV;
	uv_lightmap_frag = inUVLightmap;
	normal_frag = normalize((matModel * vec4(inNormal, 0)).xyz);
	pos_frag = (matModel * vec4(inPosition, 1)).xyz;
	col_frag = inColorRGB;
	vec4 pos = matProjection * matView * matModel * vec4(inPosition, 1);
	gl_Position = pos;
}

#fragment
#version 450
in vec3 pos_frag;
in vec3 col_frag;
in vec2 uv_frag;
in vec2 uv_lightmap_frag;
in vec3 normal_frag;
in mat3 fragTBN;

out vec4 outAlbedo;
out vec4 outPosition;
out vec4 outNormal;
out vec4 outMetalness;
out vec4 outRoughness;
out vec4 outEmission;
out vec4 outAmbientOcclusion;
out vec4 outLightness;

uniform sampler2D texAlbedo;
uniform sampler2D texNormal;
uniform sampler2D texRoughness;
uniform sampler2D texMetallic;
uniform sampler2D texEmission;
uniform sampler2D texAmbientOcclusion;
uniform sampler2D texLightmap;

#include "uniform_blocks/common.glsl"
#include "functions/tonemapping.glsl"

void main(){
	vec3 N = normalize(normal_frag);
	if(!gl_FrontFacing) {
		N *= -1;
	}
	//N = N * 2.0 - 1.0;
	//N = normalize(fragTBN * N);
	vec3 normal = texture(texNormal, uv_frag).xyz;
	normal = normal * 2.0 - 1.0;
	mat3 tbn = fragTBN;
	tbn[0] = normalize(tbn[0]);
	tbn[1] = normalize(tbn[1]);
	tbn[2] = normalize(tbn[2]);
	normal = normalize(tbn * normal);
	if(!gl_FrontFacing) {
		normal *= -1;
	}
	
	vec4 pix = texture(texAlbedo, uv_frag);
	float roughness = texture(texRoughness, uv_frag).x;
	float metallic = texture(texMetallic, uv_frag).x;
	vec3 emission = texture(texEmission, uv_frag).xyz;
	vec4 ao = texture(texAmbientOcclusion, uv_frag);
	vec4 lo = texture(texLightmap, uv_lightmap_frag);
	
	lo.rgb = inverseGammaCorrect(lo.rgb, gamma);
	pix.xyz = inverseGammaCorrect(pix.xyz, gamma);
	emission.rgb = inverseGammaCorrect(emission.rgb, gamma);
	
	if(pix.a < .5) {
		discard;
	} else {
		pix.a = 1.0;
	}
	
	outAlbedo = vec4(pix.rgb * col_frag.rgb, pix.a);
	outPosition = vec4(pos_frag, 1);
	outNormal = vec4((normal + 1.0) / 2.0, 1);
	outMetalness = vec4(metallic, 0, 0, 1);
	outRoughness = vec4(roughness, 0, 0, 1);
	outEmission = vec4(emission, 1);
	outAmbientOcclusion = vec4(ao.xyz, 1);
	outLightness = vec4((lo.rgb + emission.rgb) * pix.rgb * col_frag.rgb, 1);
}
