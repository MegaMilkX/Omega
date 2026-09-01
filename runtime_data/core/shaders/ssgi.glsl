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

mat4 invProjection;

vec3 reconstructViewPos(vec2 uv, float depth) {
    vec4 clip = vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 view = invProjection * clip;
    return view.xyz / view.w;
}

vec2 viewToScreenUV(vec3 viewPos) {
    vec4 clip = matProjection * vec4(viewPos, 1.0);
    vec2 ndc = clip.xy / clip.w;
    return ndc * 0.5 + 0.5;
}

uniform sampler2D Depth;
uniform sampler2D Normal;

bool marchRay(vec3 viewOrigin, vec3 rayDir, out vec3 hitViewPos) {
	const int STEPS = 16;
	const float MAX_DIST = 4.0;
	
	float stepSize = MAX_DIST / float(STEPS);
	vec3 rayPos = viewOrigin;
	vec3 prevPos = viewOrigin;
	
	for(int i = 0; i < STEPS; ++i) {
		prevPos = rayPos;
		rayPos += rayDir * stepSize;
		
		if(rayPos.z > -.01) {
			return false;
		}
		
		vec2 uv = viewToScreenUV(rayPos);
		if(uv.x < .0 || uv.x > 1.0 || uv.y < .0 || uv.y > 1.0) {
			return false;
		}
		
		float sceneDepth = texture(Depth, uv).x;
		if(sceneDepth > 1.0) {
			continue;
		}
		
		vec3 scenePos = reconstructViewPos(uv, sceneDepth);
		
		if(rayPos.z < scenePos.z) {
			const int REFINE_STEPS = 5;
			vec3 lo = prevPos;
			vec3 hi = rayPos;
			vec3 refinedScenePos = scenePos;
			vec2 refinedUV = uv;
			
			for(int j = 0; j < REFINE_STEPS; ++j) {
				vec3 mid = (lo + hi) * .5;
				vec2 midUV = viewToScreenUV(mid);
				
				float midDepth = texture(Depth, midUV).x;
				if(midDepth > 1.0) {
					lo = mid;
					continue;
				}
				
				vec3 midScenePos = reconstructViewPos(midUV, midDepth);
				if(mid.z < midScenePos.z) {
					hi = mid;
					refinedScenePos = midScenePos;
					refinedUV = midUV;
				} else {
					lo = mid;
				}
			}
			
			vec3 V = scenePos - viewOrigin;
			if(dot(normalize(V), rayDir) < .9) {
				continue;
			}
			
			if(dot(V, V) > MAX_DIST * MAX_DIST) {
				break;
			}
			/*
			vec3 N = (texture(Normal, uv).xyz - 0.5) * 2.0;
			N = (matView * vec4(N, 0)).xyz;
			if(dot(N, rayDir) >= .0) {
				break;
			}*/
			
			hitViewPos = scenePos;
			return true;
		}
	}
	
	return false;
}

const float PI = 3.14159265;
const int SAMPLE_COUNT = 8;

float hash12(vec2 p) {
	p = fract(p * vec2(123.34, 456.21));
	p += dot(p, p + 45.32);
	return fract(p.x * p.y);
}

vec3 cosineSampleHemisphere(vec2 xi) {
	float r = sqrt(xi.x);
	float theta = 2.0 * PI * xi.y;
	return vec3(r * cos(theta), r * sin(theta), sqrt(max(.0, 1.0 - xi.x)));
}

in vec2 frag_uv;
uniform sampler2D PrevLightness;
uniform sampler2D ORMM;
out vec4 outSSGI_Test;

void main() {
	invProjection = inverse(matProjection);
	vec2 uv = frag_uv;
	
	float light_mask = texture(ORMM, uv).w;
	
    float depth = texture(Depth, uv).r;
    if (depth >= 1.0) {
		outSSGI_Test = vec4(0.0);
		return;
	}
	
	vec3 viewPos = reconstructViewPos(uv, depth);
	vec3 normal = (texture(Normal, uv).xyz - 0.5) * 2.0;
	vec3 viewNormal = (matView * vec4(normal, .0)).xyz;
	
	vec3 up = abs(viewNormal.z) < .999 ? vec3(0, 0, 1) : vec3(1, 0, 0);
	vec3 tangent = normalize(cross(up, viewNormal));
	vec3 bitangent = cross(viewNormal, tangent);
	mat3 TBN = mat3(tangent, bitangent, viewNormal);
	
	float rotOffset = hash12(uv * viewportSize);
	
	vec3 accum = vec3(0);
	for(int i = 0; i < SAMPLE_COUNT; ++i) {
		float xi1 = hash12(uv + float(i) * 13.7 + time);
		float xi2 = fract(hash12(uv + float(i) * 91.3 + time) + rotOffset);
		
		vec3 sampleDirTS = cosineSampleHemisphere(vec2(xi1, xi2));
		vec3 rayDir = normalize(TBN * sampleDirTS);
		
		vec3 hitViewPos;
		if(marchRay(viewPos, rayDir, hitViewPos)) {
			vec2 hitUV = viewToScreenUV(hitViewPos);
			accum += texture(PrevLightness, hitUV).xyz;
		}
	}
	
	outSSGI_Test = vec4(accum / float(SAMPLE_COUNT) * light_mask, 1);
}
