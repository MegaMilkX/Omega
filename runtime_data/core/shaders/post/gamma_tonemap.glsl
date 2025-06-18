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

in vec2 frag_uv;
uniform sampler2D Color;
out vec4 outColor;


void main() {
	float gamma = 2.2;
	
	vec4 color = texture(Color, frag_uv).xyzw;
	
	color.xyz = tonemapFilmicUncharted2(color.xyz, 0.1);
	color.xyz = gammaCorrect(color.xyz, gamma);
	
	outColor = vec4(color.xyz, color.w);
}
