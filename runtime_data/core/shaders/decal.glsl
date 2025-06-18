#vertex
#version 450 
layout (location = 0) in vec3 inPosition;

#include "uniform_blocks/common.glsl"
#include "uniform_blocks/model.glsl"

out mat4 fragProjection;
out mat4 fragView;
out mat4 fragModel;
out vec4 frag_pos;
void main() {
	fragProjection = matProjection;
	fragView = matView;
	fragModel = matModel;         

	
	vec4 pos = matProjection * matView * matModel * vec4(inPosition, 1.0);
	frag_pos = pos;
	gl_Position = pos;
}



#fragment
#version 450

#include "uniform_blocks/common.glsl"
#include "functions/tonemapping.glsl"

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
in vec4 frag_pos;
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
	float GAMMA = 2.2;

	//vec4 frag_coord = gl_FragCoord;
	//vec2 frag_uv = frag_coord.xy / viewportSize.xy;
	//frag_uv = mix(vp_rect_ratio.xy, vp_rect_ratio.zw, frag_uv.xy);
	
	vec2 pos = (frag_pos.xy / frag_pos.w + vec2(1)) * .5;	
	vec2 uv = pos;
	vec2 frag_uv_viewport_space = uv;
	vec2 frag_uv = mix(vp_rect_ratio.xy, vp_rect_ratio.zw, frag_uv_viewport_space.xy);
	
	vec4 depth_sample = texture(Depth, frag_uv);
    vec4 normal_sample = texture(Normal, frag_uv);
	normal_sample.xyz = normal_sample.xyz * 2.0 - 1.0;
	
	vec3 world_pos = worldPosFromDepth(depth_sample.x, frag_uv_viewport_space, fragProjection, fragView);
	vec4 decal_pos = inverse(fragModel) * vec4(world_pos, 1);
	if(contains(decal_pos.xyz, -boxSize * .5, boxSize * .5) < 1.0) {
		discard;
	}
    vec3 decal_N = ((fragModel) * vec4(0, 1, 0, 0)).xyz;
    float d = max(0.0, dot(decal_N, normal_sample.xyz));
    
	vec2 decal_uv = vec2(1.0 - decal_pos.x / boxSize.x + .5, decal_pos.z / boxSize.z + .5);
	vec4 decal_sample = texture(tex, decal_uv);
	
	//decal_sample.xyz = inverseGammaCorrect(decal_sample.xyz, GAMMA);	
	decal_sample.xyz = inverseGammaCorrect(decal_sample.xyz, GAMMA);
	decal_sample.xyz = inverseTonemapFilmicUncharted2(decal_sample.xyz, .1);
	
	float alpha = (1.0 - abs(decal_pos.y / boxSize.y * 2.0)) * d;
	outAlbedo = vec4(decal_sample.xyz, decal_sample.a * alpha) * RGBA;
	//outAlbedo = vec4(1, 0, 0, 1);
	//outAlbedo = vec4(decal_N, 1);
	//outAlbedo = vec4(vec3(depth_sample.x), 1);
}
