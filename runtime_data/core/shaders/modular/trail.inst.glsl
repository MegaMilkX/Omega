#vertex
#version 460

// Instance
in vec4 inTrailInstanceData0;

out vec4 fragRGBA;
out vec2 fragUV;

uniform samplerBuffer lutPos;

#include "uniform_blocks/common.glsl"
#include "uniform_blocks/model.glsl"

struct TrailNode {
	vec3 pos;
	float scale;
	vec4 color;
	vec3 normal;
	float uv_offset;
};

const int NUM_SEGMENTS_PER_TRAIL = 50;
const int NUM_TEXELS_PER_NODE = 3;

TrailNode getTrailNode(int segment_id) {
	TrailNode node;
	int texel_id = (segment_id + gl_InstanceID * NUM_SEGMENTS_PER_TRAIL) * NUM_TEXELS_PER_NODE;
	vec4 texel_0 = texelFetch(lutPos, texel_id);
	vec4 texel_1 = texelFetch(lutPos, texel_id + 1);
	vec4 texel_2 = texelFetch(lutPos, texel_id + 2);
	node.pos = texel_0.xyz;
	node.scale = texel_0.w;
	node.color = texel_1.xyzw;
	node.normal = texel_2.xyz;
	node.uv_offset = texel_2.w;
	
	return node;
}
TrailNode getTrailNodeCurrent() {
	return getTrailNode(gl_VertexID / 2);
}

TrailNode getTrailNodeNext() {
	return getTrailNode(gl_VertexID / 2 + 1);
}


#include "types/vertex.glsl"
void evalToWorld(inout VERTEX vert){	
	TrailNode node = getTrailNodeCurrent();

	float half_thickness = node.scale * .5;
	
	float dir = 1.0 - mod(gl_VertexID, 2) * 2.0;
	mat4 cam = inverse(matView);
	vec3 cam_pos = (cam * vec4(0,0,0,1)).xyz;
	vec3 camN = normalize(cam_pos - node.pos);
	
	vec3 V = normalize(cross(camN, -node.normal));
	vec3 final_pos = node.pos + V * half_thickness * dir;
	
	vec3 B = V;
	vec3 T = normalize(getTrailNodeNext().pos - node.pos);
	vec3 N = normalize(cross(T, B));
	
	//float segment_id = gl_VertexID / 2;
	//fragRGBA = node.color;
	//fragUV = vec2(node.uv_offset, mod(gl_VertexID, 2));
	
	vert.col = node.color.xyz;
	vert.alpha *= node.color.w;
	vert.uv = vec2(node.uv_offset, mod(gl_VertexID, 2));
	vert.world_pos = final_pos.xyz;
	vert.pos = vec3(matView * vec4(final_pos.xyz, 1));
	vert.tangent = T;
	vert.bitangent = B;
	vert.TBN = mat3(T, B, N);
}
