#fragment
#version 460

#include "types/vertex.glsl"
#include "types/fragment.glsl"
#include "uniform_blocks/common.glsl"
#include "interface_blocks/in_vertex.glsl"
#include "util.glsl"

uniform samplerCube texSky;

// TODO: duplicated due to pass shader including tonemapping.glsl (linking issue)
vec3 _gammaCorrect(vec3 color, float gamma) {
	return pow(color, vec3(1.0 / gamma));
}

void evalFragment(in VERTEX vert, inout FRAGMENT frag) {
	vec2 uv = vert.uv;
	
	frag.normal = vert.TBN[2];
	
	vec2 ndc = gl_FragCoord.xy / viewportSize * 2.0 - 1.0;
	vec4 clip = vec4(ndc, 1.0, 1.0);
	vec4 view = inverse(matProjection) * clip;
	view.xyz /= view.w;

	vec3 view_dir = normalize((inverse(matView) * vec4(view.xyz, 0.0)).xyz);
	view_dir.z = -view_dir.z;
	vec3 color = textureLod(texSky, view_dir, 0).xyz;
	
	// TODO: Figure out who gamma corrects and who doesnt
	//color = _gammaCorrect(color, gamma);
	
	frag.albedo *= vec3(1, 1, 1);
	frag.alpha *= 1.0;
	frag.roughness = 1;
	frag.metallic = 0;
	frag.emission = color;
	frag.ao = 1;
	frag.light_mask = 0;
}

