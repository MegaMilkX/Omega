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

float bayer4x4(ivec2 p) {
    const int bayer[16] = int[16](
         0,  8,  2, 10,
        12,  4, 14,  6,
         3, 11,  1,  9,
        15,  7, 13,  5
    );
    return float(bayer[(p.y & 3) * 4 + (p.x & 3)]) / 16.0;
}

float bayer8x8(ivec2 p) {
    const int bayer[64] = int[64](
         0, 32,  8, 40,  2, 34, 10, 42,
        48, 16, 56, 24, 50, 18, 58, 26,
        12, 44,  4, 36, 14, 46,  6, 38,
        60, 28, 52, 20, 62, 30, 54, 22,
         3, 35, 11, 43,  1, 33,  9, 41,
        51, 19, 59, 27, 49, 17, 57, 25,
        15, 47,  7, 39, 13, 45,  5, 37,
        63, 31, 55, 23, 61, 29, 53, 21
    );
    return float(bayer[(p.y & 7) * 8 + (p.x & 7)]) / 64.0;
}

float hash(vec3 p) {
    p = fract(p * vec3(443.8975, 397.2973, 491.1871));
    p += dot(p.zxy, p.yxz + 19.19);
    return fract(p.x * p.y * p.z);
}

void main(){
	vec3 N = normalize(in_vertex.normal);
	if(!gl_FrontFacing) {
		N *= -1;
	}
	
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
	
	// TODO: Distance dither experiment
	if(false) {
		float fadeStart = 50;
		float fadeEnd = 100;
		float dist = length(in_vertex.pos - cameraPosition);
		float alpha = 1.0 - smoothstep(fadeStart, fadeEnd, dist);
		float threshold = hash(in_vertex.pos * 0.1);
		//float threshold = bayer8x8(ivec2(gl_FragCoord.xy));
		if (alpha < threshold) discard;
	}
	
	// TODO: Switch this with flags
	if(frag.alpha < .1) {
		discard;
	}
	
	//frag.emission = frag.albedo * frag.emission * 4.0;
	
	frag.albedo = inverseGammaCorrect(frag.albedo, gamma);
	//frag.emission = inverseGammaCorrect(frag.emission, gamma);
	
	vec3 velo
		= in_vertex.scr_to.xyz / in_vertex.scr_to.w
		- in_vertex.scr_from.xyz / in_vertex.scr_from.w;
	
	outAlbedo = vec4(frag.albedo, 1/*frag.alpha*/);
	outPosition = vec4(in_vertex.pos, 1);
	outNormal = vec4((frag.normal + 1.0) * 0.5, frag.light_mask);
	outMetalness = vec4(frag.metallic, 0, 0, 1);
	outRoughness = vec4(frag.roughness, 0, 0, 1);
	outAmbientOcclusion = vec4(frag.ao, 0, 0, 1);
	outLightness = vec4(frag.emission * frag.albedo, 1);
	outVelocityMap = vec4(velo, 1);
}

