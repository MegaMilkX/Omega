#fragment
#version 460

uniform sampler2D texAlbedo;
uniform sampler2D Normal;
uniform sampler2D Depth;

out vec4 outAlbedo;
out vec4 outPosition;
out vec4 outNormal;
out vec4 outMetalness;
out vec4 outRoughness;
//out vec4 outEmission;
out vec4 outAmbientOcclusion;
out vec4 outLightness;
out vec4 outVelocityMap;

#include "interface_blocks/in_vertex.glsl"
#include "interface_blocks/in_decal.glsl"
#include "uniform_blocks/common.glsl"
#include "uniform_blocks/decal.glsl"
#include "functions/tonemapping.glsl"

#include "types/fragment.glsl"
void evalFragment(inout FRAGMENT frag);

float contains(vec3 pos, vec3 bottom_left, vec3 top_right) {
	vec3 s = step(bottom_left, pos) - step(top_right, pos);
	return s.x * s.y * s.z;
}

vec3 worldPosFromDepth(float depth, vec2 uv, mat4 proj, mat4 view) {
	float z = depth * 2.0 - 1.0;

	vec4 clipSpacePosition = vec4(uv * 2.0 - 1.0, z, 1.0);
	vec4 viewSpacePosition = inverse(proj) * clipSpacePosition;

	viewSpacePosition /= viewSpacePosition.w;
	
	vec4 worldSpacePosition = inverse(view) * viewSpacePosition;

	return worldSpacePosition.xyz;
}

void main(){
	vec2 pos = (in_decal.clip_pos.xy / in_decal.clip_pos.w + vec2(1)) * .5;	
	vec2 uv = pos;
	vec2 frag_uv_viewport_space = uv;
	vec2 frag_uv = mix(vp_rect_ratio.xy, vp_rect_ratio.zw, frag_uv_viewport_space.xy);
	
	vec4 depth_sample = texture(Depth, frag_uv);
    vec4 normal_sample = texture(Normal, frag_uv);
	normal_sample.xyz = normal_sample.xyz * 2.0 - 1.0;
	
	vec3 world_pos = worldPosFromDepth(depth_sample.x, frag_uv_viewport_space, in_decal.projection, in_decal.view);
	vec4 decal_pos = inverse(in_decal.model) * vec4(world_pos, 1);
	if(contains(decal_pos.xyz, -boxSize * .5, boxSize * .5) < 1.0) {
		discard;
	}
    vec3 decal_N = ((in_decal.model) * vec4(0, 1, 0, 0)).xyz;
    float d = max(0.0, dot(decal_N, normal_sample.xyz));
    
	vec2 decal_uv = vec2(1.0 - decal_pos.x / boxSize.x + .5, decal_pos.z / boxSize.z + .5);
	vec4 decal_sample = texture(texAlbedo, decal_uv);
	
	//decal_sample.xyz = inverseGammaCorrect(decal_sample.xyz, gamma);	
	decal_sample.xyz = inverseGammaCorrect(decal_sample.xyz, gamma);
	decal_sample.xyz = inverseTonemapFilmicUncharted2(decal_sample.xyz, .1);
	
	float alpha = (1.0 - abs(decal_pos.y / boxSize.y * 2.0)) * d;
	outAlbedo = vec4(decal_sample.xyz, decal_sample.a * alpha) * RGBA;
	//outAlbedo = vec4(1, 0, 0, 1);
	
	/*
	vec3 N = normalize(in_vertex.normal);
	if(!gl_FrontFacing) {
		N *= -1;
	}
	//N = N * 2.0 - 1.0;
	//N = normalize(TBN_frag * N);
	
	FRAGMENT frag;
	{
		frag.albedo = vec3(1, 1, 1);
		frag.normal = N;
		frag.roughness = 1.0;
		frag.metallic = 0.0;
		frag.emission = vec3(0, 0, 0);
		frag.ao = 0.0;
		frag.alpha = 1.0;
#ifdef ENABLE_FRAG_EXTENSION
		evalFragment(frag);
#endif
	}	
	
	frag.emission = frag.albedo * frag.emission * 4.0;
	
	frag.albedo = inverseGammaCorrect(frag.albedo, gamma);
	frag.emission = inverseGammaCorrect(frag.emission, gamma);
	
	outAlbedo = vec4(frag.albedo, frag.alpha);
	outPosition = vec4(in_vertex.pos, 1);
	outNormal = vec4((frag.normal + 1.0) * 0.5, 1);
	outMetalness = vec4(frag.metallic, 0, 0, 1);
	outRoughness = vec4(frag.roughness, 0, 0, 1);
	outAmbientOcclusion = vec4(frag.ao, 0, 0, 1);
	outLightness = vec4(frag.emission * frag.albedo, 1);
	outVelocityMap = vec4(in_vertex.velo, 1);*/
}

