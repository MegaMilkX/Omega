#vertex
#version 460

layout(location = 0) in vec3 inPosition;
out vec2 fragUV;

#include "uniform_blocks/common.glsl"

void main() {
	vec2 uv = vec2((inPosition.x + 1.0) * .5, (inPosition.y + 1.0) * .5);
	fragUV = mix(vp_rect_ratio.xy, vp_rect_ratio.zw, uv.xy);
	
	gl_Position = vec4(inPosition, 1.0);
}


#fragment
#version 460

uniform sampler2D texDiffuse;
uniform sampler2D texNormal;
uniform sampler2D texWorldPos;
//uniform sampler2D texDepth;
uniform sampler2D texRoughness;
uniform sampler2D texMetallic;
uniform samplerCube texCubemapIrradiance;
uniform samplerCube texCubemapSpecular;
uniform sampler2D texBrdfLut;

in vec2 fragUV;
out vec4 outLightness;

#include "uniform_blocks/common.glsl"

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 worldPosFromDepth(float depth, vec2 vp_size, vec2 fragCoord, mat4 proj, mat4 view) {
	vec2 vpsz = max(vec2(1, 1), vp_size);
	vec2 normFragCoord = fragCoord.xy / vpsz;
	vec2 ndc_xy = normFragCoord * 2.0 - 1.0;
	float ndc_z = depth * 2.0 - 1.0;
	vec4 clipSpace = vec4(ndc_xy, ndc_z, 1.0);
	vec4 world4 = inverse(proj * view) * clipSpace;
	return world4.xyz / world4.w;
}

void main() {
    vec3 albedo = texture(texDiffuse, fragUV).xyz;
	vec4 N4 = texture(texNormal, fragUV).xyzw;
	float lightness_mask = N4.a;
	vec3 N = N4.xyz * 2.0 - 1.0;
    vec3 worldPos = texture(texWorldPos, fragUV).xyz;
    float roughness = texture(texRoughness, fragUV).x;
    float metallic = texture(texMetallic, fragUV).x;

	/*vec3 worldPos = worldPosFromDepth(
		texture(texDepth, fragUV).x,
		viewportSize.xy,
		gl_FragCoord.xy,
		matProjection,
		matView
	);*/
    vec3 V = normalize(cameraPosition - worldPos);
    vec3 R = reflect(-V, N);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);
    vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);

    vec3 kS = F;
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;

    const float MAX_REFLECTION_LOD = 4.0;
    vec3 prefilteredColor = textureLod(texCubemapSpecular, R * vec3(1, 1, -1), roughness * MAX_REFLECTION_LOD).xyz;
    vec2 envBRDF = texture(texBrdfLut, vec2(max(dot(N, V), 0.0), roughness)).xy;
    vec3 specular = prefilteredColor * (F * envBRDF.x + envBRDF.y);

    vec3 irradiance = texture(texCubemapIrradiance, N * vec3(1, 1, -1)).xyz;
    vec3 diffuse = irradiance * albedo;
	outLightness = vec4((kD * diffuse + specular) * lightness_mask, 1.0);
	
	//outLightness = vec4((kD * diffuse), 1.0);
}