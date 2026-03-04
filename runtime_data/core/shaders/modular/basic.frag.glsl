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

void evalFragment(inout FRAGMENT frag) {
	vec3 normal = texture(texNormal, in_vertex.uv).xyz;
	frag.normal = normalSampleToWorld(normal, in_vertex.TBN, gl_FrontFacing);
	
	vec4 pix = texture(texAlbedo, in_vertex.uv);
	frag.albedo = pix.rgb * in_vertex.col.rgb;
	frag.alpha = pix.a;
	frag.roughness = texture(texRoughness, in_vertex.uv).x;
	frag.metallic = texture(texMetallic, in_vertex.uv).x;
	frag.emission = texture(texEmission, in_vertex.uv).xyz;
	frag.ao = texture(texAmbientOcclusion, in_vertex.uv).x;
}

