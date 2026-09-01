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

uniform sampler2D Depth;
uniform sampler2D Normal;
uniform sampler2D SSGI_Raw;

in vec2 frag_uv;
out vec4 outSSGI;

void main() {
	vec2 uv = frag_uv;
	vec2 texelSize = 1.0 / vec2(textureSize(SSGI_Raw, 0));
	
	float centerDepth = texture(Depth, uv).x;
	if(centerDepth >= 1.0) {
		outSSGI = vec4(0);
		return;
	}
	
	vec3 centerNormal = (texture(Normal, uv).xyz - 0.5) * 2.0;
	
	const int RADIUS = 3;
	vec3 sum = vec3(0);
	float weightSum = 0;
	
	for(int y = -RADIUS; y <= RADIUS; ++y) {
		for(int x = -RADIUS; x <= RADIUS; ++x) {
			vec2 sampleUV = uv + vec2(x, y) * texelSize;
			
			float sampleDepth = texture(Depth, sampleUV).x;
			vec3 sampleNormal = (texture(Normal, sampleUV).xyz - 0.5) * 2.0;
			
			float depthDiff = abs(centerDepth - sampleDepth);
			float depthWeight = exp(-depthDiff * depthDiff * 500.0);
			float normalWeight = max(0, dot(centerNormal, sampleNormal));
			
			float weight = normalWeight * depthWeight;
			
			sum += texture(SSGI_Raw, sampleUV).xyz * weight;
			weightSum += weight;
		}
	}
	
	outSSGI = vec4(sum / max(weightSum, .0001), 1.0);
}
