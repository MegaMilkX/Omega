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
uniform sampler2D texRoughness;
uniform sampler2D texMetallic;
uniform sampler2D Depth;
uniform sampler2D Color;
uniform samplerCube texCubemapSpecular;
uniform sampler2D texBrdfLut;

in vec2 fragUV;
out vec4 outLightness;

#include "uniform_blocks/common.glsl"

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float linearizeDepth(float depth, float near, float far) {
    float z = depth * 2.0 - 1.0; // Convert [0,1] depth to [-1,1]
    return (2.0 * near * far) / (far + near - z * (far - near));
}

vec3 SSR(vec3 pos_frag, vec3 normal, vec3 prefilteredColor) {
	vec3 reflection_color;
	
	vec3 pos_view = (matView * vec4(pos_frag, 1)).xyz;
	vec3 N_view = mat3(matView) * normal;
	
	const int MAX_STEP_COUNT = 64;
	vec3 R = normalize(reflect(normalize(pos_view), N_view));
	float D = dot(R, N_view);
	
	if(dot(R, pos_view) < .0) {
		//return prefilteredColor;
	}
	
	const float MAX_DISTANCE = 1000.0;
	vec4 from_view = vec4(pos_view, 1);
	vec4 to_view = vec4(pos_view + (R * MAX_DISTANCE), 1);
	
	vec2 texSize = textureSize(Color, 0).xy;
	
	vec4 from_frag = from_view;
	from_frag = matProjection * from_frag;
	from_frag.xyz /= from_frag.w;
	from_frag.xyz = from_frag.xyz * .5 + .5;
	from_frag.xy *= texSize;
	vec4 to_frag = to_view;
	to_frag = matProjection * to_frag;
	to_frag.xyz /= to_frag.w;
	to_frag.xyz = to_frag.xyz * .5 + .5;
	//to_frag.x = clamp(to_frag.x, .0, 1.);
	//to_frag.y = clamp(to_frag.y, .0, 1.);
	to_frag.xy *= texSize;
	
	vec2 uv_from = from_frag.xy / texSize;
	vec2 uv_to = to_frag.xy / texSize;
	
	vec2 dir = uv_to - uv_from;
	float t_edge = 1.0;
	if (dir.x > 0.0) t_edge = min(t_edge, (1.0 - uv_from.x) / dir.x);
	if (dir.x < 0.0) t_edge = min(t_edge, (0.0 - uv_from.x) / dir.x);
	if (dir.y > 0.0) t_edge = min(t_edge, (1.0 - uv_from.y) / dir.y);
	if (dir.y < 0.0) t_edge = min(t_edge, (0.0 - uv_from.y) / dir.y);
	float fade = 1.0;
	float fadeZone = 0.1; // fade for last 10% before edge
	
	
	//reflection_color = vec3(to_frag.xy / texSize, 0);
	
	float STEP_LEN_SCREEN = MAX_DISTANCE / MAX_STEP_COUNT;
	float fSTEP = 1.0 / float(MAX_STEP_COUNT);
	for(int i = 0; i < MAX_STEP_COUNT; ++i) {
		float f = float(i + 1) * fSTEP;
		
		vec3 frag3 = mix(from_frag.xyz, to_frag.xyz, f);
		vec2 uv = frag3.xy / texSize;
		
		float depth = texture(Depth, uv).r;
		if(depth < frag3.z) {
			const int MAX_REFINE_STEP_COUNT = 16;
			
			vec2 uv_fin = uv;
			float delta = fSTEP * .5;
			f -= delta;
			for(int j = 0; j < MAX_REFINE_STEP_COUNT; ++j) {
				vec3 frag3 = mix(from_frag.xyz, to_frag.xyz, f);
				vec2 uv = frag3.xy / texSize;
				float depth = texture(Depth, uv).r;
				if(depth < frag3.z) {
					delta *= .5;
					f -= delta;
				} else {
					delta *= .5;
					f += delta;
				}
			}
			uv = uv_fin;
			
			// Fade after leaving the screen area
			vec2 uv_clamped = clamp(uv, vec2(0.0), vec2(1.0));
			float outDist = length(uv - uv_clamped);
			// fadeWidth is how far outside the edge we allow before full fade
			float fadeWidth = 0.1; // 10% of screen dimension
			float fade = 1.0 - clamp(outDist / fadeWidth, 0.0, 1.0);
			
			float fade_diff = 1.0;
			{
				float lin_z = linearizeDepth(frag3.z, zNear, zFar);
				float lin_d = linearizeDepth(depth, zNear, zFar);
				float dist_diff = lin_z - lin_d;
				{
					float fadeStart = 1.0;
					float fadeEnd = 3.0;
					fade_diff = 1.0 - clamp((dist_diff - fadeStart) / (fadeEnd - fadeStart), 0.0, 1.0);
				}
			}
			
			// fade_diff is bugged, causes banding on big objects
			vec3 col = texture(Color, uv).xyz;				
			//col = inverseGammaCorrect(col, gamma); // Not sure if needed here, should check
			reflection_color = mix(prefilteredColor, col, fade * fade_diff);
			break;
		} else {
			reflection_color = prefilteredColor;
		}
	}
	
	return reflection_color;
}

void main() {
    vec3 albedo = texture(texDiffuse, fragUV).xyz;
	vec3 N = texture(texNormal, fragUV).xyz * 2.0 - 1.0;
    vec3 worldPos = texture(texWorldPos, fragUV).xyz;
    float roughness = texture(texRoughness, fragUV).x;
    float metallic = texture(texMetallic, fragUV).x;

    vec3 V = normalize(cameraPosition - worldPos);
    vec3 R = reflect(-V, N);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);
    vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);

    const float MAX_REFLECTION_LOD = 4.0;
    vec3 prefilteredColor = textureLod(texCubemapSpecular, R * vec3(1, 1, -1), roughness * MAX_REFLECTION_LOD).xyz;
    vec2 envBRDF = texture(texBrdfLut, vec2(max(dot(N, V), 0.0), roughness)).xy;
	vec3 specular = SSR(worldPos, N, prefilteredColor) * (F * envBRDF.x + envBRDF.y);
    //vec3 specular = prefilteredColor * (F * envBRDF.x + envBRDF.y);

	vec3 originalColor = texture(Color, fragUV).xyz;
    outLightness = vec4(originalColor + specular, 1);
}