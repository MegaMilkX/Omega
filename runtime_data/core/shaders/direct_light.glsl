#vertex
#version 460

layout(location = 0) in vec3 inPosition;
out vec2 fragUV;

#include "uniform_blocks/common.glsl"

void main() {
	vec2 uv = vec2((inPosition.x + 1.0) * .5, (inPosition.y + 1.0) * .5);
	fragUV = mix(vp_rect_ratio.xy, vp_rect_ratio.zw, uv.xy);
	
	gl_Position = vec4(inPosition, 1.0);
}


#fragment
#version 460

uniform sampler2D Shadowmap;
uniform sampler2D WorldPos;
uniform sampler2D Normal;

in vec2 fragUV;
out vec4 outLightness;
//out vec4 outAmbientOcclusion;

#include "uniform_blocks/common.glsl"

layout(std140) uniform ubDirectLight {
	mat4 dirLightProj;
	mat4 dirLightView;
};
float lightSize = .1; // TODO:

const vec2 poissonDisk[16] = vec2[](
    vec2(-0.94201624, -0.39906216), vec2(0.94558609, -0.76890725),
    vec2(-0.094184101, -0.92938870), vec2(0.34495938, 0.29387760),
    vec2(-0.91588581, 0.45771432), vec2(-0.81544232, -0.87912464),
    vec2(-0.38277543, 0.27676845), vec2(0.97484398, 0.75648379),
    vec2(0.44323325, -0.97511554), vec2(0.53742981, -0.47373420),
    vec2(-0.26496911, -0.41893023), vec2(0.79197514, 0.19090188),
    vec2(-0.24188840, 0.99706507), vec2(-0.81409955, 0.91437590),
    vec2(0.19984126, 0.78641367), vec2(0.14383161, -0.14100790)
);

void findBlocker(vec2 uv, float receiverDepth, float searchRadius, out float avgBlockerDepth, out float numBlockers) {
	float blockerSum = .0;
	numBlockers = .0;
	
	float PI2 = 6.28318530718;
	float PI = PI2 * .5;
	
	float Directions = 16.0;
	float Quality = 3.0;
	float Size = 1.0;
	
	vec2 Radius = vec2(Size) * searchRadius;
	float sum = .0;
	
	for(float d = 0.0; d < PI2; d += PI2 / Directions) {
		for(float i = 1.0 / Quality; i <= 1.0; i += 1.0 / Quality) {
			vec2 tex_coord = uv + vec2(cos(d), sin(d)) * Radius * i;
			float sd = texture(Shadowmap, tex_coord).x;
			
			float w = smoothstep(receiverDepth + 0.001, receiverDepth - 0.02, sd);
			blockerSum += sd * w;
			numBlockers += w;
		}
	}
	
	avgBlockerDepth = (numBlockers > .0) ? (blockerSum / numBlockers) : .0;
}

float penumbraSize(float receiverDepth, float avgBlockerDepth) {	
	float d = max(avgBlockerDepth, 1e-5);
	return (receiverDepth - avgBlockerDepth) * lightSize / d;
}

float pcfFilter(vec2 uv, float receiverDepth, float radius) {
	float sum = .0;
	for (int i = 0; i < 16; i++) {
		vec2 offset = poissonDisk[i] * radius;
		float depth = texture(Shadowmap, uv + offset).x;
		sum += smoothstep(receiverDepth + 0.0005, receiverDepth - 0.0005, depth);
	}
	return sum / 16.0;
}
float myFilter(vec2 uv, float receiverDepth, float radius) {	
	float PI2 = 6.28318530718;
	float PI = PI2 * .5;
	
	float Directions = 16.0;
	float Quality = 12.0;
	float Size = 1.0;
	
	//vec2 Radius = Size / vec2(1024, 1024);
	vec2 Radius = vec2(Size) * radius;
	float sum = .0;
	
	for(float d = 0.0; d < PI2; d += PI2 / Directions) {
		for(float i = 1.0 / Quality; i <= 1.0; i += 1.0 / Quality) {
			vec2 tex_coord = uv + vec2(cos(d), sin(d)) * Radius * i;				
			float shadow_depth = texture(Shadowmap, tex_coord).x;
			//float s = shadow_depth < receiverDepth ? 0 : 1.;
			sum += smoothstep(receiverDepth + 0.0005, receiverDepth - 0.0005, shadow_depth);
		}
	}
	
	sum /= Quality * Directions;
	return sum;
}

float ndcToDepth(float z) { return z * 0.5 + 0.5; }

float linearizeDepth(float depth, float near, float far) {
	return near * far / (far + near - depth * (far - near));
}

void main() {
    vec3 wpos = texture(WorldPos, fragUV).xyz;
	
	vec4 nc_pos4 = dirLightProj * dirLightView * vec4(wpos, 1);
	nc_pos4.xyz /= nc_pos4.w;
	
	vec2 shadowUV = nc_pos4.xy * 0.5 + 0.5;
    //float shadow_depth = texture(Shadowmap, shadowUV).x;
	
	vec3 L = inverse(dirLightView)[2].xyz;
	vec3 N = (texture(Normal, fragUV).xyz - .5) * 2.0;
	
	float minBias = 0.0005;
	float maxBias = 0.005;
	float bias = max(maxBias * (1.0 - dot(N, L)), minBias);
	
	float receiverDepth = ndcToDepth(nc_pos4.z) - bias;
	float avgBlockerDepth, numBlockers;
	findBlocker(shadowUV, receiverDepth, lightSize * .5, avgBlockerDepth, numBlockers);
	
	float lit;
	if(receiverDepth > 1.0 || numBlockers < .5) {
		lit = 1.0;
	} else {
		//float radius = max(penumbraSize(receiverDepth, avgBlockerDepth), 0.0);
		float radius = .0015;
		lit = myFilter(shadowUV, receiverDepth, radius);
		lit = 1.0 - lit;
	}
	
	//vec3 col = mix(vec3(0.025, 0.01, 0.02), vec3(.4, .2, .6), max(0, dot(L, N)) * lit);
	vec3 inshadow = vec3(0.17, 0.19, 0.2) * 1.0;
	vec3 col = mix(inshadow, vec3(1), max(0, dot(L, N)) * lit);
	//outAmbientOcclusion = vec4(.15, 0, 0, 1.0 - (max(0, dot(L, N)) * lit));
	outLightness = vec4(col, 1);
	//outLightness = vec4(softness, 0, 0, 1);
}