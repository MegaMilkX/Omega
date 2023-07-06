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
uniform sampler2D Albedo;
uniform sampler2D Lightness;
uniform sampler2D Emission;
uniform sampler2D ShadowCubeMap;
out vec4 outFinal;

void main(){
	float gamma = 2.2;
	vec3 albedo = pow(texture(Albedo, frag_uv).xyz, vec3(gamma));	
	vec3 lightness = texture(Lightness, frag_uv).xyz;
	vec3 emission = texture(Emission, frag_uv).xyz;
	
	vec3 Lo = lightness;
	
	//vec3 ambient = vec3(.03) * albedo * ao;
	vec3 ambient = vec3(0);
	vec3 color = mix(ambient + Lo, ambient + emission, emission);
	
	color = color / (color + vec3(1.0));
	color = pow(color, vec3(1.0/gamma));
	
	outFinal = vec4(color, 1.0);
}
