#vertex
#version 450 
in vec3 inPosition;

layout(std140) uniform bufShadowmapCamera3d {
	mat4 matProjection;
	mat4 matView;
};
#include "uniform_blocks/model.glsl"

void main(){
	gl_Position = matProjection * matView * matModel * vec4(inPosition, 1.0);
}

#fragment
#version 450
layout(location = 0) out float outAlbedo;

void main(){
	outAlbedo = gl_FragCoord.z;
}
