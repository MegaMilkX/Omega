#vertex
#version 450 

in vec3 inPosition;
out vec2 frag_uv;

void main(){
	frag_uv = vec2((inPosition.x + 1.0) * .5, (inPosition.y + 1.0) * .5);
	gl_Position = vec4(inPosition, 1.0);
}

#fragment
#version 450

in vec2 frag_uv;
uniform vec3 lightDir;
uniform vec3 lightColor;
uniform float lightIntensity;
uniform vec3 camPos;
uniform sampler2D Albedo;
uniform sampler2D Position;
uniform sampler2D Normal;
uniform sampler2D Metalness;
uniform sampler2D Roughness;
uniform sampler2D Emission;
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

vec3 addDirectLight(
	vec3 camPos,
	vec3 lightDir,
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
	
	vec3 L = -lightDir;
	vec3 H = normalize(V + L);
	float attenuation = 1.0;
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
	
	float NdotL = max(dot(N, L), 0.0);
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

void main(){
	float gamma = 2.2;
	vec3 albedo = pow(texture(Albedo, frag_uv).xyz, vec3(gamma));	
	float metallic = texture(Metalness, frag_uv).x;
	float roughness = texture(Roughness, frag_uv).x;
	vec3 emission = texture(Emission, frag_uv).xyz;
	vec3 N = texture(Normal, frag_uv).xyz * 2.0 - 1.0;
	vec3 WorldPos = texture(Position, frag_uv).xyz;
	
	vec3 Lo = vec3(.0);
	Lo += addDirectLight(camPos, lightDir, lightColor, lightIntensity, WorldPos, albedo, N, metallic, roughness);
	
	outLightness = vec4(Lo, 1);
	
}
