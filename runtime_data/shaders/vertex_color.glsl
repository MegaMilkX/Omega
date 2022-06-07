#vertex
#version 450 
in vec3 inPosition;
in vec3 inColorRGB;
in vec3 inNormal;
out vec3 col_frag;
out vec3 normal_frag;

layout(std140) uniform bufCamera3d {
	mat4 matProjection;
	mat4 matView;
};
layout(std140) uniform bufModel {
	mat4 matModel;
};

void main(){
	col_frag = inColorRGB;
	vec4 pos = matProjection * matView * matModel * vec4(inPosition, 1);
	normal_frag = inNormal;
	gl_Position = pos;
}

#fragment
#version 450
in vec3 col_frag;
in vec3 normal_frag;
out vec4 outAlbedo;
void main(){
	vec3 color = col_frag;
	float lightness = dot(normal_frag, normalize(vec3(0, 0, 1))) + dot(normal_frag, normalize(vec3(0, 1, -1)));
	outAlbedo = vec4(color * lightness, 1.0f);
}
