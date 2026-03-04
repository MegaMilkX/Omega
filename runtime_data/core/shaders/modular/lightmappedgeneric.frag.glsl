#fragment
#version 460

#include "types/fragment.glsl"
#include "interface_blocks/in_vertex.glsl"
#include "util.glsl"

uniform sampler2D texAlbedo;
uniform sampler2D texNormal;
uniform sampler2D texRoughness;
uniform sampler2D texMetallic;
uniform sampler2D texEmission;
uniform sampler2D texAmbientOcclusion;
uniform sampler2D texLightmap;

uniform int alpha_mode;

in vec2 uv_lm;

void evalFragment(inout FRAGMENT frag) {
	vec3 normal = texture(texNormal, in_vertex.uv).xyz;
	frag.normal = normalSampleToWorld(normal, in_vertex.TBN, gl_FrontFacing);
	
	frag.light_mask = 0;
	
	vec4 pix = texture(texAlbedo, in_vertex.uv);
	frag.albedo = pix.rgb * in_vertex.col.rgb;
	vec3 lo = texture(texLightmap, uv_lm).xyz;
	lo = lo * pix.rgb * in_vertex.col.rgb;
	if(alpha_mode == 0) {
		if(pix.a < 0.5) {
			discard;
		}
		frag.alpha = 1;
		frag.roughness = texture(texRoughness, in_vertex.uv).x;
		frag.emission = lo;
	} else if(alpha_mode == 1) {
		frag.alpha = 1.0;
		frag.roughness = pix.a;
		frag.emission = lo;
	} else if(alpha_mode == 2) {
		frag.alpha = 1.0;
		frag.roughness = texture(texRoughness, in_vertex.uv).x;
		frag.emission = lo + pix.rgb * pix.a;
	}
	frag.metallic = texture(texMetallic, in_vertex.uv).x;
	frag.ao = texture(texAmbientOcclusion, in_vertex.uv).x;
}

