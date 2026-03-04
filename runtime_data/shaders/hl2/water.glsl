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
	
	vec2 vpsz = max(vec2(1, 1), viewportSize);
	vec2 screen_uv = gl_FragCoord.xy / vpsz.xy;
	vec3 world_color = texture(Color, screen_uv.xy).xyz;
	float depth = texture(Depth, screen_uv.xy).r;
		
	float water_alpha = 1;
	float water_alpha2 = 1;
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
		vec3 N_view = normalize(mat3(matView) * normal);
		
		const int MAX_STEP_COUNT = 32;
		vec3 R = normalize(reflect(normalize(pos_view), N_view));
		float D = dot(R, N_view);
		
		const float MAX_DISTANCE = 2000.0;
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
		to_frag.xy *= texSize;
		
		const float FADE_WIDTH = .20;
		
		float STEP_LEN_SCREEN = MAX_DISTANCE / MAX_STEP_COUNT;
		float fSTEP = 1.0 / float(MAX_STEP_COUNT);
		for(int i = 0; i < MAX_STEP_COUNT; ++i) {
			float f = float(i) * fSTEP;
			
			vec3 frag3 = mix(from_frag.xyz, to_frag.xyz, f);
			vec2 uv = frag3.xy / texSize;
			
			if(R.z > .0) {
				break;
			}
			
			float depth = texture(Depth, uv).r;	
#if 1	
			if(depth < frag3.z) {
				const int MAX_REFINE_STEP_COUNT = 16;
				vec2 uv_refined = uv;
				float delta = fSTEP * .5;
				f -= delta;
				for(int j = 0; j < MAX_REFINE_STEP_COUNT; ++j) {
					frag3 = mix(from_frag.xyz, to_frag.xyz, f);
					uv_refined = frag3.xy / texSize;
					float depth = texture(Depth, uv_refined).r;
					if(depth < frag3.z) {
						delta *= .5;
						f -= delta;
					} else {
						delta *= .5;
						f += delta;
					}
				}
				uv = uv_refined;
				
				float fade_diff = 1.0;
				{
					float lin_z = linearizeDepth(frag3.z, zNear, zFar);
					float lin_d = linearizeDepth(depth, zNear, zFar);
					float dist_diff = lin_z - lin_d;
					{
						float fadeStart = 0.1;
						float fadeEnd = 3.0;
						fade_diff = 1.0 - clamp((dist_diff - fadeStart) / (fadeEnd - fadeStart), 0.0, 1.0);
					}
				}
				
				vec2 uv_clamped = clamp(
					uv, 
					vec2(.0 + FADE_WIDTH * D, .0 + FADE_WIDTH),
					vec2(1. - FADE_WIDTH * D, 1. - FADE_WIDTH)
				);
				float out_of_bounds_dist = distance(uv, uv_clamped);
				float fade = 1.0 - clamp(out_of_bounds_dist / FADE_WIDTH, 0.0, 1.0);
				fade = smoothstep(.0, 1., fade);
				

				// fade_diff is bugged, causes banding on big objects
				vec3 col = texture(Color, uv).xyz;
				reflection_color = mix(prefilteredColor, col, fade * fade_diff);
				//reflection_color = mix(vec3(1, 0, 0), col, fade);
				//reflection_color = col;
				break;
			}
#endif
		}
	}
	
	vec3 refraction_color = vec3(0);
	float distance_traveled = 1.0 / .0;
