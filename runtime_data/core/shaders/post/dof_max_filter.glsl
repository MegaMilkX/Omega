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

void main() {	
	vec3 mask = texture(Color, frag_uv).rgb;
	
	float SIZE = .04;
	float HALF_SIZE = SIZE * .5;
	float GRANULARITY = 5;
	float STEP = SIZE / float(GRANULARITY);
	float aspect = viewportSize.y / viewportSize.x;
	
	float value = .0;
	for(float y = frag_uv.y - HALF_SIZE; y <= frag_uv.y + HALF_SIZE; y += STEP) {
		for(float x = frag_uv.x - HALF_SIZE * aspect; x <= frag_uv.x + HALF_SIZE * aspect; x += STEP) {
			value = max(value, texture(Color, vec2(x, y)).g);
		}
	}
	mask.g = value;	
	
	outColor = vec4(mask, 1);
}
