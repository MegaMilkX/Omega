#vertex
#version 450 

in vec3 inPosition;
out vec3 frag_cube_vec;

#include "uniform_blocks/common.glsl"

void main(){
	frag_cube_vec = vec3(inPosition.x, inPosition.y, -inPosition.z);
	mat4 view = matView;
	view[3] = vec4(0,0,0,1);
	vec4 pos = matProjection * view * vec4(inPosition, 1.0);
	gl_Position = pos.xyww;
}

#fragment
#version 450

in vec3 frag_cube_vec;
uniform samplerCube cubeMap;
out vec4 outAlbedo;

#include "functions/tonemapping.glsl"
#include "uniform_blocks/common.glsl"

void main(){
	vec3 color = textureLod(cubeMap, (frag_cube_vec), 0/*(cos(time) + 1.0) * 2.0*/).xyz;
	
	// Gamma correction
	//color = gammaCorrect(tonemapFilmicUncharted2(color, 0.1), gamma);
	//color = color / (color + vec3(1.0));
	//color = pow(color, vec3(1.0/gamma));

	outAlbedo = vec4(color.xyz, 1.0);
}
