#fragment
#version 460

layout(std140) uniform ubMaterial {
	vec4 rgba;
	float depth_bias;
	float soft_clip_dist;
};

#include "types/vertex.glsl"
#include "types/fragment.glsl"
#include "uniform_blocks/common.glsl"
#include "interface_blocks/in_vertex.glsl"
#include "util.glsl"

uniform sampler2D texAlbedo;
uniform sampler2D Depth;

float LinearizeDepth(float depth, float near, float far)
{
    float z = depth * 2.0 - 1.0; // Back to NDC
    return (2.0 * near * far) / (far + near - z * (far - near));
}

void evalFragment(in VERTEX vert, inout FRAGMENT frag) {
	//float depth_bias = .0;
	//float soft_clip_dist = .5;
	
	vec2 uv = vert.uv;
	//uv *= uv_scale;
	//uv += uv_offset;
	
	float angle_alpha = 1.0;
	float intersect_alpha = 1.0;
	
#ifdef ENABLE_DEPTH_TEST
	vec2 vpsz = max(vec2(1, 1), viewportSize);
	vec2 screen_uv = gl_FragCoord.xy / vpsz.xy;
	float depth = texture(Depth, screen_uv.xy).r;
	float dist = LinearizeDepth(depth, zNear, zFar);
	float dist_this = depth_bias + LinearizeDepth(gl_FragCoord.z, zNear, zFar);
	
	if(dist < dist_this) {
		discard;
	}
	
#ifdef ENABLE_SOFT_CLIPPING
	intersect_alpha = clamp(dist - dist_this, 0.0, 1.0);
	intersect_alpha 
		= soft_clip_dist > 0.0
		? clamp(intersect_alpha / soft_clip_dist, 0.0, 1.0)
		: 1.0;
	intersect_alpha = smoothstep(0.0, 1.0, intersect_alpha);
#endif
	
#endif
	
	// TODO:
	angle_alpha = 1;//clamp(dot(vec3(matView[2]), vert.TBN[2]), 0.0, 1.0);
	
	vec4 pix = texture(texAlbedo, uv);
	
	float alpha = intersect_alpha * angle_alpha * pix.a * rgba.a;	
	
	frag.albedo = pix.rgb * vert.col.rgb * rgba.xyz;
	frag.alpha *= alpha;
}

