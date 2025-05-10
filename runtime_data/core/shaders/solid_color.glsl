#vertex
#version 450 
in vec3 inPosition;

#include "uniform_blocks/common.glsl"
#include "uniform_blocks/model.glsl"

void main(){
	gl_Position = matProjection * matView * matModel * vec4(inPosition.xyz, 1.0);
}

#fragment
#version 450
out vec4 outAlbedo;

uniform vec4 color;

void main(){
	// vec4(.1, .3, 1, 1)
	outAlbedo = color;
}
