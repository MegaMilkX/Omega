#vertex
#version 460

in vec3 inPosition;

#include "uniform_blocks/common.glsl"
#include "uniform_blocks/model.glsl"

void main(){
	gl_Position = matProjection * matView * matModel * vec4(inPosition.xyz, 1.0);
}

#fragment
#version 460

out vec4 Frag;

void main() {
	Frag = vec4(1, 1, 1, 1);
}
