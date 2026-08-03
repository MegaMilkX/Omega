#fragment
#version 460

#if FLAGS & 0x0001
#define ENABLE_PARALLAX
#endif

layout(std140) uniform ubMaterial {
	vec4 albedo_color;
	vec3 emission_color;
	float roughness;
	float metallic;
};

#include "types/fragment.glsl"
#include "uniform_blocks/common.glsl"
#include "interface_blocks/in_vertex.glsl"
#include "util.glsl"

uniform sampler2D texAlbedo;
uniform sampler2D texNormal;
uniform sampler2D texRoughness;
uniform sampler2D texMetallic;
uniform sampler2D texEmission;
uniform sampler2D texAmbientOcclusion;

#ifdef ENABLE_PARALLAX
uniform sampler2D texDisplacement;

vec2 parallax(vec2 uv) {
	vec3 N_surfToCam = normalize(cameraPosition - in_vertex.pos);
	float D = dot(N_surfToCam, vec3(in_vertex.TBN[2]));
	
	vec3 n = in_vertex.invTBN * N_surfToCam;
	
	vec2 prev_uv = uv;
	float prev_depth = .0;
	float prev_disp = .0;
	float depth = .0;
	float disp = .0;
	
	const int MIN_LAYERS = 8;
	const int MAX_LAYERS = 32;
	float PARALLAX_STEPS = mix(float(MAX_LAYERS), float(MIN_LAYERS), clamp(abs(D), .0, 1.));
	
	float invD = 1.0 / D;
	const float PARALLAX_DEPTH = .1;
	const float PARALLAX_STEP = PARALLAX_DEPTH / float(PARALLAX_STEPS);
	for(int i = 0; i < PARALLAX_STEPS; ++i) {
		depth = float(i + 1) / float(PARALLAX_STEPS);
		disp = 1.0 - texture(texDisplacement, uv).r;
		if(depth > disp) {
			break;
		}
		
		prev_uv = uv;
		prev_depth = depth;
		prev_disp = disp;
		
		uv -= n.xy * invD * PARALLAX_STEP;
	}
	
	const int MAX_REFINE_STEPS = 5;
	float t0 = .0;
	float t1 = 1.;
	for(int i = 0; i < MAX_REFINE_STEPS; ++i) {
		float t = .5 * (t0 + t1);
		vec2 uv_mid = mix(prev_uv, uv, t);
		float depth_mid = mix(prev_depth, depth, t);
		float h = 1.0 - texture(texDisplacement, uv_mid).r;
		if(depth_mid < h) {
			t0 = t;
		} else {
			t1 = t;
		}
	}
	
	uv = mix(prev_uv, uv, .5 * (t0 + t1));
	
	float before = prev_disp - prev_depth;
	float after = disp - depth;
	float w = before / (before - after);
	uv = mix(prev_uv, uv, w);
	
	return uv;
}
#endif

void evalFragment(inout FRAGMENT frag) {
	vec2 uv = in_vertex.uv;
#ifdef ENABLE_PARALLAX
	uv = parallax(uv);
#endif
	
	vec3 normal = texture(texNormal, uv).xyz;
	frag.normal = normalSampleToWorld(normal, in_vertex.TBN, gl_FrontFacing);
	
	vec4 pix = texture(texAlbedo, uv);
	frag.albedo = pix.rgb * in_vertex.col.rgb * albedo_color.xyz;
	frag.alpha = pix.a * albedo_color.a;
	frag.roughness = texture(texRoughness, uv).x * roughness;
	frag.metallic = texture(texMetallic, uv).x * metallic;
	frag.emission = texture(texEmission, uv).xyz * emission_color;
	frag.ao = texture(texAmbientOcclusion, uv).x;
}

