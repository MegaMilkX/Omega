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
uniform sampler2D DOFMask;
out vec4 outColor;

float LinearizeDepth(float depth, float near, float far)
{
    float z = depth * 2.0 - 1.0; // Back to NDC
    return (2.0 * near * far) / (far + near - z * (far - near));
}

vec3 blurFar(vec3 original_color, float factor) {
	float PI2 = 6.28318530718;
	float PI = PI2 * .5;
	
	vec3 color = vec3(0,0,0);
	
	float Directions = 16.0 + 32.0 * factor;
	float Quality = 3.0 + 6.0 * factor;
	float Size = 32.0 * factor;
	
	vec2 Radius = Size / viewportSize.xy;
	
	float total_weight = 0.000001;
	for(float d = 0.0; d < PI2; d += PI2 / Directions) {
		for(float i = 1.0 / Quality; i <= 1.0; i += 1.0 / Quality) {
			vec2 offset = vec2(cos(d), sin(d)) * Radius * i;
			float mask_weight = texture(DOFMask, frag_uv + offset).r;
			float weight = exp(-length(offset) * length(offset) / 2.0);
			weight *= mask_weight;
			total_weight += weight;
			
			vec3 c = texture(Color, frag_uv + offset).xyz;
			color += c * weight;
		}
	}
	
	color /= total_weight;
	return color;
}

vec3 blurNear(vec3 original_color, float factor) {
	float PI2 = 6.28318530718;
	float PI = PI2 * .5;
	
	vec3 color = original_color;
	
	float Directions = 16.0 + 32.0 * factor;
	float Quality = 3.0 + 6.0 * factor;
	float Size = 32.0 * factor;
	
	vec2 Radius = Size / viewportSize.xy;
	
	float total_weight = 1;
	for(float d = 0.0; d < PI2; d += PI2 / Directions) {
		for(float i = 1.0 / Quality; i <= 1.0; i += 1.0 / Quality) {
			vec2 offset = vec2(cos(d), sin(d)) * Radius * i;
			float weight = exp(-length(offset) * length(offset) / 2.0);
			total_weight += weight;
			
			vec3 c = texture(Color, frag_uv + offset).xyz;
			color += c * weight;
		}
	}
	
	color /= total_weight;
	return color;
}

float blurNearMask(float original_value, float factor) {
	float PI2 = 6.28318530718;
	float PI = PI2 * .5;
	
	float value = original_value;
	
	float Directions = 16.0 + 32.0 * factor;
	float Quality = 3.0 + 6.0 * factor;
	float Size = 32.0 * factor;
	
	vec2 Radius = Size / viewportSize.xy;
	
	float total_weight = 1;
	for(float d = 0.0; d < PI2; d += PI2 / Directions) {
		for(float i = 1.0 / Quality; i <= 1.0; i += 1.0 / Quality) {
			vec2 offset = vec2(cos(d), sin(d)) * Radius * i;
			float v = texture(DOFMask, frag_uv + offset).g;
			float weight = exp(-length(offset) * length(offset) / 2.0);
			weight *= v;
			total_weight += weight;
			
			value += v * weight;
		}
	}
	
	value /= total_weight;
	return value;
}

void main() {	
	vec4 color = texture(Color, frag_uv).xyzw;
	float a = color.a;
	
	vec3 mask = texture(DOFMask, frag_uv).rgb;
	float far_mask = mask.r;
	float near_mask = mask.g;
	float mid_mask = 1 - mask.r - mask.g;
	
	vec3 color_far = blurFar(color.xyz, far_mask);
	vec3 color_near = blurNear(color.xyz, near_mask);
	near_mask = blurNearMask(near_mask, near_mask);
	
	vec3 C = vec3(0, 0, 0);
	//C = color_far.xyz;
	C = mix(color.xyz, color_far.xyz, far_mask);
	C = mix(C, color_near.xyz, near_mask);
	outColor = vec4(C, a);
	//outColor = vec4(mask.r, mask.g, 0, 1);
	//outColor = vec4(0, mask.g, 0, 1);
	//outColor = vec4(0, 0, 1 - mask.r - mask.g, 1);
}
