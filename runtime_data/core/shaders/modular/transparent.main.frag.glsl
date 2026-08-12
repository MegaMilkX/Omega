#fragment
#version 460

out vec4 outFinal;

#include "interface_blocks/in_vertex.glsl"
#include "uniform_blocks/common.glsl"
#include "functions/tonemapping.glsl"

#include "types/vertex.glsl"
#include "types/fragment.glsl"
void evalAttribFragment(inout VERTEX vert, inout FRAGMENT frag);
void evalFragment(in VERTEX vert, inout FRAGMENT frag);


uniform samplerCube texCubemapIrradiance;
uniform samplerCube texCubemapSpecular;
uniform sampler2D texBrdfLut;

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void IBL(out vec3 out_diffuse, out vec3 out_specular, vec3 N, vec3 V, vec3 albedo, float roughness, float metallic) {
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
	
	out_diffuse = kD * diffuse;
	out_specular = specular;
}


void main(){
	vec3 N = normalize(in_vertex.normal);
	if(!gl_FrontFacing) {
		N *= -1;
	}
	
	VERTEX vert;
	FRAGMENT frag;
	{
		vert.pos = in_vertex.pos;
		vert.col = in_vertex.col;
		vert.alpha = in_vertex.alpha;
		vert.uv = in_vertex.uv;
		vert.TBN = in_vertex.TBN;
		vert.invTBN = in_vertex.invTBN;
		
		frag.albedo = vert.col;
		frag.normal = N;
		frag.light_mask = 1.0;
		frag.roughness = 1.0;
		frag.metallic = 0.0;
		frag.emission = vec3(0, 0, 0);
		frag.ao = 0.0;
		frag.alpha = vert.alpha;
		
#ifdef ENABLE_ATTRIBS		
		evalAttribFragment(vert, frag);
#endif
#ifdef ENABLE_FRAG_EXTENSION
		evalFragment(vert, frag);
#endif
	}
	
	frag.albedo = inverseGammaCorrect(frag.albedo, gamma);
	
    vec3 V = normalize(cameraPosition - vert.pos);
	vec3 ibl_diffuse;
	vec3 ibl_specular;
	IBL(ibl_diffuse, ibl_specular, frag.normal, V, frag.albedo, frag.roughness, frag.metallic);
	float spec_alpha = min(1.0, max(ibl_specular.x, max(ibl_specular.y, ibl_specular.z)));
	
	outFinal = vec4(ibl_diffuse + ibl_specular, max(frag.alpha, spec_alpha));
}

