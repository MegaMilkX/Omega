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
uniform sampler2D Depth;
out vec4 outColor;

float LinearizeDepth(float depth, float near, float far)
{
    float z = depth * 2.0 - 1.0; // Back to NDC
    return (2.0 * near * far) / (far + near - z * (far - near));
}

void main() {
	float PI2 = 6.28318530718;
	float PI = PI2 * .5;
	
	float depth = texture(Depth, frag_uv).r;
	depth = LinearizeDepth(depth, zNear, zFar);
	depth = depth - 2.5;
	depth = depth * .10;
	float blur_factor = min(1, abs(depth));
	blur_factor = blur_factor * blur_factor;
	
	float Directions = 16.0 + 32.0 * blur_factor;
	float Quality = 3.0 + 6.0 * blur_factor;
	float Size = 16.0 * blur_factor;
	
	vec2 Radius = Size / viewportSize.xy;
	vec4 color = texture(Color, frag_uv).xyzw;
	vec4 color_original = color;
	float alphaSum = color.a;
	float a = color.a;
	
	float total_weight = .0;
	for(float d = 0.0; d < PI2; d += PI2 / Directions) {
		for(float i = 1.0 / Quality; i <= 1.0; i += 1.0 / Quality) {
			vec2 offset = vec2(cos(d), sin(d)) * Radius * i;
			float weight = exp(-length(offset) * length(offset) / 2.0);
			total_weight += weight;
			
			vec4 c = texture(Color, frag_uv + offset);
			//color += c;
			color += c * weight;
			//color = max(c, color);
		}
	}
	
	color /= total_weight;//Quality * Directions;
	
	outColor = vec4(color.xyz, a);
}
