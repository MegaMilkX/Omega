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

in vec2 frag_uv;
uniform sampler2D Color;
out vec4 outColor;


void main(){
	float PI2 = 6.28318530718;
	
	float Directions = 16.0;
	float Quality = 3.0;
	float Size = 16.0;
	
	vec2 Radius = Size/viewportSize.xy;
	vec4 color = texture(Color, frag_uv).xyzw;
	float alphaSum = color.a;
	float a = color.a;
	
	for(float d = 0.0; d < PI2; d += PI2 / Directions) {
		for(float i = 1.0 / Quality; i <= 1.0; i += 1.0 / Quality) {
			vec4 c = texture(Color, frag_uv + vec2(cos(d), sin(d)) * Radius * i);
			//color += c;
			color += c * c.w;
			alphaSum += c.w;
			a += c.w;
		}
	}
	
	if(alphaSum > .0) {
		color /= alphaSum;
	} else {
		color = vec4(0);
	}
	
	//color /= Quality * Directions - 15.0;
	a /= Quality * Directions - 15.0;
	a = min(3.0, a * 3.0);
	outColor = vec4(color.xyz, a);
}
