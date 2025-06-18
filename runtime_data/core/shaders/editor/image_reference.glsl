#vertex
#version 450 

in vec3 inPosition;
in vec2 inUV;
out vec2 uv_frag;

#include "uniform_blocks/common.glsl"
#include "uniform_blocks/model.glsl"

void main(){	
	uv_frag = inUV;
	vec4 pos = matProjection * matView * matModel * vec4(inPosition, 1);
	gl_Position = pos;
}

#fragment
#version 450
in vec2 uv_frag;

out vec4 outColor;

uniform sampler2D texImage;

#include "uniform_blocks/common.glsl"

void main(){
	vec4 pix = texture(texImage, uv_frag);
	outColor = vec4(pix.xyz, 1);
}
