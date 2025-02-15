#vertex
#version 450

#include "../uniform_blocks/common.glsl"

in vec3 inPosition;
out vec2 frag_uv;

void main(){
	vec2 uv = vec2((inPosition.x + 1.0) * .5, (inPosition.y + 1.0) * .5);
	frag_uv = mix(vp_rect_ratio.xy, vp_rect_ratio.zw, uv.xy);
	
	gl_Position = vec4(inPosition, 1.0);
}

#fragment
#version 450

#include "../uniform_blocks/common.glsl"

in vec2 frag_uv;
uniform sampler2D Color;
out vec4 outColor;

float rand(vec2 co){
    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

void main() {
	//float R = rand(frag_uv);
	//vec2 uv_offset = vec2(sin(frag_uv.y * 20 + time) * .05, 0);
	vec2 P = (frag_uv - vec2(.5)) * 2;
	vec2 N = normalize(P);
	float L = length(P);
	vec2 uv_offset = N * .01 * sin(L * 50 + time * 3) * clamp(L, .0, 1.);
	vec2 uv = frag_uv + uv_offset;
	//uv.x = mod(uv.x, 1);
	//uv.y = mod(uv.y, 1);
	vec4 C = texture(Color, uv);
	outColor = vec4(C.xyz, 1);
}
