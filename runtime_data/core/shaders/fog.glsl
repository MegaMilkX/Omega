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
uniform sampler2D Depth;
out vec4 outColor;

float linearizeDepth(float depth, float near, float far) {
    float z = depth * 2.0 - 1.0; // Convert [0,1] depth to [-1,1]
    return (2.0 * near * far) / (far + near - z * (far - near));
	//return near * far / (far + near - depth * (far - near));
}

vec3 worldPosFromDepth(float depth, vec2 uv) {
	// depth from [0, 1] to clip space z coordinate [-1, 1]
    float z = depth * 2.0 - 1.0;

    // Clip space position
    vec4 clipSpacePosition = vec4(uv * 2.0 - 1.0, z, 1.0);

    // View space position
    vec4 viewSpacePosition = inverse(matProjection) * clipSpacePosition;
    viewSpacePosition /= viewSpacePosition.w; // Perspective divide

    // World space position
    vec4 worldSpacePosition = inverse(matView) * viewSpacePosition;
    vec3 worldPosition = worldSpacePosition.xyz / worldSpacePosition.w;
	return worldPosition;
}

uniform samplerCube texCubemapIrradiance;
uniform samplerCube texCubemapEnvironment;

void main() {
	float fog_near = 60;
	float fog_far = 180;
	
	float depth = texture(Depth, frag_uv).x;
	
	vec3 worldPos = worldPosFromDepth(depth, frag_uv);
	vec3 V = cameraPosition - worldPos;
	float dist = length(V);
	
    vec3 irradiance = texture(texCubemapIrradiance, normalize(-V) * vec3(1, 1, -1)).xyz;
    vec3 environment = texture(texCubemapEnvironment, normalize(-V) * vec3(1, 1, -1)).xyz;
	
	dist = 1 * min(1, ( (1. / (fog_far - fog_near)) * max(0, dist - fog_near) ));
	dist = smoothstep(.0, 1., dist);
	//outColor = vec4(0, 0, 0, dist);
	//outColor = vec4(1, .9, .8, dist);
	//outColor = vec4(.70, .65, 1, dist);
	//outColor = vec4(mix(irradiance, environment, dist * dist * dist), dist);
	outColor = vec4(irradiance, dist);
}
