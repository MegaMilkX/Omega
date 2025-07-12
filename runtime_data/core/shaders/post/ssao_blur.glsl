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
	vec2 texelSize = 1.0 / vec2(textureSize(Color, 0));
	float result = .0;
	
	for(int y = -2; y < 2; ++y) {
		for(int x = -2; x < 2; ++x) {
			vec2 offset = vec2(float(x), float(y)) * texelSize;
			result += texture(Color, frag_uv + offset).r;
		}
	}
	
	const float div = 1.0 / (4.0 * 4.0);
	result = result * div;
	outColor = vec4(result, .0, .0, 1.);
}
