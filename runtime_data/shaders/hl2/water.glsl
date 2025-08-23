#vertex
#version 450 

in vec3 inPosition;
in vec3 inColorRGB;
in vec2 inUV;
in vec2 inUVLightmap;
in vec3 inNormal;
in vec3 inTangent;
in vec3 inBitangent;
out vec3 pos_frag;
out vec3 col_frag;
out vec2 uv_frag;
out vec2 uv_lightmap_frag;
out vec3 normal_frag;
out mat3 fragTBN;

#include "uniform_blocks/common.glsl"
#include "uniform_blocks/model.glsl"

void main(){
	vec3 T = normalize(vec3(matModel * vec4(inTangent, 0.0)));
	vec3 B = normalize(vec3(matModel * vec4(inBitangent, 0.0)));
	vec3 N = normalize(vec3(matModel * vec4(inNormal, 0.0)));
	fragTBN = mat3(T, B, N);
	
	uv_frag = inUV;
	uv_lightmap_frag = inUVLightmap;
	normal_frag = normalize((matModel * vec4(inNormal, 0)).xyz);
	pos_frag = (matModel * vec4(inPosition, 1)).xyz;
	col_frag = inColorRGB;
	vec4 pos = matProjection * matView * matModel * vec4(inPosition, 1);
	gl_Position = pos;
}

#fragment
#version 450
in vec3 pos_frag;
in vec3 col_frag;
in vec2 uv_frag;
in vec2 uv_lightmap_frag;
in vec3 normal_frag;
in mat3 fragTBN;

out vec4 outAlbedo;
out vec4 outPosition;
out vec4 outMetalness;
out vec4 outEmission;
out vec4 outAmbientOcclusion;

uniform sampler2D texAlbedo;
uniform sampler2D texNormal;
uniform sampler2D texEmission;
uniform sampler2D texAmbientOcclusion;
uniform sampler2D Depth;
uniform sampler2D Color;
uniform samplerCube texCubemapIrradiance;
uniform samplerCube texCubemapSpecular;
uniform sampler2D texBrdfLut;

#include "uniform_blocks/common.glsl"
#include "functions/tonemapping.glsl"

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

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

vec3 viewPosFromDepth(vec2 uv, float depth) {
	vec3 raw_pos = vec3(uv, depth);
	vec4 sspos = vec4(raw_pos * 2.0 - 1.0, 1);
	vec4 view_pos = inverse(matProjection) * sspos;
	return view_pos.xyz / view_pos.w;
}

