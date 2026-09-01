#vertex
#version 450

#include "uniform_blocks/common.glsl"

in vec3 inPosition;
out vec2 frag_uv;

void main(){
	vec2 uv = vec2((inPosition.x + 1.0) * .5, (inPosition.y + 1.0) * .5);
	frag_uv = mix(vp_rect_ratio.xy, vp_rect_ratio.zw, uv.xy);
	
	gl_Position = vec4(inPosition, 1.0);
}

#fragment
#version 450

#include "uniform_blocks/common.glsl"
#include "functions/tonemapping.glsl"

uniform sampler2D Albedo;
uniform sampler2D SSGI;

in vec2 frag_uv;
out vec4 outLightness;

void main() {
	vec2 uv = frag_uv;
	
	vec3 albedo = texture(Albedo, uv).xyz;
	//albedo = gammaCorrect(albedo, gamma);
	vec3 gi = texture(SSGI, uv).xyz;
	vec3 color = gi * min(albedo, 1.0);
	color = any(isnan(color)) || any(isinf(color)) ? vec3(0) : color;
	outLightness = vec4(color, 1);
}
