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
uniform vec3 lightPos;
uniform vec3 lightColor;
uniform float lightIntensity;
uniform vec3 camPos;
uniform sampler2D Albedo;
uniform sampler2D Position;
uniform sampler2D Normal;
uniform sampler2D Metalness;
uniform sampler2D Roughness;
uniform sampler2D Emission;
uniform samplerCube ShadowCubeMap;
out vec4 outLightness;

const float PI = 3.14159265359;



float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
	
    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return num / denom;
}
float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return num / denom;
}
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);
	
    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}


vec3 calcPointLightness(vec3 frag_pos, vec3 normal, vec3 light_pos, float radius, vec3 L_col) {
	vec3 light_vec = light_pos - frag_pos;
	float dist = length(light_vec);
	vec3 light_dir = normalize(light_vec);
	float lightness = max(dot(normal, light_dir), .0);
	float attenuation = clamp(1.0 - dist*dist/(radius*radius), 0.0, 1.0);
	attenuation *= attenuation;
	return L_col * lightness * attenuation;
}

vec3 addLight(
	vec3 camPos,
	vec3 lightPos,
	vec3 lightColor,
	float lightIntensity,
	vec3 WorldPos,
	vec3 albedo,
	vec3 N,
	float metallic,
	float roughness
){
	vec3 V = normalize(camPos - WorldPos);
	
	vec3 F0 = vec3(.04);
	F0 = mix(F0, albedo, metallic);
	
	vec3 L = normalize(lightPos - WorldPos);
	vec3 H = normalize(V + L);
	float distance = length(lightPos - WorldPos);
	float attenuation = 1.0 / (distance * distance);
	vec3 radiance = lightColor * lightIntensity * attenuation;
	
	float NDF = DistributionGGX(N, H, roughness);
	float G = GeometrySmith(N, V, L, roughness);
	vec3 F = fresnelSchlick(max(dot(H, V), .0), F0);
	
	vec3 kS = F;
	vec3 kD = vec3(1.0) - kS;
	kD *= 1.0 - metallic;
	
	vec3 numerator = NDF * G * F;
	float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
	vec3 specular = numerator / denominator;
	
	float NdotL = max(dot(N, L), .0);
	return (kD * albedo / PI + specular) * radiance * NdotL;
}

float vectorToDepthValue(vec3 Vec) {
    vec3 AbsVec = abs(Vec);
    float LocalZcomp = max(AbsVec.x, max(AbsVec.y, AbsVec.z));

    const float f = 1000.0;
    const float n = 0.1;
    float NormZComp = (f+n) / (f-n) - (2*f*n)/(f-n)/LocalZcomp;
    return (NormZComp + 1.0) * 0.5;
}

float linearizeDepth(float depth, float near, float far) {
	//depth = (2.0 * depth - 1.0);
	//return depth * (far - near) + near;
	//return (2.0 * near * far) / (far + near - depth * (far - near));
	return near * far / (far + near - depth * (far - near));
	//return (2.0 * near * far) / (far + near - depth * (far - near));
	//return near * far / (far + depth * (near - far));
}

vec3 hsv2rgb(vec3 c)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

void main(){
	float gamma = 2.2;
	vec3 albedo = pow(texture(Albedo, frag_uv).xyz, vec3(gamma));	
	float metallic = texture(Metalness, frag_uv).x;
	float roughness = texture(Roughness, frag_uv).x;
	vec3 emission = texture(Emission, frag_uv).xyz;
	vec3 N = texture(Normal, frag_uv).xyz * 2.0 - 1.0;
	vec3 WorldPos = texture(Position, frag_uv).xyz;
	
	vec3 Lo = vec3(.0);
	Lo += addLight(camPos, lightPos, lightColor, lightIntensity, WorldPos, albedo, N, metallic, roughness);
	
	vec3 light_to_surf = normalize(WorldPos - lightPos);
	float surf_dist = length(WorldPos - lightPos);
	//float shadow_bias = max(0.001 * (1.0 - dot(N, -light_to_surf)), 0.0005);
	float shadow_bias = .0005;
	float calc_depth = vectorToDepthValue(WorldPos - lightPos) - shadow_bias;
	
	float shadow = .0;
	float depth_diff = .0;
	float distance_diff = .0;
	float hard_shadow = .0;
	float shadow_depth = texture(ShadowCubeMap, light_to_surf).x;
	float occluder_lin_depth = linearizeDepth(shadow_depth, .1, 1000.);
	{		
		float PI2 = 6.28318530718;
		float PI = PI2 * .5;
		
		float Directions = 16.0;
		float Quality = 3.0;
		float Size = 16.0;
		
		vec3 occluder_pos = lightPos + light_to_surf * occluder_lin_depth;
		distance_diff = length(WorldPos - occluder_pos);
		Size = mix(0, 16, clamp(distance_diff * .25, 0, 1));
		
		vec3 Radius = Size / vec3(1024, 1024, 1024);
		shadow = shadow_depth < calc_depth ? 0 : 1.;
		hard_shadow = shadow;
		//shadow = texture(ShadowCubeMap, vec4(light_to_surf, calc_depth));
		
		
		for(float d = 0.0; d < PI2; d += PI2 / Directions) {
			for(float i = 1.0 / Quality; i <= 1.0; i += 1.0 / Quality) {
				vec3 lts = light_to_surf + vec3(cos(d), sin(d), sin(d)) * Radius * i;
				vec4 tex_coord = vec4(normalize(lts), calc_depth);				
				float shadow_depth = texture(ShadowCubeMap, lts).x;
				float s = shadow_depth < calc_depth ? 0 : 1.;
				//float s = texture(ShadowCubeMap, tex_coord);
				shadow += s;
			}
		}
		
		shadow /= Quality * Directions;
	}
	
	//float shadow = texture(ShadowCubeMap, vec4(light_to_surf, calc_depth));

	//vec3 rgb = hsv2rgb(vec3(clamp(distance_diff * .05, 0, 1), 1, 1));
	//outLightness = vec4(Lo * (shadow) + (1. - hard_shadow) * rgb, 1);
	//outLightness = vec4(vec3(distance_diff), 1);
	outLightness = vec4(Lo * (shadow), 1);
}
