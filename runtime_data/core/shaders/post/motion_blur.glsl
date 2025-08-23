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
uniform sampler2D VelocityMap;
out vec4 outColor;

void main() {	
	vec2 texelSize = 1.0 / vec2(textureSize(Color, 0));
	float result = .0;
	
	const int ITERATION_COUNT = 32;
	
	vec2 texSize = textureSize(Color, 0);
	vec2 V = texture(VelocityMap, frag_uv).xy;
	float MAX_BLUR = 500.0;
	V *= .5;
	V = clamp(V, -MAX_BLUR / texSize, MAX_BLUR / texSize);
	
	float weightSum = .0;
	vec3 color = vec3(0,0,0);
	for(int i = 0; i < ITERATION_COUNT; ++i) {
		float t = (float(i) / float(ITERATION_COUNT - 1)) - .5;
		vec2 uv = frag_uv + V * t;
		vec3 c = texture(Color, uv).xyz;
		
		float w = exp(-4.0 * t * t);
		color += c * w;
		weightSum += w;
	}
	color = color / weightSum;
	
	outColor = vec4(color, 1);
}
