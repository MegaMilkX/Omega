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
	vec2 V = frag_uv * 2.0 - 1.0;
	float aspect = viewportSize.y / viewportSize.x;
	V.y *= aspect;
	float len = length(V);
	len = len * len;
	vec2 N = normalize(V);
	
	/*
	vec2 radial = radialDistortion(uv, .01, .01, .01);
	vec2 tangential = tangentialDistortion(uv, 0, 0);
	
	uv = radial + tangential;
	uv = (uv + 1.0) * .5;
	*/
	
	float red_offset = .002 * len;
	float blue_offset = 0 * len;
	float green_offset = -.002 * len;
	
	float R = texture(Color, frag_uv + N * red_offset).x;
	float G = texture(Color, frag_uv + N * blue_offset).y;
	float B = texture(Color, frag_uv + N * green_offset).z;
	
	vec4 color = vec4(R, G, B, 1);
	
	outColor = vec4(color.xyz, color.w);
}

