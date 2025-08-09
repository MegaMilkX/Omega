#vertex
#version 450 
in vec3 inPosition;

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

void main(){
	TrailNode node = getTrailNodeCurrent();

	float half_thickness = node.scale * .5;
	
	float dir = 1.0 - mod(gl_VertexID, 2) * 2.0;
	mat4 cam = inverse(matView);
	vec3 cam_pos = (cam * vec4(0,0,0,1)).xyz;
	vec3 camN = normalize(cam_pos - node.pos);
	
	vec3 V = normalize(cross(camN, node.normal));
	vec3 final_pos = node.pos + V * half_thickness * dir;
	
	float segment_id = gl_VertexID / 2;
	fragRGBA = node.color;// * vec4(1,1,1,node.scale);
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
uniform sampler2D Depth;

in vec4 fragRGBA;
in vec2 fragUV;

out vec4 outAlbedo;

#include "uniform_blocks/common.glsl"

float LinearizeDepth(float depth, float near, float far)
{
    float z = depth * 2.0 - 1.0; // Back to NDC
    return (2.0 * near * far) / (far + near - z * (far - near));
}

void main(){
	vec4 s = texture(tex, fragUV.xy);
	
	vec2 screen_uv = gl_FragCoord.xy / viewportSize.xy;
	float depth = texture(Depth, screen_uv.xy).r;
	if(depth < gl_FragCoord.z) {
		discard;
	}
	
	const float ALPHA_TRANSITION_DIST = .5;
	float dist = LinearizeDepth(depth, zNear, zFar);
	float dist_this = LinearizeDepth(gl_FragCoord.z, zNear, zFar);
	float alpha = clamp(dist - dist_this, 0.0, 1.0);
	alpha = clamp(alpha / ALPHA_TRANSITION_DIST, 0.0, 1.0);
	alpha = smoothstep(0.0, 1.0, alpha);
	alpha = alpha * s.a * fragRGBA.a;
	
	outAlbedo = vec4(s.rgb * fragRGBA.rgb, alpha);
}