#if 0
		const int MAX_STEP_COUNT = 32;
		const float MAX_DISTANCE = 2000.0;
		vec3 pos_view = (matView * vec4(pos_frag, 1)).xyz;
		vec3 N_view = normalize(mat3(matView) * normal);
		
		//distance_traveled = length(pos_view);
		vec3 I = normalize(pos_view);
		//vec3 R = normalize(reflect(I, N_view));
		//vec3 R = normalize(refract(I, N_view, .985));
		vec3 R = normalize(refract(I, N_view, 1.0/1.33));
		//vec3 R = normalize(refract(I, N_view, 1.33/1.0));
		
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
		to_frag.xy *= texSize;
		
		float STEP_LEN_SCREEN = MAX_DISTANCE / MAX_STEP_COUNT;
		float fSTEP = 1.0 / float(MAX_STEP_COUNT);
		for(int i = 0; i < MAX_STEP_COUNT; ++i) {
			float f = float(i) * fSTEP;
			
			vec3 frag3 = mix(from_frag.xyz, to_frag.xyz, f);
			vec2 uv = frag3.xy / texSize;
			
			if(R.z > .0) {
				break;
			}
			
			float depth = texture(Depth, uv).r;	
			if(depth < frag3.z) {
				const int MAX_REFINE_STEP_COUNT = 16;
				vec2 uv_refined = uv;
				float delta = fSTEP * .5;
				f -= delta;
				for(int j = 0; j < MAX_REFINE_STEP_COUNT; ++j) {
					frag3 = mix(from_frag.xyz, to_frag.xyz, f);
					uv_refined = frag3.xy / texSize;
					float depth = texture(Depth, uv_refined).r;
					if(depth < frag3.z) {
						delta *= .5;
						f -= delta;
					} else {
						delta *= .5;
						f += delta;
					}
				}
				uv = uv_refined;
				
				vec3 col = texture(Color, uv).xyz;
				//vec3 col = texture(Color, from_frag.xy / texSize).xyz;	
				refraction_color = vec3(col.xyz);
				
				// Override for now
				depth = texture(Depth, from_frag.xy / texSize).r;
				
				float z_ndc = depth * 2.0 - 1.0;
				vec2 ndc = uv * 2.0 - 1.0;
				vec4 clip = vec4(ndc, z_ndc, 1.0);
				vec4 view_hit = inverse(matProjection) * clip;
				view_hit /= view_hit.w;
				vec3 hit_view = view_hit.xyz;
				distance_traveled = length(hit_view - pos_view);
				
				break;
			}			
		}
#else
	{
		vec3 pos_view = (matView * vec4(pos_frag, 1)).xyz;
		vec4 from_view = vec4(pos_view, 1);
		vec4 from_frag = from_view;
		from_frag = matProjection * from_frag;
		from_frag.xyz /= from_frag.w;
		from_frag.xyz = from_frag.xyz * .5 + .5;
		
		vec2 uv = from_frag.xy;
		vec3 col = texture(Color, uv).xyz;
		refraction_color = vec3(col.xyz);
		
		float depth = texture(Depth, uv).r;	
		float z_ndc = depth * 2.0 - 1.0;
		vec2 ndc = uv * 2.0 - 1.0;
		vec4 clip = vec4(ndc, z_ndc, 1.0);
		vec4 view_hit = inverse(matProjection) * clip;
		view_hit /= view_hit.w;
		vec3 hit_view = view_hit.xyz;
		distance_traveled = length(hit_view - pos_view);
	}
#endif
	
	// Beer-Lambert absorption
	const vec3 water_color = vec3(.075, .125, .18); // Greenish blue
	//const vec3 absorption = vec3(.02, .04, .08); // very clear
	//const vec3 absorption = vec3(0.15, 0.08, 0.03); // pretty clear
	//const vec3 absorption = vec3(0.15, 0.08, 0.03);
	const vec3 absorption = vec3(.02, .04, .08) * 16.0;
	vec3 transmittance = exp(-absorption * distance_traveled);
	
    vec3 F0 = vec3(0.04);
    vec3 F = fresnelSchlickRoughness(max(dot(normal, V), 0.0), F0, roughness);
    //vec3 specular = prefilteredColor * (F * envBRDF.x + envBRDF.y);
	vec3 specular = reflection_color * (F * envBRDF.x + envBRDF.y);
	refraction_color = mix(water_color, refraction_color, transmittance);
	vec3 color = mix(refraction_color, reflection_color, (F * envBRDF.x + envBRDF.y));
	//vec3 color = refraction_color;
	//vec3 color = reflection_color;
	//vec3 color = vec3(distance_traveled) * .1;
	
	float spec_luminance = min(1.0, 0.2126 * specular.x + 0.7152 * specular.y + 0.0722 * specular.z);
	
	//outAlbedo = vec4(reflection_color, 1);
	//outAlbedo = vec4(water_color + specular, min(1.0, water_alpha + spec_luminance * water_alpha2));
	//outAlbedo = vec4(refraction_color + specular, 1.f);
	outAlbedo = vec4(color, 1.f);
}
