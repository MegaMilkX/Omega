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
uniform sampler2D WorldPos;
uniform sampler2D Normal;
uniform sampler2D texNoise;
out vec4 outAO;

uniform vec3 kernel[64];

void main() {
	vec3 position = texture(WorldPos, frag_uv).xyz;
	position = (matView * vec4(position, 1)).xyz;
	vec3 normal = texture(Normal, frag_uv).xyz;
	normal = (normal - .5) * 2.0;
	normal = (matView * vec4(normal, 0)).xyz;
	normal = normalize(normal);
	vec2 noise_scale = viewportSize / 4.0;
	vec3 random_vec = texture(texNoise, frag_uv * noise_scale).xyz;
	vec3 tangent = normalize(random_vec - normal * dot(random_vec, normal));
	vec3 bitangent = cross(normal, tangent);
	mat3 TBN = mat3(tangent, bitangent, normal);
	
	float STRENGTH = 2.0;
	float RADIUS = .5;
	float BIAS = .025;
	
	float occlusion = .0;
	for(int i = 0; i < 64; ++i) {
		vec3 sample_pos = TBN * kernel[i];
		sample_pos = position + sample_pos * RADIUS;
		
		vec4 offset = vec4(sample_pos, 1.);
		offset = matProjection * offset;
		offset.xyz /= offset.w;
		offset.xyz = offset.xyz * .5 + .5;
		
		vec3 pos = texture(WorldPos, offset.xy).xyz;
		pos = (matView * vec4(pos, 1)).xyz;
		float depth = pos.z;
		
		//float rangeCheck = smoothstep(0.0, 1.0, RADIUS / abs(position.z - depth));
		float rangeCheck = smoothstep(0.0, 1.0, RADIUS / abs(depth - sample_pos.z));
		occlusion += (depth >= sample_pos.z + BIAS ? 1.0 : 0.0) * rangeCheck;
	}
	
	occlusion = 1.0 - (occlusion * STRENGTH / 64);
	outAO = vec4(occlusion, 0, 0, 1);
}
