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

#include "functions/tonemapping.glsl"
#include "uniform_blocks/common.glsl"

in vec2 frag_uv;
uniform sampler2D Albedo;
uniform sampler2D Lightness;
uniform sampler2D Emission;
uniform sampler2D AmbientOcclusion;
out vec4 outFinal;


void main(){
	//vec3 albedo = pow(texture(Albedo, frag_uv).xyz, vec3(gamma));	
	vec3 albedo = texture(Albedo, frag_uv).xyz;
	//albedo.xyz = inverseGammaCorrect(inverseTonemapFilmicUncharted2(albedo.xyz, .1), gamma);
	vec3 lightness = texture(Lightness, frag_uv).xyz;
	vec3 emission = texture(Emission, frag_uv).xyz;
	float ao = texture(AmbientOcclusion, frag_uv).x;
	
	vec3 Lo = lightness;
	
	//vec3 ambient = vec3(.03) * albedo * ao;
	//vec3 ambient = vec3(.35) * albedo;
    vec3 ambient = vec3(0.0);
	//vec3 color = ambient + Lo * ao;
	//vec3 color = mix(ambient + Lo * ao, ambient + emission, emission);
	vec3 color = ambient + Lo * ao + albedo * emission;
	
	// Gamma correction
	//color = gammaCorrect(tonemapFilmicUncharted2(color, 0.1), gamma);
	color += emission;
	
	outFinal = vec4(color, 1.0);
}
