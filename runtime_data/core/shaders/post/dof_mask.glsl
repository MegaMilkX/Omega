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

float LinearizeDepth(float depth, float near, float far)
{
    float z = depth * 2.0 - 1.0; // Back to NDC
    return (2.0 * near * far) / (far + near - z * (far - near));
}

void main() {
	//float FOCUS_DIST = texture(Color, vec2(.5)).r;
	//FOCUS_DIST = LinearizeDepth(FOCUS_DIST, zNear, zFar);
	float FOCUS_DIST = 2.5;
	float FOCUS_LEN = 1.0 / 12.0;
	
	float depth = texture(Color, frag_uv).r;
	depth = LinearizeDepth(depth, zNear, zFar);
	
	float far_mask = smoothstep(0.0, 1.0, (depth - FOCUS_DIST) * FOCUS_LEN);
	float near_mask = smoothstep(0.0, 1.0, -(depth - FOCUS_DIST) * FOCUS_LEN);
	//float far_mask = min(1.0, (depth - FOCUS_DIST) * FOCUS_LEN);
	//float near_mask = min(1.0, -(depth - FOCUS_DIST) * FOCUS_LEN);
	
	outColor = vec4(far_mask, near_mask, 1.0 - clamp(depth, 0.0, 1.0), 1.0);
}
