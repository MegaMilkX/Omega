#vertex
#version 450 
in vec3 inUVLightmap;

#include "uniform_blocks/common.glsl"
#include "uniform_blocks/model.glsl"

void main(){
	gl_Position = matProjection * matView * vec4(inUVLightmap.x, inUVLightmap.y, 0.0, 1.0);
}

#fragment
#version 450
out vec4 outAlbedo;

void main(){
	outAlbedo = vec4(1, 1, 1, 1);
}
