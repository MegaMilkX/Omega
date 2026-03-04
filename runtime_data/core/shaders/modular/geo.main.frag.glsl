#fragment
#version 460

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
#include "uniform_blocks/common.glsl"
#include "functions/tonemapping.glsl"

#include "types/fragment.glsl"
void evalFragment(inout FRAGMENT frag);

void main(){
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
		frag.light_mask = 1.0;
		frag.roughness = 1.0;
		frag.metallic = 0.0;
		frag.emission = vec3(0, 0, 0);
		frag.ao = 0.0;
		frag.alpha = 1.0;
#ifdef ENABLE_FRAG_EXTENSION
		evalFragment(frag);
#endif
	}	
	
	//frag.emission = frag.albedo * frag.emission * 4.0;
	
	frag.albedo = inverseGammaCorrect(frag.albedo, gamma);
	//frag.emission = inverseGammaCorrect(frag.emission, gamma);
	
	vec3 velo
		= in_vertex.scr_to.xyz / in_vertex.scr_to.w
		- in_vertex.scr_from.xyz / in_vertex.scr_from.w;
	
	outAlbedo = vec4(frag.albedo, frag.alpha);
	outPosition = vec4(in_vertex.pos, 1);
	outNormal = vec4((frag.normal + 1.0) * 0.5, frag.light_mask);
	outMetalness = vec4(frag.metallic, 0, 0, 1);
	outRoughness = vec4(frag.roughness, 0, 0, 1);
	outAmbientOcclusion = vec4(frag.ao, 0, 0, 1);
	outLightness = vec4(frag.emission * frag.albedo, 1);
	outVelocityMap = vec4(velo, 1);
}

