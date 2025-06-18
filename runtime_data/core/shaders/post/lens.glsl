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
#include "functions/tonemapping.glsl"

in vec2 frag_uv;
uniform sampler2D Color;
out vec4 outColor;

vec2 radialDistortion(vec2 coord, float k1, float k2, float k3) {
	float r = length(coord);
	float distortionFactor = 1.0 + k1 * pow(r, 2) + k2 * pow(r, 4) + k3 * pow(r, 6);
	return distortionFactor * coord;
}

vec2 tangentialDistortion(vec2 coord, float p1, float p2) {
	float x = coord.x;
    float y = coord.y;
    float r2 = x * x + y * y;
    float dx = 2.0 * p1 * x * y + p2 * (r2 + 2.0 * x * x);
    float dy = p1 * (r2 + 2.0 * y * y) + 2.0 * p2 * x * y;
    return vec2(dx, dy);
}

void main() {
	vec2 uv = frag_uv * 2.0 - 1.0;
	vec2 radial = radialDistortion(uv, .01, .01, .01);
	vec2 tangential = tangentialDistortion(uv, 0, 0);
	
	uv = radial + tangential;
	uv = (uv + 1.0) * .5;
	
	//vec4 color = texture(Color, frag_uv).xyzw;
	vec4 color = texture(Color, uv).xyzw;
	
	outColor = vec4(color.xyz, color.w);
}

