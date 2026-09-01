#fragment
#version 460

uniform sampler2D Normal;
uniform sampler2D Depth;

#include "interface_blocks/in_vertex.glsl"
#include "interface_blocks/in_decal.glsl"
#include "uniform_blocks/common.glsl"
#include "uniform_blocks/model.glsl"
#include "uniform_blocks/decal.glsl"

float contains(vec3 pos, vec3 bottom_left, vec3 top_right) {
	vec3 s = step(bottom_left, pos) - step(top_right, pos);
	return s.x * s.y * s.z;
}

vec3 worldPosFromDepth(float depth, vec2 uv, mat4 proj, mat4 view) {
	float z = depth * 2.0 - 1.0;

	vec4 clipSpacePosition = vec4(uv * 2.0 - 1.0, z, 1.0);
	vec4 viewSpacePosition = inverse(proj) * clipSpacePosition;

	viewSpacePosition /= viewSpacePosition.w;
	
	vec4 worldSpacePosition = inverse(view) * viewSpacePosition;

	return worldSpacePosition.xyz;
}

#include "types/vertex.glsl"
#include "types/fragment.glsl"
void evalAttribFragment(inout VERTEX vert, inout FRAGMENT frag) {
	vec2 pos = (in_decal.clip_pos.xy / in_decal.clip_pos.w + vec2(1)) * .5;	
	vec2 uv = pos;
	vec2 frag_uv_viewport_space = uv;
	vec2 frag_uv = mix(vp_rect_ratio.xy, vp_rect_ratio.zw, frag_uv_viewport_space.xy);
	
	vec4 depth_sample = texture(Depth, frag_uv);
    vec4 normal_sample = texture(Normal, frag_uv);
	normal_sample.xyz = normal_sample.xyz * 2.0 - 1.0;
	
	vec3 world_pos = worldPosFromDepth(depth_sample.x, frag_uv_viewport_space, in_decal.projection, in_decal.view);
	vec4 decal_pos = inverse(in_decal.model) * vec4(world_pos, 1);
	if(contains(decal_pos.xyz, -boxSize * .5, boxSize * .5) < 1.0) {
		discard;
	}
	
    vec3 decal_N = (matModel * vec4(0, 1, 0, 0)).xyz;
    //vec3 decal_N = ((in_decal.model) * vec4(0, 1, 0, 0)).xyz;
    float d = max(0.0, dot(decal_N, normal_sample.xyz));
    
	vert.pos = world_pos.xyz;
	vert.uv = vec2(1.0 - decal_pos.x / boxSize.x + .5, decal_pos.z / boxSize.z + .5);
	vert.col = vec3(1);
	frag.alpha *= (1.0 - abs(decal_pos.y / boxSize.y * 2.0)) * d;
	frag.depth = depth_sample.x;
}

