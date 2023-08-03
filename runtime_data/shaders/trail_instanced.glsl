#vertex
#version 450 
in vec3 inPosition;

// Instance
in vec4 inTrailInstanceData0;

out vec4 fragRGBA;
out vec2 fragUV;

uniform samplerBuffer lutPos;

layout(std140) uniform bufCamera3d {
	mat4 matProjection;
	mat4 matView;
	vec2 screenSize;
};
layout(std140) uniform bufModel {
	mat4 matModel;
};

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

void main(){
	TrailNode node = getTrailNodeCurrent();

	float half_thickness = .2;
	
	float dir = 1.0 - mod(gl_VertexID, 2) * 2.0;
	mat4 cam = inverse(matView);
	vec3 cam_pos = (cam * vec4(0,0,0,1)).xyz;
	vec3 camN = normalize(cam_pos - node.pos);
	
	vec3 final_pos = node.pos + normalize(cross(camN, node.normal)) * half_thickness * dir;
	
	float segment_id = gl_VertexID / 2;
	fragRGBA = node.color * vec4(1,1,1,node.scale);
	//fragUV = vec2(-node.uv_offset, mod(gl_VertexID, 2));
	fragUV = vec2(node.uv_offset, mod(gl_VertexID, 2));
	//fragUV = vec2(segment_id / NUM_SEGMENTS_PER_TRAIL, mod(gl_VertexID, 2));
	//fragUV = vec2(node.uv_offset / inTrailInstanceData0.x, mod(gl_VertexID, 2));
	
	gl_Position = matProjection * matView * vec4(final_pos, 1.0);
}

#fragment
#version 450
//uniform sampler2D lut;
uniform sampler2D tex;

in vec4 fragRGBA;
in vec2 fragUV;

out vec4 outAlbedo;

void main(){
	vec4 s = texture(tex, fragUV.xy);
	outAlbedo = vec4(1,1,1, s.x) * fragRGBA;
	//outAlbedo = vec4(1,1,1,1);
}
