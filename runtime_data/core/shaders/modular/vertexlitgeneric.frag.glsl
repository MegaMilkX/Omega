#fragment
#version 460

#include "types/vertex.glsl"
#include "types/fragment.glsl"
#include "interface_blocks/in_vertex.glsl"
#include "util.glsl"

uniform sampler2D texAlbedo;
uniform sampler2D texNormal;
uniform sampler2D texRoughness;
uniform sampler2D texMetallic;
uniform sampler2D texEmission;
uniform sampler2D texAmbientOcclusion;

uniform int alpha_mode;

void evalFragment(in VERTEX vert, inout FRAGMENT frag) {
	vec3 normal = texture(texNormal, vert.uv).xyz;
	frag.normal = normalSampleToWorld(normal, vert.TBN, gl_FrontFacing);
	
	vec4 pix = texture(texAlbedo, vert.uv);
	frag.albedo = pix.rgb * vert.col.rgb;
	if(alpha_mode == 0) {
		if(pix.a < 0.5) {
			discard;
		}
		frag.alpha *= 1;
		frag.roughness = texture(texRoughness, vert.uv).x;
		frag.emission = texture(texEmission, vert.uv).xyz;
	} else if(alpha_mode == 1) {
		frag.alpha *= 1.0;
		frag.roughness = pix.a;
		frag.emission = texture(texEmission, vert.uv).xyz;
	} else if(alpha_mode == 2) {
		frag.alpha *= 1.0;
		frag.roughness = texture(texRoughness, vert.uv).x;
		frag.emission = pix.rgb * pix.a * 10.0;
	}
	frag.metallic = texture(texMetallic, vert.uv).x;
	frag.ao = texture(texAmbientOcclusion, vert.uv).x;
}