void main(){
	vec3 N = normalize(normal_frag);
	if(!gl_FrontFacing) {
		N *= -1;
	}
	//N = N * 2.0 - 1.0;
	//N = normalize(fragTBN * N);
	float water_speed = .1;
	vec2 uv_offset0 = normalize(vec2(1, 1)) * time * water_speed;
	vec2 uv_offset1 = normalize(vec2(1, 5)) * time * .75 * water_speed;
	vec2 uv_offset2 = normalize(vec2(5, 1)) * time * .50 * water_speed;
	vec3 n0 = texture(texNormal, uv_frag + uv_offset0).xyz;
	vec3 n1 = texture(texNormal, uv_frag * 0.75 + uv_offset1).xyz;
	vec3 n2 = texture(texNormal, uv_frag * 1.75 + uv_offset2).xyz;
	vec3 normal = (n0 + n1 + n2) / 3.0;
	normal = normal * 2.0 - 1.0;

	mat3 tbn = fragTBN;
	tbn[0] = normalize(tbn[0]);
	tbn[1] = normalize(tbn[1]);
	tbn[2] = normalize(tbn[2]);
	normal = normalize(tbn * normal);
	if(!gl_FrontFacing) {
		normal *= -1;
	}
	
	const float roughness = .0;
    vec3 V = normalize(cameraPosition - pos_frag);
    vec3 R = reflect(-V, normal);
    vec2 envBRDF = texture(texBrdfLut, vec2(max(dot(normal, V), 0.0), roughness)).xy;
    const float MAX_REFLECTION_LOD = 4.0;
    vec3 prefilteredColor = textureLod(texCubemapSpecular, R * vec3(1, 1, -1), roughness * MAX_REFLECTION_LOD).xyz;
	
	vec4 pix = texture(texAlbedo, uv_frag);
	vec3 emission = texture(texEmission, uv_frag).xyz;
	vec4 ao = texture(texAmbientOcclusion, uv_frag);
	//vec3 specular = texture(texCubemapEnvironment, normal).xyz;
	//lo.xyz = lo.xyz * pow(2.0, lo.a * 255);
	
	//lo.xyz = inverseGammaCorrect(lo.xyz, gamma);
	pix.xyz = inverseGammaCorrect(pix.xyz, gamma);
	
	vec2 screen_uv = gl_FragCoord.xy / viewportSize.xy;
	vec3 world_color = texture(Color, screen_uv.xy).xyz;
	float depth = texture(Depth, screen_uv.xy).r;
		
	float water_alpha = 1;
	float water_alpha2 = 1;
	vec3 water_color = vec3(.075, .125, .18);
	{	
		if(depth < gl_FragCoord.z) {
			discard;
		}
		
		//vec3 v_refraction = refract(normalize(pos_frag - cameraPosition), normal, 1.0 / 1.33); // air to water
		//refract(normalize(pos_frag - cameraPosition), normal, 1.33 / 1.0); // water to air
		//v_refraction = (matProjection * matView * vec4(v_refraction, .0)).xyz;
		
		vec3 back_color = texture(Color, screen_uv.xy).xyz;
		
		//water_color = water_color * back_color;
		//water_color = vec3(v_refraction.xy, 0);
		
		const float ALPHA_TRANSITION_DIST = .1;
		const float ALPHA_TRANSITION_DIST2 = .05;
		vec3 backWorldPos = worldPosFromDepth(depth, screen_uv);
		vec3 frontWorldPos = pos_frag;
		
		float water_depth = length(backWorldPos - frontWorldPos);
		//water_alpha = clamp(water_alpha / ALPHA_TRANSITION_DIST, 0.0, 1.0);
		water_alpha = water_depth / ALPHA_TRANSITION_DIST;
		water_alpha2 = water_depth / ALPHA_TRANSITION_DIST2;
		water_alpha = clamp(1.0 - exp(-water_alpha * .05), .0, 1.);
		water_alpha2 = clamp(1.0 - exp(-water_alpha2 * .05), .0, 1.);
		
		//water_alpha = smoothstep(0.0, 1.0, pow(water_alpha, 1.5));
		//water_alpha = smoothstep(0.0, 1.0, water_alpha);
	}
	
	vec3 reflection_color = prefilteredColor;
	{
		vec3 pos_view = (matView * vec4(pos_frag, 1)).xyz;
		vec3 N_view = mat3(matView) * normal;
		
		const int MAX_STEP_COUNT = 64;
		vec3 R = normalize(reflect(normalize(pos_view), N_view));
		float D = dot(R, N_view);
		
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
			float f = float(i) * fSTEP;
			
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
				reflection_color = mix(prefilteredColor, col, fade);
				break;
			}
		}
	}
	
    vec3 F0 = vec3(0.04);
    vec3 F = fresnelSchlickRoughness(max(dot(normal, V), 0.0), F0, roughness);
    //vec3 specular = prefilteredColor * (F * envBRDF.x + envBRDF.y);
	vec3 specular = reflection_color * (F * envBRDF.x + envBRDF.y);
	
	float spec_luminance = min(1.0, 0.2126 * specular.x + 0.7152 * specular.y + 0.0722 * specular.z);
	
	outAlbedo = vec4(water_color + specular, min(1.0, water_alpha + spec_luminance * water_alpha2));
	//outAlbedo = vec4(reflection_color, min(1.0, water_alpha + spec_luminance * water_alpha2));
	
	outPosition = vec4(pos_frag, 1);
	outEmission = vec4(emission, 1);
	outAmbientOcclusion = vec4(ao.xyz, 1);
}
