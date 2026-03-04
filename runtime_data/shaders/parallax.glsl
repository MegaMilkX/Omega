#vertex
#version 450 

in vec3 inPosition;
in vec3 inColorRGB;
in vec2 inUV;
in vec3 inNormal;
in vec3 inTangent;
in vec3 inBitangent;

#include "interface_blocks/out_vertex.glsl"
#include "uniform_blocks/common.glsl"
#include "uniform_blocks/model.glsl"

void main(){
	vec3 T = normalize(vec3(matModel * vec4(inTangent, 0.0)));
	vec3 B = normalize(vec3(matModel * vec4(inBitangent, 0.0)));
	vec3 N = normalize(vec3(matModel * vec4(inNormal, 0.0)));
	out_vertex.TBN = mat3(T, B, N);
	out_vertex.invTBN = inverse(out_vertex.TBN);
	
	vec4 scrTo = (matProjection * matView * matModel * vec4(inPosition, 1));
	vec4 scrFrom = (matProjection * matView * matModel_prev * vec4(inPosition, 1));
	//scrTo.xyz /= scrTo.w;
	//scrFrom.xyz /= scrFrom.w;
	
	out_vertex.uv = inUV;
	out_vertex.normal = normalize((matModel * vec4(inNormal, 0)).xyz);
	out_vertex.pos = (matModel * vec4(inPosition, 1)).xyz;
	//out_vertex.velo = scrTo.xyz - scrFrom.xyz;
	out_vertex.scr_from = scrFrom;
	out_vertex.scr_to = scrTo;
	out_vertex.col = inColorRGB;
	
	vec4 pos = matProjection * matView * matModel * vec4(inPosition, 1);
	gl_Position = pos;
}

#fragment
#version 450

out vec4 outAlbedo;
out vec4 outPosition;
out vec4 outNormal;
out vec4 outMetalness;
out vec4 outRoughness;
out vec4 outAmbientOcclusion;
out vec4 outLightness;
out vec4 outVelocityMap;

uniform sampler2D texAlbedo;
uniform sampler2D texNormal;
uniform sampler2D texRoughness;
uniform sampler2D texMetallic;
uniform sampler2D texEmission;
uniform sampler2D texAmbientOcclusion;
uniform sampler2D texDisplacement;

#include "interface_blocks/in_vertex.glsl"
#include "uniform_blocks/common.glsl"
#include "functions/tonemapping.glsl"

void main(){
	vec3 N_surfToCam = normalize(cameraPosition - in_vertex.pos);
	float D = dot(N_surfToCam, vec3(in_vertex.TBN[2]));
	
	float invD = 1.0 / D;
	vec2 uv = in_vertex.uv;
	const int MAX_PARALLAX_ITERATIONS = 64;
	const float PARALLAX_DEPTH = .1;
	const float PARALLAX_STEP = PARALLAX_DEPTH / float(MAX_PARALLAX_ITERATIONS);
	for(int i = 0; i < MAX_PARALLAX_ITERATIONS; ++i) {
		float depth = float(i + 1) / float(MAX_PARALLAX_ITERATIONS);
		float disp = texture(texDisplacement, uv).r;
		disp = 1.0 - disp;
		if(depth > disp) {
			break;
		}
		vec3 n = in_vertex.invTBN * N_surfToCam;
		uv -= n.xy * invD * PARALLAX_STEP;
	}
	
	vec3 N = normalize(in_vertex.normal);
	if(!gl_FrontFacing) {
		N *= -1;
	}
	//N = N * 2.0 - 1.0;
	//N = normalize(TBN_frag * N);
	vec3 normal = texture(texNormal, uv).xyz;
	normal = normal * 2.0 - 1.0;
	mat3 tbn = in_vertex.TBN;
	tbn[0] = normalize(tbn[0]);
	tbn[1] = normalize(tbn[1]);
	tbn[2] = normalize(tbn[2]);
	normal = normalize(tbn * normal);
	if(!gl_FrontFacing) {
		normal *= -1;
	}
	
	vec4 pix = texture(texAlbedo, uv);
	float roughness = texture(texRoughness, uv).x;
	float metallic = texture(texMetallic, uv).x;
	vec3 emission = texture(texEmission, uv).xyz;
	vec4 ao = texture(texAmbientOcclusion, uv);
	
	emission.xyz = pix.xyz * emission.xyz * 4.0;
	
	pix.xyz = inverseGammaCorrect(pix.xyz, gamma);
	emission.xyz = inverseGammaCorrect(emission.xyz, gamma);
	
	vec3 velo
		= in_vertex.scr_to.xyz / in_vertex.scr_to.w
		- in_vertex.scr_from.xyz / in_vertex.scr_from.w;
		
	outAlbedo = vec4(pix.rgb * in_vertex.col.rgb, pix.a);
	outPosition = vec4(in_vertex.pos, 1);
	outNormal = vec4((normal + 1.0) / 2.0, 1);
	outMetalness = vec4(metallic, 0, 0, 1);
	outRoughness = vec4(roughness, 0, 0, 1);
	outAmbientOcclusion = vec4(ao.xyz, 1);
	outLightness = vec4(emission.xyz * pix.rgb, 1);
	outVelocityMap = vec4(velo, 1);
}
