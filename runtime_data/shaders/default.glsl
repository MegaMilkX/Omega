#vertex
#version 450 

in vec3 inPosition;
in vec3 inColorRGB;
in vec2 inUV;
in vec3 inNormal;
out vec3 pos_frag;
out vec3 col_frag;
out vec2 uv_frag;
out vec3 normal_frag;

layout(std140) uniform bufCamera3d {
	mat4 matProjection;
	mat4 matView;
};
layout(std140) uniform bufModel {
	mat4 matModel;
};

void main(){
	uv_frag = inUV;
	normal_frag = normalize((matModel * vec4(inNormal, 0)).xyz);
	pos_frag = (matModel * vec4(inPosition, 1)).xyz;
	col_frag = inColorRGB;
	vec4 pos = matProjection * matView * matModel * vec4(inPosition, 1);
	gl_Position = pos;
}

#fragment
#version 450
in vec3 pos_frag;
in vec3 col_frag;
in vec2 uv_frag;
in vec3 normal_frag;

out vec4 outAlbedo;
out vec4 outPosition;
out vec4 outNormal;
out vec4 outMetalness;
out vec4 outRoughness;

uniform sampler2D texAlbedo;
layout(std140) uniform bufCamera3d {
	mat4 matProjection;
	mat4 matView;
};

void main(){
	vec3 N = normalize(normal_frag);
	if(!gl_FrontFacing) {
		N *= -1;
	}
	vec4 pix = texture(texAlbedo, uv_frag);
	outAlbedo = vec4(pix);
	outPosition = vec4(pos_frag, 1);
	outNormal = vec4(N, 1);
	outMetalness = vec4(0.3, 0, 0, 1);
	outRoughness = vec4(0.4, 0, 0, 1);
}
