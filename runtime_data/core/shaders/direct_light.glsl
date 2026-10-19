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

uniform sampler2DArray Shadowmap;
uniform sampler2D WorldPos;
uniform sampler2D Normal;

in vec2 fragUV;
out vec4 outLightness;
//out vec4 outAmbientOcclusion;

#include "uniform_blocks/common.glsl"

#define NUM_CASCADES 6

layout(std140) uniform ubDirectLight {
	mat4 cascadeViewProj[NUM_CASCADES]; // proj * view, precomputed on the CPU
	vec4 cascadeParams[NUM_CASCADES];   // x: far split (camera view depth)
	                                    // y: 1 / ortho width (world units)
	                                    // z: ortho depth range (world units)
	vec4 lightDir;                      // xyz: unit vector pointing TOWARD the light
};

const float lightSizeWorld     = 0.1;  // blocker search radius, world units (tune)
const float filterRadiusWorld  = 0.05; // PCF radius, world units (tune)

float sampleShadow(vec2 uv, int cascade) {
	return textureLod(Shadowmap, vec3(uv, float(cascade)), 0.0).x;
}

// 1 = fully occluded, 0 = lit
float occlusion(float sd, float receiver) {
	return 1.0 - smoothstep(receiver - 0.0005, receiver + 0.0005, sd);
}

float findBlockerCount(int c, vec2 uv, float receiver, float radius) {
	float count = 0.0;
	for (int d = 0; d < 16; ++d) {
		float a = float(d) * (6.28318530718 / 16.0);
		for (int i = 1; i <= 3; ++i) {
			vec2 p = uv + vec2(cos(a), sin(a)) * radius * (float(i) / 3.0);
			float sd = sampleShadow(p, c);
			count += 1.0 - smoothstep(receiver - 0.02, receiver + 0.001, sd);
		}
	}
	return count;
}

float filterShadow(int c, vec2 uv, float receiver, float radius) {
	float sum = 0.0;
	for (int d = 0; d < 16; ++d) {
		float a = float(d) * (6.28318530718 / 16.0);
		for (int i = 1; i <= 12; ++i) {
			vec2 p = uv + vec2(cos(a), sin(a)) * radius * (float(i) / 12.0);
			sum += occlusion(sampleShadow(p, c), receiver);
		}
	}
	return sum / (16.0 * 12.0);
}

// returns lit factor, 1 = lit, 0 = shadowed
float cascadeLit(int c, vec3 wpos, float NdotL) {
	vec3 s = (cascadeViewProj[c] * vec4(wpos, 1.0)).xyz * 0.5 + 0.5; // ortho: w == 1

	float res        = float(textureSize(Shadowmap, 0).x);
	float invWidth   = cascadeParams[c].y;
	float depthRange = cascadeParams[c].z;

	// bias expressed in texels of THIS cascade, converted to depth units
	float texelWorld = 1.0 / (invWidth * res);
	float bias       = texelWorld * max(2.0 * (1.0 - NdotL), 0.5) / depthRange;
	float receiver   = s.z - bias;
	if (receiver > 1.0) return 1.0;

	// radii: constant in world space, but never below ~1.5 texels
	float searchR = max(lightSizeWorld    * invWidth, 1.5 / res);
	float filterR = max(filterRadiusWorld * invWidth, 1.5 / res);

	if (findBlockerCount(c, s.xy, receiver, searchR) < 0.5) return 1.0;
	return 1.0 - filterShadow(c, s.xy, receiver, filterR);
}

void main() {
	vec3 wpos = texture(WorldPos, fragUV).xyz;
	vec3 N = (texture(Normal, fragUV).xyz - 0.5) * 2.0;
	vec3 L = lightDir.xyz;
	float NdotL = dot(N, L);

	float viewDepth = -(matView * vec4(wpos, 1.0)).z;

	float lit = 1.0;
	if (viewDepth < cascadeParams[NUM_CASCADES - 1].x) {
		int c = NUM_CASCADES - 1;
		for (int i = 0; i < NUM_CASCADES; ++i) {
			if (viewDepth < cascadeParams[i].x) { c = i; break; }
		}

		lit = cascadeLit(c, wpos, NdotL);

		// blend into the next cascade over the last 10% of this one
		if (c + 1 < NUM_CASCADES) {
			float nearSplit = (c > 0) ? cascadeParams[c - 1].x : 0.0;
			float farSplit  = cascadeParams[c].x;
			float t = smoothstep(0.9, 1.0, (viewDepth - nearSplit) / (farSplit - nearSplit));
			if (t > 0.0) lit = mix(lit, cascadeLit(c + 1, wpos, NdotL), t);
		}

		// debug: uncomment to visualize cascades
		// outLightness = vec4(vec3(float(c) / float(NUM_CASCADES - 1)), 1); return;
	}

	vec3 inshadow = vec3(0.3);
	vec3 col = mix(inshadow, vec3(1.0), max(0.0, NdotL) * lit);
	outLightness = vec4(col, 1.0);
}