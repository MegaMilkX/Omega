#vertex
#version 450 
layout (location = 0) in vec3 inPosition;

#include "uniform_blocks/common.glsl"
#include "uniform_blocks/model.glsl"

out mat4 fragProjection;
out mat4 fragView;
out mat4 fragModel;
void main() {
	fragProjection = matProjection;
	fragView = matView;
	fragModel = matModel;         

	gl_Position = matProjection * matView * matModel * vec4(inPosition, 1.0);
}



#fragment
#version 450

#include "uniform_blocks/common.glsl"

layout(std140) uniform bufDecal {
	uniform vec3 boxSize;
	uniform vec4 RGBA;
};          
  
uniform sampler2D tex;
uniform sampler2D Normal;
uniform sampler2D Depth;
in mat4 fragProjection;
in mat4 fragView;
in mat4 fragModel;
out vec4 outAlbedo;

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

void main(){
	vec4 frag_coord = gl_FragCoord;
	float frag_u = frag_coord.x / viewportSize.x;
	float frag_v = frag_coord.y / viewportSize.y;
	vec4 depth_sample = texture(Depth, vec2(frag_u, frag_v));
    vec4 normal_sample = texture(Normal, vec2(frag_u, frag_v));
	
	vec3 world_pos = worldPosFromDepth(depth_sample.x, vec2(frag_u, frag_v), fragProjection, fragView);
	vec4 decal_pos = inverse(fragModel) * vec4(world_pos, 1);
	if(contains(decal_pos.xyz, -boxSize * .5, boxSize * .5) < 1.0) {
		discard;
	}
    vec3 decal_N = (fragModel * vec4(0, 1, 0, 0)).xyz;
    float d = dot(decal_N, normal_sample.xyz);
    
	vec2 decal_uv = vec2(1.0 - decal_pos.x / boxSize.x + .5, decal_pos.z / boxSize.z + .5);
	vec4 decal_sample = texture(tex, decal_uv);
	float alpha = (1.0 - abs(decal_pos.y / boxSize.y * 2.0)) * d;
	outAlbedo = vec4(decal_sample.xyz, decal_sample.a * alpha) * RGBA;
}
